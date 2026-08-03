/*  SPDX-FileCopyrightText: (c) 2020 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "check_internal.hpp"

#include <string>
#include <vector>

namespace sak{
	// 콜론 낫괄호의 두 갈래 — 상속·enum 기반 타입 지정이냐, 생성자 멤버초기화 리스트냐.
	enum class Colon_vb_kind{ inherit_or_enum, ctor_init };

	static auto Next_code_row(Lines const &lines, Lines const &mask, int const from)->int;

	static auto Find_stmt_semi(
		Lines const &mask, int const r, int const from, int &close_row, int &close_col
	)->bool;

	static void Push_anchor_indent_check(
		Lines const &lines, Lines const &mask, int const cur_row, char const *msg,
		std::vector<Violation> &out
	);

	static void Check_colon_cbracket_layout(
		Lines const &lines, Lines const &mask,
		int const a_row, int const a_col, Colon_vb_kind const kind,
		std::vector<Violation> &out
	);

	static void Scan_type_decl_colon(
		Lines const &lines, Lines const &mask, std::vector<Violation> &out
	);

	static void Scan_ctor_init_colon(
		Lines const &lines, Lines const &mask, std::vector<Violation> &out
	);

	static auto Marker_decl_close_row(Lines const &mask, int const r)->int;
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

// from 다음의 코드 행(공행과 주석·전처리 등 무코드 행은 건너뜀)의 번호. 없으면 행 수(rows).
auto sak::Next_code_row(Lines const &lines, Lines const &mask, int const from)->int{
	int const rows = static_cast<int>(lines.size());
	int nr = from + 1;

	while(nr < rows){
		if( !Is_blank_row(lines[nr]) && Has_code(mask[nr]) ){
			break;
		}

		++nr;
	}

	return nr;
}

// (r, from) 부터 행을 넘어가며 ()[]{} 통합 깊이 추적으로 depth-0 의 짝 `;` 를 찾는다.
// 찾으면 close_row·close_col 에 위치를 채우고 true.
auto sak::Find_stmt_semi(
	Lines const &mask, int const r, int const from, int &close_row, int &close_col
)->bool{
	int const rows = static_cast<int>(mask.size());
	int depth = 0;

	for(int rr = r; rr < rows; ++rr){
		std::string const &cm = mask[rr];
		int const cn = static_cast<int>(cm.size());
		int const start = rr == r ? from : 0;

		for(int cc = start; cc < cn; ++cc){
			char const ch = cm[cc];

			if(ch == '(' || ch == '[' || ch == '{'){
				++depth;
			}
			else if(ch == ')' || ch == ']' || ch == '}'){
				--depth;
			}
			else if(ch == ';' && depth == 0){
				close_row = rr;
				close_col = cc;

				return true;
			}
		}
	}

	return false;
}

// 다음 코드 행을 찾고, 그 들여쓰기 ≥ cur+1 이 아니면 §5.5 위반(msg)으로 기록.
void sak::Push_anchor_indent_check(
	Lines const &lines, Lines const &mask, int const cur_row, char const *msg,
	std::vector<Violation> &out
){
	int const rows = static_cast<int>(lines.size());
	int const nr = Next_code_row(lines, mask, cur_row);

	if(nr >= rows){
		return;
	}

	if( int const cur = Indent_depth(lines[cur_row]); Indent_depth(lines[nr]) < cur + 1 ){
		Push_fix(out, { nr, 0, "5.5", msg }, Fix_kind::indent, 0, cur + 1);
	}
}

// §5.5 낫괄호 — return/throw/using 앵커 (같은 구조: 여는 키워드 ~ 닫는 ';').
// 세 조건 검사: (a) 여는 키워드가 행 마지막 코드 토큰인지, (b) 짝 ';' 가 그 행 첫 코드
// 토큰인지, (c) 다음 코드 행 들여쓰기 ≥ cur+1 (내용 +1). 세 키워드는 정본 §5.5 표에서
// 동일 구조(open=keyword, close=';')이므로 단일 매처로 통합. 그 외 앵커(`->`·`case`·
// 변수선언·`:`)는 의미적 판정이 더 필요해 별도.
void sak::Check_anchor_keyword_semicolon(
	Lines const &lines, Lines const &mask, std::vector<Violation> &out
){
	int const rows = static_cast<int>(lines.size());

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

			if(w != "return" && w != "throw" && w != "using"){
				c = e;

				continue;
			}

			// (r, e) 부터 행을 넘어가며 깊이 추적으로 짝 ; 를 찾는다.
			int close_row = -1, close_col = -1;
			Find_stmt_semi(mask, r, e, close_row, close_col);

			// 같은 행에서 ; 를 찾았으면 단일행 return/throw — 낫괄호 미발현.
			if(close_row == r){
				c = e;

				continue;
			}

			// (a) 여는 키워드는 행 마지막 코드 토큰이어야 한다.
			bool keyword_last = true;

			for(int cc = e; cc < n; ++cc){
				if( Is_code_char(m[cc]) ){
					keyword_last = false;

					out.push_back({ r, cc, "5.5", "return/throw/using: keyword not last" });

					break;
				}
			}

			if(close_row >= 0){
				std::string const &cm = mask[close_row];

				for(int cc = 0; cc < close_col; ++cc){
					if( Is_code_char(cm[cc]) ){
						out.push_back(
							{ close_row, cc, "5.5", "return/throw/using: ';' not first" }
						);

						break;
					}
				}
			}

			// 여는 키워드가 행 마지막일 때에만 다음 행 들여쓰기를 검사한다.
			// (키워드 뒤에 표현식이 이어지는 형태는 위 (a) 가 이미 잡았고,
			//  그 경우 "다음 코드 행"이 낫괄호 내용의 첫 행이 아니라
			//  중간 괄호의 닫는 행이 될 수 있어 위양성 위험.)
			if(keyword_last){
				Push_anchor_indent_check(
					lines, mask, r, "corner bracket: continuation underindented", out
				);
			}

			c = e;
		}
	}
}

// §5.5 후행반환 `->` 낫괄호.
// `->` 직전 비공백이 `)` 이면 후행반환 자리로 본다. 같은 행에 `{` 또는 `;` 가 (괄호 깊이 0)
// 없으면 다중행 낫괄호 발현 — 다음 코드 행 들여쓰기 ≥ `->` 행 +1.
void sak::Check_anchor_trailing_return(
	Lines const &lines, Lines const &mask, std::vector<Violation> &out
){
	int const rows = static_cast<int>(lines.size());

	for(int r = 0; r < rows; ++r){
		std::string const &m = mask[r];
		int const n = static_cast<int>(m.size());

		for(int c = 0; c + 1 < n; ++c){
			if(m[c] != '-' || m[c + 1] != '>'){
				continue;
			}

			// `->` 직전(행을 거슬러서)의 의미 토큰이 `)` 여야 후행반환 자리다.
			if(
				int pr = r, pc = c - 1;
				!Prev_significant(mask, pr, pc) || mask[pr][pc] != ')'
			){
				continue;
			}

			int depth = 0, cc = c + 2;
			bool found = false;

			while(cc < n){
				char const ch = m[cc];

				if(ch == '{' && depth == 0){
					found = true;

					break;
				}

				if(ch == '(' || ch == '[' || ch == '{'){
					++depth;
				}
				else if(ch == ')' || ch == ']' || ch == '}'){
					--depth;
				}
				else if(ch == ';' && depth == 0){
					found = true;

					break;
				}

				++cc;
			}

			if(found){
				continue;
			}

			Push_anchor_indent_check(
				lines, mask, r, "corner bracket: continuation underindented", out
			);
		}
	}
}

// `}` 의 짝 `{` 직전(혹은 그 직전 식별자 직전)이 struct/class/union/enum 인지 확인.
auto sak::Is_inline_type_close(Lines const &mask, Bk_pair const &p)->bool{
	if(p.kind != '{'){
		return false;
	}

	std::string const &m_o = mask[p.o_row];
	int q = p.o_col - 1;

	while( q >= 0 && (m_o[q] == ' ' || m_o[q] == '\t' || m_o[q] == '@') ){
		--q;
	}

	if(q < 0){
		return false;
	}

	std::string const w = Word_ending_at(m_o, q);

	if(w.empty()){
		return false;
	}

	int const s = q - static_cast<int>(w.size());

	if(w == "struct" || w == "class" || w == "union" || w == "enum"){
		// `new struct S{ 1 }` 는 정의가 아니라 상술형 타입 지정 + 중괄호 초기화다 — 제외.
		return Word_before(mask, p.o_row, s + 1) != "new";
	}

	int q2 = s;

	while( q2 >= 0 && (m_o[q2] == ' ' || m_o[q2] == '\t' || m_o[q2] == '@') ){
		--q2;
	}

	if(q2 < 0){
		return false;
	}

	std::string const w2 = Word_ending_at(m_o, q2);

	if(w2 == "struct" || w2 == "class" || w2 == "union" || w2 == "enum"){
		int const s2 = q2 - static_cast<int>(w2.size());

		return Word_before(mask, p.o_row, s2 + 1) != "new";
	}

	return false;
}

// §5.5 인라인 타입 정의 낫괄호 — `struct{…}『var…』;` 패턴.
// 매처 결과의 `{ }` 짝 중 인라인 타입 정의의 close 인 것을 골라, close `}` 가 행 마지막이고
// 다음 코드 행이 식별자로 시작하면 다중행 낫괄호 발현 — 다음 코드 행 들여쓰기 ≥ `}` 행 +1.
// 이 자리는 sak 이 마커 없이도 구조를 확정하므로, v2.10 의무 2칸 마커의 누락·오개수(≠2)까지
// 위양성 0 으로 잡아 자동삽입한다(일반 변수선언 자리와 달리).
void sak::Check_anchor_inline_type(
	Lines const &lines, Lines const &mask,
	std::vector<Bk_pair> const &pairs, std::vector<Violation> &out
){
	int const rows = static_cast<int>(lines.size());

	for(Bk_pair const &p : pairs){
		if( !Is_inline_type_close(mask, p) ){
			continue;
		}

		std::string const &m_c = mask[p.c_row];
		int const end = p.c_col + p.c_len, c_n = static_cast<int>(m_c.size());
		bool last_token = true;

		for(int cc = end; cc < c_n; ++cc){
			if( Is_code_char(m_c[cc]) ){
				last_token = false;

				break;
			}
		}

		if(!last_token){
			continue;
		}

		int const nr = Next_code_row(lines, mask, p.c_row);

		if(nr >= rows){
			continue;
		}

		std::string const &m_n = mask[nr];
		int first = 0;
		int const n_n = static_cast<int>(m_n.size());

		while( first < n_n && (m_n[first] == ' ' || m_n[first] == '\t' || m_n[first] == '@') ){
			++first;
		}

		if(first >= n_n){
			continue;
		}

		// 선언자 머리 — 식별자뿐 아니라 `*`(포인터)·`&`(참조)·`(`(매몰 선언자, §5.5)도 온다.
		char const head = m_n[first];

		bool const  
			decl_head
			= is_word_char(head) || head == '*' || head == '&' || head == '('
		;

		if(!decl_head){
			continue;
		}

		// v2.10 §5.5: 인라인 타입 정의의 다중행 변수선언 발현이 확정된 자리다. 잉여공백 2칸
		// 마커가 의무이므로, `}` 가 행의 진짜 마지막 글자(주석·문자열 꼬리 없음)일 때 꼬리 잉여가
		// 정확히 2칸인지 강제하고, 다른 개수(0·1·3·탭 혼입)는 gap_right 로 2칸 정규화한다.
		bool pure_ws_tail = true;

		for(int cc = end; cc < c_n; ++cc){
			if(m_c[cc] == '@'){
				pure_ws_tail = false;

				break;
			}
		}

		if( pure_ws_tail && lines[p.c_row].substr(end) != "  " ){
			Push_fix(
				out, { p.c_row, end, "5.5", "inline type: var-decl marker must be two spaces" },
				Fix_kind::gap_right, end, 2
			);
		}

		Push_anchor_indent_check(
			lines, mask, p.c_row, "inline type: var-list underindented", out
		);
	}
}

// §5.5 case 라벨 낫괄호.
// `case` 단어 같은 행에 (괄호 깊이 0, `::` 제외) `:` 없으면 다중행 발현 —
// 다음 코드 행 들여쓰기 ≥ `case` 행 +1.
void sak::Check_anchor_case(
	Lines const &lines, Lines const &mask, std::vector<Violation> &out
){
	int const rows = static_cast<int>(lines.size());

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

			if(w != "case"){
				c = e;

				continue;
			}

			int depth = 0, cc = e;
			bool found = false;

			while(cc < n){
				char const ch = m[cc];

				if(ch == '(' || ch == '[' || ch == '{'){
					++depth;
				}
				else if(ch == ')' || ch == ']' || ch == '}'){
					--depth;
				}
				else if(ch == ':' && depth == 0){
					bool const  
						is_scope
						= (cc + 1 < n && m[cc + 1] == ':') || (cc > 0 && m[cc - 1] == ':')
					;

					if(!is_scope){
						found = true;

						break;
					}
				}

				++cc;
			}

			if(!found){
				Push_anchor_indent_check(
					lines, mask, r, "corner bracket: continuation underindented", out
				);
			}

			c = e;
		}
	}
}

// §5.5 콜론 낫괄호 — 상속·enum 기반 타입·생성자 init list 세 자리 공통 레이아웃 검사.
// 앵커 ':' 위치를 받아, 짝지어질 body '{' 를 전방 스캔으로 찾고, 다중행이면 세 조건 검사:
//   (a) ':' 이 여는 행 마지막 코드 토큰인지
//   (b) '{' 이 닫는 행 첫 코드 토큰인지
//   (c) 사이 첫 코드 행 들여쓰기 ≥ ind(':') + 1
// ctor init 자리에선 'mem_{val}' 형태의 braced-init 를 body '{' 로 오인하지 않도록,
// '{' 직전 코드 문자가 식별자면 braced-init 로 간주해 짝 '}' 까지 skip.
void sak::Check_colon_cbracket_layout(
	Lines const &lines, Lines const &mask,
	int const a_row, int const a_col, Colon_vb_kind const kind,
	std::vector<Violation> &out
){
	int const rows = static_cast<int>(lines.size());

	int p_depth = 0, sq_depth = 0, ang_depth = 0, c_depth = 0;
	int close_row = -1, close_col = -1;
	bool stopped = false;

	for(int rr = a_row; rr < rows && close_row < 0 && !stopped; ++rr){
		std::string const &cm = mask[rr];
		int const cn = static_cast<int>(cm.size());
		int const start = rr == a_row ? a_col + 1 : 0;

		for(int cc = start; cc < cn; ++cc){
			char const ch = cm[cc];

			if(ch == '('){
				++p_depth;
			}
			else if(ch == ')'){
				--p_depth;
			}
			else if(ch == '['){
				++sq_depth;
			}
			else if(ch == ']'){
				--sq_depth;
			}
			else if(ch == '<'){
				++ang_depth;
			}
			else if(ch == '>' && ang_depth > 0){
				--ang_depth;
			}
			else if(ch == '{'){
				bool const  
					inside_expr
					= p_depth > 0 || sq_depth > 0 || ang_depth > 0
				;

				if(inside_expr){
					// 표현식 내부 braced-init·요소. 우리 대상 아님.
					continue;
				}

				if(c_depth > 0){
					++c_depth;

					continue;
				}

				bool is_body = true;

				if(kind == Colon_vb_kind::ctor_init){
					int rprev = rr, cprev = cc - 1;

					bool const  
						braced_init
						= Prev_significant(mask, rprev, cprev)
						&& is_word_char(mask[rprev][cprev])
					;

					if(braced_init){
						is_body = false;
					}
				}

				if(is_body){
					close_row = rr;
					close_col = cc;

					break;
				}

				++c_depth;
			}
			else if(ch == '}'){
				if(p_depth > 0 || sq_depth > 0 || ang_depth > 0){
					continue;
				}

				if(c_depth > 0){
					--c_depth;
				}
			}
			else if(
				ch == ';'
				&& p_depth == 0 && sq_depth == 0 && ang_depth == 0 && c_depth == 0
			){
				stopped = true;

				break;
			}
		}
	}

	if(close_row < 0){
		return;
	}

	if(close_row == a_row){
		return;
	}

	// (a) ':' 이 여는 행 마지막 코드 토큰.
	bool colon_last = true;
	std::string const &am = mask[a_row];
	int const an = static_cast<int>(am.size());

	for(int cc = a_col + 1; cc < an; ++cc){
		if( Is_code_char(am[cc]) ){
			colon_last = false;

			out.push_back({ a_row, cc, "5.5", "colon cbracket: ':' not last" });

			break;
		}
	}

	// (b) '{' 이 닫는 행 첫 코드 토큰.
	std::string const &cm = mask[close_row];

	for(int cc = 0; cc < close_col; ++cc){
		if( Is_code_char(cm[cc]) ){
			out.push_back(
				{ close_row, cc, "5.5", "colon cbracket: '{' not first" }
			);

			break;
		}
	}

	if(!colon_last){
		return;
	}

	// (c) ':' 다음 첫 코드 행 들여쓰기 ≥ ind(':') + 1.
	int const cur = Indent_depth(lines[a_row]);
	int nr = a_row + 1;

	while(nr < rows && nr < close_row){
		if( !Is_blank_row(lines[nr]) && Has_code(mask[nr]) ){
			break;
		}

		++nr;
	}

	if(nr >= close_row || nr >= rows){
		return;
	}

	if( Indent_depth(lines[nr]) < cur + 1 ){
		Push_fix(
			out, { nr, 0, "5.5", "colon cbracket: content underindented" },
			Fix_kind::indent, 0, cur + 1
		);
	}
}

// §5.5 콜론 낫괄호 스캐너 (A) — 상속·enum 기반 타입.
// class/struct/union/enum 키워드 앵커에서 전방 스캔, (), <>, [] 깊이 추적으로
// depth-0 ':' 이 나오면 앵커 확정. 앞서 '{' 나 ';' 을 만나면 무시 (본체 시작 or 전방 선언).
void sak::Scan_type_decl_colon(
	Lines const &lines, Lines const &mask, std::vector<Violation> &out
){
	int const rows = static_cast<int>(lines.size());

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

			if(w != "class" && w != "struct" && w != "union" && w != "enum"){
				c = e;

				continue;
			}

			// 'enum class Name' / 'enum struct Name' — 뒤의 class/struct 는 skip (enum 이 앵커).
			if(w == "class" || w == "struct"){
				int pc = c - 1;

				while( pc >= 0 && (m[pc] == ' ' || m[pc] == '\t' || m[pc] == '@') ){
					--pc;
				}

				if( pc >= 0 && Word_ending_at(m, pc) == "enum" ){
					c = e;

					continue;
				}
			}

			int p_depth = 0, sq_depth = 0, ang_depth = 0;
			int colon_row = -1, colon_col = -1;
			bool stopped = false;

			for(int rr = r; rr < rows && colon_row < 0 && !stopped; ++rr){
				std::string const &cm = mask[rr];
				int const cn = static_cast<int>(cm.size());
				int const start = rr == r ? e : 0;

				for(int cc = start; cc < cn; ++cc){
					char const ch = cm[cc];

					if(ch == '('){
						++p_depth;
					}
					else if(ch == ')'){
						--p_depth;
					}
					else if(ch == '['){
						++sq_depth;
					}
					else if(ch == ']'){
						--sq_depth;
					}
					else if(ch == '<'){
						++ang_depth;
					}
					else if(ch == '>' && ang_depth > 0){
						--ang_depth;
					}
					else if(p_depth == 0 && sq_depth == 0 && ang_depth == 0){
						if(ch == ';' || ch == '{'){
							stopped = true;

							break;
						}
						else if(ch == ':'){
							bool const  
								is_scope
								= (cc + 1 < cn && cm[cc + 1] == ':')
								|| (cc > 0 && cm[cc - 1] == ':')
							;

							if(!is_scope){
								colon_row = rr;
								colon_col = cc;

								break;
							}
						}
					}
				}
			}

			if(colon_row >= 0){
				Check_colon_cbracket_layout(
					lines, mask, colon_row, colon_col,
					Colon_vb_kind::inherit_or_enum, out
				);
			}

			c = e;
		}
	}
}

// §5.5 콜론 낫괄호 스캐너 (B) — 생성자 멤버초기화 리스트.
// ':' 좌측 근접 코드 문자가 ')' 이고, 문장 시작부터 여기까지 '?' 스택이 balanced 면 ctor init.
// 문장 시작 = 역방향으로 depth-0 의 ';' / '{' / '}' 를 만난 지점 (또는 파일 시작).
void sak::Scan_ctor_init_colon(
	Lines const &lines, Lines const &mask, std::vector<Violation> &out
){
	int const rows = static_cast<int>(lines.size());

	for(int r = 0; r < rows; ++r){
		std::string const &m = mask[r];
		int const n = static_cast<int>(m.size());

		for(int c = 0; c < n; ++c){
			if(m[c] != ':'){
				continue;
			}

			bool const  
				is_scope
				= (c + 1 < n && m[c + 1] == ':') || (c > 0 && m[c - 1] == ':')
			;

			if(is_scope){
				continue;
			}

			// 좌측 근접 코드 문자(행을 거슬러서)가 ')' 여야 ctor init 자리다.
			if(
				int pr = r, pc = c - 1;
				!Prev_significant(mask, pr, pc) || mask[pr][pc] != ')'
			){
				continue;
			}

			// 문장 시작 찾기 (역방향, depth-0 ';' / '{' / '}' 또는 파일 시작).
			int start_row = 0, start_col = 0;

			{
				int lr = r, lc = c - 1;
				int p_d = 0, s_d = 0;
				bool found = false;

				while(lr >= 0){
					if(lc < 0){
						--lr;

						if(lr < 0){
							break;
						}

						lc = static_cast<int>(mask[lr].size()) - 1;

						continue;
					}

					char const pc = mask[lr][lc];

					if(pc == ')'){
						++p_d;
					}
					else if(pc == '('){
						--p_d;
					}
					else if(pc == ']'){
						++s_d;
					}
					else if(pc == '['){
						--s_d;
					}
					else if(p_d == 0 && s_d == 0){
						if(pc == ';' || pc == '{' || pc == '}'){
							start_row = lr;
							start_col = lc + 1;
							found = true;

							break;
						}
					}

					--lc;
				}

				if(!found){
					start_row = 0;
					start_col = 0;
				}
			}

			// 문장 시작부터 ':' 까지 전방 스캔해 '?' 카운트.
			int q_count = 0;

			{
				int p_d = 0, s_d = 0;

				for(int rr = start_row; rr <= r; ++rr){
					std::string const &sm = mask[rr];
					int const sn = static_cast<int>(sm.size());
					int const s = rr == start_row ? start_col : 0;
					int const eend = rr == r ? c : sn;

					for(int cc = s; cc < eend; ++cc){
						char const sc = sm[cc];

						if(sc == '('){
							++p_d;
						}
						else if(sc == ')'){
							--p_d;
						}
						else if(sc == '['){
							++s_d;
						}
						else if(sc == ']'){
							--s_d;
						}
						else if(p_d == 0 && s_d == 0){
							if(sc == '?'){
								++q_count;
							}
							else if(sc == ':'){
								bool const  
									sub_scope
									= (cc + 1 < sn && sm[cc + 1] == ':')
									|| (cc > 0 && sm[cc - 1] == ':')
								;

								if(!sub_scope && q_count > 0){
									--q_count;
								}
							}
						}
					}
				}
			}

			if(q_count > 0){
				continue;
			}

			Check_colon_cbracket_layout(
				lines, mask, r, c, Colon_vb_kind::ctor_init, out
			);
		}
	}
}

// §5.5 콜론 낫괄호 — 두 스캐너 병치 진입점.
void sak::Check_anchor_colon_cbracket(
	Lines const &lines, Lines const &mask, std::vector<Violation> &out
){
	Scan_type_decl_colon(lines, mask, out);
	Scan_ctor_init_colon(lines, mask, out);
}

// §5.5 변수 선언문 낫괄호 — 의무화된 2칸 마커로 반자동 감지 (v2.10: 마커는 권장이 아니라
// 의무). 마지막 top-notorious(vexing parse)라 파서 없이는 잡기 어려운 자리를, 사용자가 놓은
// "타입 끝·개행 직전 잉여공백 2칸"(§5.5)을 신뢰하고 닫는 `;` 로 재검증해 잡는다. 마커 자체의
// 누락은 (인라인 타입 자리를 뺀) 일반 변수선언에선 sak 이 구조를 확정 못 해 감지 불가 —
// 서브에이전트가 채운다. sak 은 마커가 있을 때 그 레이아웃만 검증한다.
// 판정: 어떤 행이 (마스크 기준) 정확히 공백 2칸으로 끝나고, 그 앞 마지막 코드 문자가 타입
// 표현의 꼬리로 볼 수 있으면(‹`; { } ) ] , ( [`› 아님) 여는 낫괄호 후보. 후보부터 통합
// 깊이 추적으로 depth-0 `;`(중첩 `()[]{}`·람다 본문 skip)을 찾으면 변수선언 낫괄호로 확정.
// 확정 시 §5.5 레이아웃을 검사한다: (a) 타입이 마커 행 마지막 — 마커가 보장하므로 생략,
// (b) `;` 이 닫는 행 첫 코드 토큰, (c) 이음줄 들여쓰기 ≥ 마커 행 +1. 미확정이면 조용히 넘긴다
// (무해한 거짓음성 → 서브에이전트 폴백). 위양성 0 계약은 사용자 표식을 신뢰하는 형태로 지킨다.
// 변수 선언문 2칸 마커 행인가 — 마커 후보(정확히 2칸 꼬리 + 타입 꼬리 문자)에서 통합 깊이
// 추적으로 짝 `;` 를 찾아, 다른 행 첫 코드 토큰이면 그 닫는 행을 돌려준다. 아니면 -1.
auto sak::Marker_decl_close_row(Lines const &mask, int const r)->int{
	std::string const &m = mask[r];
	int const n = static_cast<int>(m.size());

	if( Tail_spaces(m) != 2 ){
		return -1;
	}

	int const t = n - 2;

	int const p = t - 1;

	if( p < 0 || !Is_code_char(m[p]) ){
		return -1;
	}

	if(
		char const last = m[p];
		last == ';' || last == '{' || last == '}' || last == ')' || last == ']'
		|| last == ',' || last == '(' || last == '['
	){
		return -1;
	}

	int close_row = -1, close_col = -1;

	if( !Find_stmt_semi(mask, r, t, close_row, close_col) ){
		return -1;
	}

	bool const  
		vclose = close_row > r && First_significant_col(mask[close_row]) == close_col
	;

	return vclose ? close_row : -1;
}

// §9.4 — 다중행 낫괄호(§5.5)도 다중행 괄호와 같은 공행 봉투를 요구한다(§5 서두 — 가상
// 괄호도 괄호류). 렉서가 원문에서 확정할 수 있는 자리만 본다(위양성 0 원칙).
//   · 여는 행(위쪽) — return/throw/using 이 행 끝에 홀로 선 행(『 가 행 끝에서 열린다),
//     그리고 변수 선언문의 2칸 마커 행. 위 인접 행이 같은 깊이의 문장 종결(`;`·`}`)이면
//     그 사이엔 공행이 필수다.
//   · 닫는 행(아래쪽) — 전개된 낫괄호의 종결 `;` 홀로 행(직전 코드 행이 한 단계 깊은
//     전개 내용일 때만 — 홀로 선 빈 문장 `;` 과 가른다). 아래 인접 행이 같은 깊이의 코드면
//     그 사이엔 공행이 필수다.
// 다중행 소괄호·첨자·속성 안은 문장이 아니라 이음줄이라 제외한다(`for` 머리의 `;` 포함).
void sak::Check_cbracket_blank_line(
	Lines const &lines, Lines const &mask, Lines const &cut_mask,
	std::vector<Bk_pair> const &pairs, std::vector<Violation> &out
){
	int const rows = static_cast<int>(lines.size());
	std::vector<char> inner(rows, 0);

	for(Bk_pair const &p : pairs){
		if(p.kind == '{'){
			continue;
		}

		for(int r = p.o_row + 1; r < p.c_row && r < rows; ++r){
			inner[r] = 1;
		}
	}

	for(int r = 0; r < rows; ++r){
		if(inner[r] != 0){
			continue;
		}

		int const fc = First_significant_col(mask[r]);

		if(fc < 0){
			continue;
		}

		int const lc = Last_significant_col(mask[r]);

		// 여는 행 — return/throw/using 홀로, 또는 2칸 마커 행.
		bool open_row = false;

		if( is_word_char(mask[r][fc]) ){
			std::string const w = Word_at(mask[r], fc);

			bool const  
				kw_alone
				= (w == "return" || w == "throw" || w == "using")
				&& lc == fc + static_cast<int>(w.size()) - 1
			;

			open_row = kw_alone;
		}

		if( !open_row && Marker_decl_close_row(cut_mask, r) >= 0 ){
			open_row = true;
		}

		if(open_row && r > 0){
			int const pr = r - 1;

			bool const  
				collides
				= !Is_blank_row(lines[pr]) && Has_code(mask[pr])
				&& Indent_depth(lines[pr]) == Indent_depth(lines[r])
				&& (
					Last_code_char(mask[pr]) == ';'
					|| Last_code_char(mask[pr]) == '}'
				)
			;

			if(collides){
				out.push_back(
					{ r, 0, "9.4", "missing blank line above multi-line corner bracket" }
				);
			}
		}

		// 닫는 행 — 종결 `;` 홀로 행, 직전 코드 행이 한 단계 깊은 전개 내용.
		bool const semi_alone = mask[r][fc] == ';' && lc == fc;

		if(semi_alone && r > 0 && r + 1 < rows){
			bool const  
				vclose
				= Has_code(mask[r - 1])
				&& Indent_depth(lines[r - 1]) == Indent_depth(lines[r]) + 1
			;

			int const nr = r + 1;

			bool const  
				collides
				= vclose
				&& !Is_blank_row(lines[nr]) && Has_code(mask[nr])
				&& Indent_depth(lines[nr]) == Indent_depth(lines[r])
				&& !Continues_statement(mask[nr], true)
			;

			if(collides){
				out.push_back(
					{ r, 0, "9.4", "missing blank line below multi-line corner bracket" }
				);
			}
		}
	}
}

void sak::Check_anchor_var_decl_marker(
	Lines const &lines, Lines const &mask, std::vector<Violation> &out
){
	int const rows = static_cast<int>(lines.size());

	for(int r = 0; r < rows; ++r){
		std::string const &m = mask[r];
		int const n = static_cast<int>(m.size());

		// 꼬리 공백이 정확히 2칸(§5.5 마커)이어야 후보다. 탭은 §8.2 소관이라 세지 않는다.
		if( Tail_spaces(m) != 2 ){
			continue;
		}

		int const t = n - 2;

		// 2칸 바로 앞은 코드 문자여야 하고, 타입 표현의 꼬리로 볼 수 있어야 한다.
		int const p = t - 1;

		if( p < 0 || !Is_code_char(m[p]) ){
			continue;
		}

		if(
			char const last = m[p];
			last == ';' || last == '{' || last == '}' || last == ')' || last == ']'
			|| last == ',' || last == '(' || last == '['
		){
			continue;
		}

		// 후보 지점부터 통합 깊이 추적으로 짝 `;` 를 찾는다(중첩 `()[]{}`·람다 본문 skip).
		int close_row = -1, close_col = -1;
		Find_stmt_semi(mask, r, t, close_row, close_col);

		// 짝 `;` 를 못 찾았거나 같은 행이면 변수선언 낫괄호로 확정하지 않는다.
		if(close_row <= r){
			continue;
		}

		// (b) `;` 이 닫는 행 첫 코드 토큰인지.
		std::string const &cm = mask[close_row];

		for(int cc = 0; cc < close_col; ++cc){
			if( Is_code_char(cm[cc]) ){
				out.push_back({ close_row, cc, "5.5", "var-decl marker: ';' not first" });

				break;
			}
		}

		// (c) 다음 코드 행 들여쓰기 ≥ 마커 행 +1.
		Push_anchor_indent_check(
			lines, mask, r, "var-decl marker: continuation underindented", out
		);
	}
}
