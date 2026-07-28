/*  SPDX-FileCopyrightText: (c) 2020 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "check_internal.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace sak{
	static auto Is_closer_signal(Lines const &mask, int const row, int const col)->bool;

	static auto Glued_declarator_tail(
		std::string const &m, int const from, bool const decor_head
	)->bool;
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

// §9.4 — 다중행 꺾쇠도 다중행 괄호와 같은 공행 봉투를 요구한다(§5 서두 — 꺾쇠도 괄호류).
void sak::Check_angle_blank_line(
	Lines const &lines, Lines const &mask, Angle_pair const &a, std::vector<Violation> &out
){
	Check_close_open_blank_line(lines, mask, a.o_row, a.c_row, a.c_col, 1, '<', out);
}

// Angle_pair 벡터를 위치 기준으로 정렬해 완전히 동일한 중복 쌍을 제거한다(여러 매처의
// 결과를 병합할 때·중첩 template 안 static_cast 가 두 번 잡히는 자리를 정리).
void sak::Dedup_angles(std::vector<Angle_pair> &v){
	std::sort(
		v.begin(), v.end(),
		[](Angle_pair const &a, Angle_pair const &b)->bool{
			if(a.o_row != b.o_row){
				return a.o_row < b.o_row;
			}

			if(a.o_col != b.o_col){
				return a.o_col < b.o_col;
			}

			if(a.c_row != b.c_row){
				return a.c_row < b.c_row;
			}

			return a.c_col < b.c_col;
		}
	);

	v.erase(
		std::unique(
			v.begin(), v.end(),
			[](Angle_pair const &a, Angle_pair const &b)->bool{
				return
					a.o_row == b.o_row && a.o_col == b.o_col
					&& a.c_row == b.c_row && a.c_col == b.c_col
				;
			}
		),
		v.end()
	);
}

// template/static_cast/dynamic_cast/const_cast/reinterpret_cast 뒤 `<` 부터 짝 `>` 를
// depth 추적으로 찾는다. `>>` 는 두 개의 close 이벤트로 분해해 각각 pair 를 방출.
// `(...)` `[...]` 내부는 통째로 skip (내부 표현식에서 나체 `<`/`>` 는 안전).
auto sak::Match_template_cast_angles(Lines const &mask)->std::vector<Angle_pair>{
	std::vector<Angle_pair> out;
	int const rows = static_cast<int>(mask.size());

	for(int r = 0; r < rows; ++r){
		std::string const &m = mask[r];
		int const n = static_cast<int>(m.size());
		int c = 0;

		while(c < n){
			if( !Word_starts_at(m, c) ){
				++c;

				continue;
			}

			std::string const w = Word_at(m, c);
			int const e = c + static_cast<int>(w.size());

			bool const  
				is_anchor
				= w == "template" || w == "static_cast"
				|| w == "dynamic_cast" || w == "const_cast" || w == "reinterpret_cast"
			;

			if(!is_anchor){
				c = e;

				continue;
			}

			// 앵커 뒤에서 첫 여는 `<` 를 찾는다.
			//  - `template` 은 뒤에 식별자·::·keyword·개행이 낄 수 있음(예: `obj.template
			//    get<T>()`, `template class Foo<T>;`, `template\n<...>`). 문장 경계
			//    (`;`/`{`/`}`) 나 예상 밖 기호를 만나면 abort.
			//  - `*_cast` 는 문법상 공백만 skip 후 즉시 `<` 여야 함.
			int p_r = r;
			int p_c = e;
			bool found_open = false;

			if(w == "template"){
				while(true){
					if(p_r >= rows){
						break;
					}

					std::string const &row_m = mask[p_r];
					int const row_n = static_cast<int>(row_m.size());

					if(p_c >= row_n){
						++p_r;
						p_c = 0;

						continue;
					}

					char const ch = row_m[p_c];

					if(ch == ' ' || ch == '\t' || ch == '@'){
						++p_c;

						continue;
					}

					if(ch == '<'){
						found_open = true;

						break;
					}

					if( is_word_char(ch) ){
						while( p_c < row_n && is_word_char(row_m[p_c]) ){
							++p_c;
						}

						continue;
					}

					if(ch == ':' && p_c + 1 < row_n && row_m[p_c + 1] == ':'){
						p_c += 2;

						continue;
					}

					break;
				}
			}
			else{
				while( p_c < n && (m[p_c] == ' ' || m[p_c] == '\t' || m[p_c] == '@') ){
					++p_c;
				}

				if(p_c < n && m[p_c] == '<'){
					found_open = true;
				}
			}

			if(!found_open){
				c = e;

				continue;
			}

			std::string const &m_at_open = mask[p_r];
			int const n_at_open = static_cast<int>(m_at_open.size());

			// `<=` `<<` `<=>` 는 template 시작 아님 → skip.
			if(
				p_c + 1 < n_at_open
				&& (m_at_open[p_c + 1] == '=' || m_at_open[p_c + 1] == '<')
			){
				c = e;

				continue;
			}

			// 빈 `<>` (§5.1 — 내용 없는 괄호는 괄호 표현 아님).
			if(p_c + 1 < n_at_open && m_at_open[p_c + 1] == '>'){
				c = e;

				continue;
			}

			// depth 추적 시작. 열린 위치 스택.
			struct Open_pos{ int r, c; };

			std::vector<Open_pos> open_stack;
			open_stack.push_back({ p_r, p_c });

			int scan_r = p_r;
			int scan_c = p_c + 1;
			bool aborted = false;
			std::vector<Angle_pair> local;

			while(!open_stack.empty() && !aborted){
				if(scan_r >= rows){
					aborted = true;

					break;
				}

				std::string const &sm = mask[scan_r];
				int const sn = static_cast<int>(sm.size());

				if(scan_c >= sn){
					++scan_r;
					scan_c = 0;

					continue;
				}

				char const ch = sm[scan_c];

				if(ch == '(' || ch == '['){
					char const close_ch = ch == '(' ? ')' : ']';
					int inner = 1;
					++scan_c;

					while(inner > 0 && scan_r < rows){
						if( scan_c >= static_cast<int>(mask[scan_r].size()) ){
							++scan_r;
							scan_c = 0;

							continue;
						}

						char const cc = mask[scan_r][scan_c];

						if(cc == ch){
							++inner;
						}
						else if(cc == close_ch){
							--inner;
						}

						++scan_c;
					}

					continue;
				}

				if(
					ch == '<' && scan_c + 2 < sn
					&& sm[scan_c + 1] == '=' && sm[scan_c + 2] == '>'
				){
					scan_c += 3;

					continue;
				}

				if(scan_c + 1 < sn){
					char const nx = sm[scan_c + 1];

					if( ch == '<' && (nx == '=' || nx == '<') ){
						scan_c += 2;

						continue;
					}

					if(ch == '>' && nx == '='){
						Open_pos const op = open_stack.back();
						open_stack.pop_back();
						local.push_back({ op.r, op.c, scan_r, scan_c });
						scan_c += 2;

						continue;
					}

					if(ch == '>' && nx == '>'){
						if(open_stack.size() < 2){
							aborted = true;

							break;
						}

						Open_pos const op1 = open_stack.back();
						open_stack.pop_back();
						local.push_back({ op1.r, op1.c, scan_r, scan_c });
						Open_pos const op2 = open_stack.back();
						open_stack.pop_back();
						local.push_back({ op2.r, op2.c, scan_r, scan_c + 1 });
						scan_c += 2;

						continue;
					}
				}

				if(ch == '<'){
					open_stack.push_back({ scan_r, scan_c });
					++scan_c;

					continue;
				}

				if(ch == '>'){
					Open_pos const op = open_stack.back();
					open_stack.pop_back();
					local.push_back({ op.r, op.c, scan_r, scan_c });
					++scan_c;

					continue;
				}

				if(ch == ';' || ch == '{'){
					aborted = true;

					break;
				}

				++scan_c;
			}

			if(!aborted && open_stack.empty()){
				for(Angle_pair const &a : local){
					out.push_back(a);
				}
			}

			c = e;
		}
	}

	// 중복 제거 (중첩 template 안의 static_cast 등이 두 번 잡히는 자리 처리).
	Dedup_angles(out);

	return out;
}

// `>` 뒤 첫 의미 토큰이 "표현식을 시작할 수 없는 토큰"(닫힘 신호)인지 판정한다. 이런
// 자리의 `>` 는 이항 비교/시프트일 수 없어 닫는 꺾쇠로 확정된다((col) 은 그 토큰의 시작).
auto sak::Is_closer_signal(Lines const &mask, int const row, int const col)->bool{
	std::string const &m = mask[row];
	int const n = static_cast<int>(m.size());
	char const c0 = m[col];
	char const c1 = col + 1 < n ? m[col + 1] : '\0';

	// 1문자 신호 — 구조 토큰과 단항형 없는 연산자(복합·중복형은 첫 문자로 포섭).
	if( std::string("{;,)]}?/%|^=").find(c0) != std::string::npos ){
		return true;
	}

	// 2문자 신호 — 둘째 문자로 단항형·`::`·리터럴·fold 를 갈라낸다.
	if(c0 == ':'){
		return c1 != ':';  // `::` 은 표현식 시작 가능 → 배제
	}

	if(c0 == '&'){
		return c1 == '&';  // `&&` 만 (단독 `&` 는 주소 연산자)
	}

	if(c0 == '!'){
		return c1 == '=';  // `!=` 만 (단독 `!` 는 논리 부정)
	}

	if(c0 == '+' || c0 == '*'){
		return c1 == '=';  // `+=` `*=` 만 (단독 `+` `*` 는 단항형)
	}

	if(c0 == '-'){
		return c1 == '=' || c1 == '>';  // `-=` `->` (`->*` 자동 포섭)
	}

	if(c0 == '.'){
		// `.`+숫자는 부동리터럴(`a > .5`), `..`(→`...`)은 fold — 둘 다 표현식 시작.
		unsigned char const u1 = static_cast<unsigned char>(c1);

		return c1 != '.' && !std::isdigit(u1);
	}

	// 키워드 신호 — 정확 단어 경계(`const_cast`·`static_cast` 등 접두 오인 방지).
	if( Word_starts_at(m, col) ){
		std::string const w = Word_at(m, col);

		return
			w == "const" || w == "constexpr"
			|| w == "volatile" || w == "static"
		;
	}

	return false;
}

// 닫힘 신호로 앵커되는 꺾쇠. `>` 뒤 첫 의미 토큰이 닫힘 신호(Is_closer_signal)면 그 `>`
// 를 닫는 꺾쇠로 확정하고 역방향으로 짝 `<` 를 찾아 쌍(중첩 포함)을 방출한다. `template`/
// `*_cast` 앵커의 보완재로, 인스턴스화·상속·후행반환·brace-init·특수화·변수 템플릿을 커버.
auto sak::Match_closer_anchored_angles(Lines const &mask)->std::vector<Angle_pair>{
	std::vector<Angle_pair> out;
	int const rows = static_cast<int>(mask.size());

	for(int r = 0; r < rows; ++r){
		std::string const &m = mask[r];
		int const n = static_cast<int>(m.size());

		for(int c = 0; c < n; ++c){
			if(m[c] != '>'){
				continue;
			}

			// 후보 `>` 자체 제외 — `>=`·`>>` 런 중간, `->`·`<=>` 조각.
			if( c + 1 < n && (m[c + 1] == '=' || m[c + 1] == '>') ){
				continue;
			}

			if( c > 0 && (m[c - 1] == '-' || m[c - 1] == '=') ){
				continue;
			}

			// operator>·operator>> 이름의 `>` 제외 — `>` 런을 건너 직전 단어 확인.
			int w = c;

			while(w > 0 && m[w - 1] == '>'){
				--w;
			}

			if( Word_before(mask, r, w) == "operator" ){
				continue;
			}

			// `>` 다음 첫 의미 토큰이 닫힘 신호인지.
			int sr = r;
			int sc = c + 1;

			if( !Next_code(mask, rows - 1, sr, sc) ){
				continue;
			}

			if( !Is_closer_signal(mask, sr, sc) ){
				continue;
			}

			// 역방향 열거 — 후보 `>` 를 pending-close 스택에 넣고 왼쪽으로 짝 `<` 를 찾는다.
			struct Close_pos{ int r, c; };

			std::vector<Close_pos> close_stack;
			std::vector<Angle_pair> local;
			close_stack.push_back({ r, c });

			int br = r;
			int bc = c - 1;
			bool aborted = false;

			while(!close_stack.empty() && !aborted){
				if(br < 0){
					aborted = true;

					break;
				}

				if(bc < 0){
					--br;

					if(br < 0){
						aborted = true;

						break;
					}

					bc = static_cast<int>(mask[br].size()) - 1;

					continue;
				}

				char const ch = mask[br][bc];

				if(ch == ' ' || ch == '\t' || ch == '@'){
					--bc;

					continue;
				}

				if(ch == ')' || ch == ']'){
					char const op = ch == ')' ? '(' : '[';

					if( !Match_bracket_back(mask, op, ch, br, bc) ){
						aborted = true;

						break;
					}

					--bc;

					continue;
				}

				// 짝 없는 여는 괄호·문장 경계 → 짝 `<` 를 못 찾음 → abort.
				if(ch == '(' || ch == '[' || ch == '{' || ch == '}' || ch == ';'){
					aborted = true;

					break;
				}

				if(ch == '>'){
					// `<=>`·`->` 조각이면 그 토큰 전체를 건너뛴다.
					if(bc >= 2 && mask[br][bc - 1] == '=' && mask[br][bc - 2] == '<'){
						bc -= 3;

						continue;
					}

					if(bc > 0 && mask[br][bc - 1] == '-'){
						bc -= 2;

						continue;
					}

					close_stack.push_back({ br, bc });
					--bc;

					continue;
				}

				if(ch == '<'){
					// operator< 이름의 `<` 는 괄호 아님.
					if( Word_before(mask, br, bc) == "operator" ){
						--bc;

						continue;
					}

					// `<<` 는 잘 형성된 인자 목록에 나체로 못 옴 → abort.
					if(bc > 0 && mask[br][bc - 1] == '<'){
						aborted = true;

						break;
					}

					if(close_stack.empty()){
						aborted = true;

						break;
					}

					Close_pos const cl = close_stack.back();
					close_stack.pop_back();
					local.push_back({ br, bc, cl.r, cl.c });
					--bc;

					continue;
				}

				--bc;
			}

			if(!aborted && close_stack.empty()){
				for(Angle_pair const &a : local){
					out.push_back(a);
				}
			}
		}
	}

	Dedup_angles(out);

	return out;
}

// §5.4 다중행 꺾쇠 레이아웃 — 여는 `<` 이 여는 행 마지막·닫는 `>` 이 닫는 행 첫·여닫는
// 들여쓰기 동일·중간 코드 행 들여쓰기 ≥ 여는 행 +1.
// `>>` 로 분해된 두 pair 는 각각 검사되지만 c_col 이 인접해 자연히 성립한다.
void sak::Check_multiline_angle(
	Lines const &lines, Lines const &mask, Angle_pair const &p, std::vector<Violation> &out
){
	if(p.o_row == p.c_row){
		return;
	}

	int const o_ind = Indent_depth(lines[p.o_row]);
	int const c_ind = Indent_depth(lines[p.c_row]);

	if(o_ind != c_ind){
		Push_fix(
			out, { p.c_row, 0, "5.4", "angle: open/close indent differ" },
			Fix_kind::indent, 0, o_ind
		);
	}

	std::string const &o_line = mask[p.o_row];
	int const o_n = static_cast<int>(o_line.size());

	for(int cc = p.o_col + 1; cc < o_n; ++cc){
		if( Is_code_char(o_line[cc]) ){
			out.push_back({ p.o_row, cc, "5.4", "angle: opening '<' not last token" });

			break;
		}
	}

	std::string const &c_line = mask[p.c_row];

	for(int cc = 0; cc < p.c_col; ++cc){
		if( Is_code_char(c_line[cc]) ){
			// `>>` 로 분해된 둘째 `>` 는 첫 `>` 뒤라 이 조건이 자연 걸림 —
			// 그 자리는 첫 `>` pair 가 이미 잡아주므로 이 pair 는 skip.
			if(cc + 1 == p.c_col && c_line[cc] == '>'){
				break;
			}

			out.push_back({ p.c_row, cc, "5.4", "angle: closing '>' not first token" });

			break;
		}
	}

	int const rows = static_cast<int>(lines.size());

	for(int r = p.o_row + 1; r < p.c_row && r < rows; ++r){
		if( lines[r].empty() || !Has_code(mask[r]) ){
			continue;
		}

		if( Indent_depth(lines[r]) < o_ind + 1 ){
			Push_fix(
				out, { r, 0, "5.4", "angle: middle line indent insufficient" },
				Fix_kind::indent, 0, o_ind + 1
			);
		}
	}
}

// §5.7 특수괄호 닫힘 — 다중행 닫는 `>` 뒤에 개행 외 코드 토큰 없음.
void sak::Check_angle_close_last(
	Lines const &mask, Angle_pair const &p, std::vector<Violation> &out
){
	if(p.o_row == p.c_row){
		return;
	}

	std::string const &c_line = mask[p.c_row];
	int const cn = static_cast<int>(c_line.size());

	for(int cc = p.c_col + 1; cc < cn; ++cc){
		if( Is_code_char(c_line[cc]) ){
			// `>>` 로 분해된 첫 `>` 뒤 바로 다음 `>` 는 자기 짝의 c_col 이라 예외.
			if(cc == p.c_col + 1 && c_line[cc] == '>'){
				break;
			}

			// §4.3 — 다중행 닫는 `>` 뒤에 피연산 토큰(단어)이 이어지면 `▽` 가 개행하므로 `>` 가
			// 행 끝에 남아야 한다(위반). 부착·종결 토큰(`(`·`::`·`;` 등)은 `▽` 자리가 아니라 침묵.
			if( is_word_char(c_line[cc]) ){
				out.push_back(
					{ p.c_row, cc, "4.3", "'>' before an operand: it must end the line" }
				);
			}

			break;
		}
	}
}

// 다중행 꺾쇠의 닫는 `>` 뒤로 이어지는 `::식별자(::식별자)*` 타입 사슬의 끝(마지막 식별자
// 바로 뒤) 열을 돌려준다. 사슬 꼴이 아니면 -1.
auto sak::Angle_chain_end(std::string const &m, int const from)->int{
	int const n = static_cast<int>(m.size());
	int i = from;

	if(i + 1 >= n || m[i] != ':' || m[i + 1] != ':'){
		return -1;
	}

	while(i + 1 < n && m[i] == ':' && m[i + 1] == ':'){
		i += 2;

		if( i >= n || !is_word_char(m[i]) ){
			return -1;
		}

		while( i < n && is_word_char(m[i]) ){
			++i;
		}
	}

	return i;
}

// P5 약식 꼬리 — 타입이 끝난 지점 뒤가 `<공백> 식별자 (;|,|=)` 꼴(선언자를 타입 닫는 행에
// 붙인 약식)인지 본다. 그 밖(호출 `(`·부착 `{`·표현식 `==` 등)은 거짓(위양성 0).
// decor_head 는 식별자 앞의 데코레이터 `*`·`&` 허용 여부 — 인라인 타입의 `}` 는 피연산자가
// 될 수 없어 곱셈·비트 AND 와 모호하지 않으므로 중괄호 쪽만 허용한다(꺾쇠 사슬 끝은 값일 수
// 있어 불허).
auto sak::Glued_declarator_tail(
	std::string const &m, int const from, bool const decor_head
)->bool{
	int const n = static_cast<int>(m.size());
	int i = from, ws = 0;

	while( i < n && (m[i] == ' ' || m[i] == '\t') ){
		++i;
		++ws;
	}

	if(ws == 0){
		return false;
	}

	if(decor_head){
		while( i < n && (m[i] == '*' || m[i] == '&') ){
			++i;
		}
	}

	if( i >= n || !is_word_char(m[i]) ){
		return false;
	}

	while( i < n && is_word_char(m[i]) ){
		++i;
	}

	while( i < n && (m[i] == ' ' || m[i] == '\t') ){
		++i;
	}

	return i < n && (m[i] == ';' || m[i] == ',' || m[i] == '=');
}

// §5.5 P5 — 다중행 타입의 변수 선언은 가상 괄호를 전개해야 한다. 다중행 `<...>` 의 닫는 행이
// `>::사슬 <공백> 식별자 (;|,|=)` 꼴이면 위반. 좁게 — 사슬과 약식 꼬리가 정확히 맞는 자리만
// 본다(위양성 0).
void sak::Check_declarator_expansion(
	Lines const &mask, Angle_pair const &p, std::vector<Violation> &out
){
	if(p.o_row == p.c_row){
		return;
	}

	int const i = Angle_chain_end(mask[p.c_row], p.c_col + 1);

	if( i >= 0 && Glued_declarator_tail(mask[p.c_row], i, false) ){
		out.push_back(
			{ p.c_row, p.c_col, "5.5", "multi-line declaration must expand its virtual bracket" }
		);
	}
}

// §5.5 P5 — 인라인 타입 정의도 동일하다. 다중행 `{...}` 로 타입을 정의한 변수 선언문의 닫는
// 행이 `} <공백> 식별자 (;|,|=)` 꼴이면 위반. 선언자가 하나든 여럿이든 같다.
void sak::Check_declarator_expansion_brace(
	Lines const &mask, std::vector<Bk_pair> const &pairs, std::vector<Violation> &out
){
	for(Bk_pair const &p : pairs){
		bool const  
			target
			= p.o_row != p.c_row && Is_inline_type_close(mask, p)
			&& Glued_declarator_tail(mask[p.c_row], p.c_col + p.c_len, true)
		;

		if(target){
			out.push_back(
				{
					p.c_row, p.c_col, "5.5",
					"multi-line declaration must expand its virtual bracket"
				}
			);
		}
	}
}

// §8.4 경계 공백 — 여는 `<` 직전 word 는 무공백, 닫는 `>` 직후 word 는 공백·`(` `[` 은 무공백.
void sak::Check_angle_boundary(
	Lines const &mask, Angle_pair const &p, std::vector<Violation> &out
){
	std::string const &o_line = mask[p.o_row];

	if(p.o_col > 0){
		char const prev = o_line[p.o_col - 1];

		if(prev == ' ' || prev == '\t'){
			// 앞이 공백이면, 그 앞의 코드가 word 인지 확인
			int q = p.o_col - 1;

			while( q > 0 && (o_line[q] == ' ' || o_line[q] == '\t') ){
				--q;
			}

			if( q >= 0 && is_word_char(o_line[q]) ){
				Push_fix(
					out, { p.o_row, p.o_col, "8.4", "angle: space between word and '<'" },
					Fix_kind::gap_left, p.o_col, 0
				);
			}
		}
	}

	std::string const &c_line = mask[p.c_row];
	int const cn = static_cast<int>(c_line.size());
	int q = p.c_col + 1;

	while( q < cn && (c_line[q] == ' ' || c_line[q] == '\t') ){
		++q;
	}

	if(q >= cn){
		return;
	}

	char const nx = c_line[q];
	bool const has_space = q > p.c_col + 1;

	if(nx == '(' || nx == '['){
		if(has_space){
			Push_fix(
				out, { p.c_row, p.c_col + 1, "8.4", "angle: space between '>' and '(' or '['" },
				Fix_kind::gap_right, p.c_col + 1, 0
			);
		}
	}
	else if( is_word_char(nx) ){
		if(!has_space){
			Push_fix(
				out, { p.c_row, p.c_col + 1, "8.4", "angle: '>' and word need one space" },
				Fix_kind::gap_right, p.c_col + 1, 1
			);
		}
	}
}

// §8.5 안쪽 공백 n — 단일행 꺾쇠는 자기 안 최대 중첩 단계 +1 (자기 자리 포함).
// pairs 전체를 참조해 이 pair 안에 몇 겹의 단일행 꺾쇠가 있는지 센다.
void sak::Check_angle_inner_space(
	Lines const &mask, std::vector<Angle_pair> const &pairs,
	Angle_pair const &p, std::vector<Violation> &out
){
	if(p.o_row != p.c_row){
		return;
	}

	// 최대 중첩 단계: 이 pair 안에 포함된 단일행 pair 들의 최대 자체 중첩 +1.
	// 재귀 없이 iterative — 자기가 포함하는 pair 들의 depth 를 recursion 으로 계산.
	// (pairs 는 정렬되어 있고 크기 작음 — O(N^2) 로 충분.)
	auto  
		nest_depth
		= [&pairs](Angle_pair const &a, auto &self)->int{
			int max_inner = -1;

			for(Angle_pair const &b : pairs){
				if(b.o_row != b.c_row){
					continue;
				}

				if(&b == &a){
					continue;
				}

				bool const  
					inside
					= ( b.o_row > a.o_row || (b.o_row == a.o_row && b.o_col > a.o_col) )
					&& ( b.c_row < a.c_row || (b.c_row == a.c_row && b.c_col < a.c_col) )
				;

				if(!inside){
					continue;
				}

				int const d = self(b, self);

				if(d > max_inner){
					max_inner = d;
				}
			}

			return max_inner + 1;
		}
	;

	int const n = nest_depth(p, nest_depth);
	std::string const &m = mask[p.o_row];
	int const line_n = static_cast<int>(m.size());

	int left = 0;
	int q = p.o_col + 1;

	while( q < line_n && (m[q] == ' ' || m[q] == '\t') ){
		++left;
		++q;
	}

	if(q >= line_n){
		return;
	}

	int right = 0;
	int r = p.c_col - 1;

	while( r > p.o_col && (m[r] == ' ' || m[r] == '\t') ){
		++right;
		--r;
	}

	if(left != n){
		Push_fix(
			out, { p.o_row, p.o_col + 1, "8.5", "angle: inner space must be N" },
			Fix_kind::gap_right, p.o_col + 1, n
		);
	}

	if(right != n){
		Push_fix(
			out, { p.o_row, p.c_col - right, "8.5", "angle: inner space must be N" },
			Fix_kind::gap_left, p.c_col, n
		);
	}
}
