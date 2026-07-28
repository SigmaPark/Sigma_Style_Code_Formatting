/*  SPDX-FileCopyrightText: (c) 2020 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "check_internal.hpp"

#include <cctype>
#include <string>
#include <vector>

namespace sak{
	struct Shield;

	static auto Is_no_space_bidir(std::string const &t)->bool;
	static auto Var_decl_close_semi(std::string const &m, int const from)->int;

	static auto Build_shields(
		Lines const &mask, std::vector<Adj_tok> const &toks,
		std::vector<Bk_pair> const &pairs, std::vector<Angle_pair> const &angles
	)->std::vector<Shield>;

	static auto Brace_opens_statement_scope(Lines const &mask, Bk_pair const &p)->bool;
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

// 차폐 구간 — 단일행 괄호(가상 괄호 포함)의 안쪽. 그 안의 토큰은 하나의 피연산 토큰에 속하므로
// 개행 우선순위를 갖지 않는다(§6.2).
struct sak::Shield{
	int row, lo, hi;
};
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

// 좌우 모두 붙여 쓰는 양방향 토큰(개행 우선순위가 산술 이항연산자보다 낮은 것들, §8.4).
auto sak::Is_no_space_bidir(std::string const &t)->bool{
	return t == "::" || t == "." || t == "->" || t == ".*" || t == "->*";
}

// §8.4 — 표기 판정으로 분류된 문맥 의존 토큰의 공백.
//
// 표기가 곧 분류이므로(표기 판정), 어느 분류의 합법 표기와도 맞지 않는 자리(unresolved)는 그 자체로
// 위반이다. 반면 분류가 표기가 아니라 **이웃의 정체**로 정해지는 토큰(`+` `-` `::` `:` `!` `~`
// `<<`)은 표기가 그 분류의 요구와 어긋날 수 있어 따로 검사한다.
//
// 자동교정 힌트는 달지 않는다 — 이 자리의 공백은 분류를 싣는 신호여서, 기계가 그 유무를 뒤집는
// 것은 서식 교정이 아니라 코드의 의미 선언을 바꾸는 일이다(신호 공백 불가침).
void sak::Check_adjudicated_space(
	std::vector<Adj_tok> const &toks, std::vector<Violation> &out
){
	for(Adj_tok const &t : toks){
		if(t.lex != Tk_cls::skip || t.cls == Adj_cls::word || t.cls == Adj_cls::lit){
			continue;
		}

		// 꺾쇠는 괄호다 — §5.4·§5.7·§8.4·§8.5 이 맡는다.
		if(t.cls == Adj_cls::angle_open || t.cls == Adj_cls::angle_close){
			continue;
		}

		if(t.cls == Adj_cls::unresolved){
			out.push_back(
				{
					t.row, t.col, "8.4",
					"'" + t.text + "': spacing matches no legal token class"
				}
			);

			continue;
		}

		if(t.cls == Adj_cls::bidir){
			bool const tight = Is_no_space_bidir(t.text);

			// §8.4 는 **단일행 상태**의 공백만 정한다. 무공백 양방향 토큰이 개행되면 §9.1 에
			// 따라 넣은 공백이 유지되므로, 이웃이 다른 행에 있으면 그 쪽은 보지 않는다.
			bool const  
				far_l = tight && t.el >= 0 && toks[t.el].row != t.row,
				far_r = tight && t.er >= 0 && toks[t.er].row != t.row
			;

			if( t.el >= 0 && !far_l && (tight ? t.gl > 0 : t.gl == 0) ){
				out.push_back(
					{
						t.row, t.col, "8.4",
						"'" + t.text + "': binary operator spacing on the left"
					}
				);
			}

			if( t.er >= 0 && !far_r && (tight ? t.gr > 0 : t.gr == 0) ){
				out.push_back(
					{
						t.row, t.col, "8.4",
						"'" + t.text + "': binary operator spacing on the right"
					}
				);
			}

			continue;
		}

		if(t.cls == Adj_cls::unidir && t.text != "..."){
			// 라벨 콜론은 왼쪽 피연산자에 붙고, 나머지 단방향은 오른쪽 피연산자에 붙는다.
			bool const label = t.text == ":";

			if(label){
				if(t.er >= 0 && t.gr == 0){
					out.push_back(
						{ t.row, t.col, "8.4", "':': label colon needs a space after it" }
					);
				}

				continue;
			}

			if(t.er >= 0 && t.gr > 0 && t.text != "++" && t.text != "--"){
				out.push_back(
					{
						t.row, t.col, "8.4",
						"'" + t.text + "': unary operator must attach to its operand"
					}
				);
			}

			// 반대쪽 — 앞 토큰이 단어(키워드 포함)인데 붙어 있으면 공백이 빠진 것이다.
			// 앞이 기호형이면 그 토큰 자신의 규칙이 그 공백을 이미 정한다(중복 보고 회피).
			if(t.el >= 0 && t.gl == 0 && toks[t.el].cls == Adj_cls::word){
				out.push_back(
					{
						t.row, t.col, "8.4",
						"'" + t.text + "': space required before this unary operator"
					}
				);
			}

			continue;
		}

		// 피연산 기호형 토큰 — 비기호형 토큰과 인접할 때 그 사이에 공백을 둔다(§8.4).
		if(t.cls == Adj_cls::operand_like){
			bool const  
				word_l
				= t.el >= 0
				&& (
					toks[t.el].cls == Adj_cls::word || toks[t.el].cls == Adj_cls::lit
				)
			;

			bool const  
				word_r
				= t.er >= 0
				&& (
					toks[t.er].cls == Adj_cls::word || toks[t.er].cls == Adj_cls::lit
				)
			;

			if(word_l && t.gl == 0){
				out.push_back(
					{
						t.row, t.col, "8.4",
						"'" + t.text + "': operand-like token needs a space on the left"
					}
				);
			}

			if(word_r && t.gr == 0){
				out.push_back(
					{
						t.row, t.col, "8.4",
						"'" + t.text + "': operand-like token needs a space on the right"
					}
				);
			}
		}
	}
}

// §9.1 — 개행 우선순위가 부여된 기호형 토큰이 다중행 상태로 갈 때의 형태.
//
// 단일행 상태를 기준으로 앞쪽 공백을 개행으로 치환하므로, 앞뒤에 공백을 두는 **양방향 토큰은
// 다음 행을 이끈다**(제 행 끝에 남을 수 없다). 앞쪽에 공백이 없는 **구분자 `,` 는 뒤쪽이
// 치환되어 제 행에 남는다**(다음 행을 이끌 수 없다). 앞뒤 모두 공백이 없는 양방향(`::` `.` `->`)
// 은 앞뒤에 공백을 넣은 뒤 같은 방식을 적용하므로, 개행 후에도 그 공백이 유지된다.
//
// 세미콜론과 괄호(꺾쇠 포함)의 자리는 §5.4·§5.5 가 정하므로 여기서 보지 않는다.
// 용의 위에 선 판정은 그 판정보다 확실할 수 없으므로 함께 용의로 내린다.
void sak::Check_break_form(std::vector<Adj_tok> const &toks, std::vector<Violation> &out){
	int const n = static_cast<int>(toks.size());

	for(int i = 0; i < n; ++i){
		Adj_tok const &t = toks[i];

		bool const  
			governed_elsewhere
			= t.cls == Adj_cls::semi || t.cls == Adj_cls::open_b
			|| t.cls == Adj_cls::close_b || t.cls == Adj_cls::angle_open
			|| t.cls == Adj_cls::angle_close
		;

		if(t.prio == Prio::none || governed_elsewhere){
			continue;
		}

		bool const  
			line_final = i + 1 < n && toks[i + 1].row != t.row,
			line_first = i > 0 && toks[i - 1].row != t.row
		;

		V_cat const cat = t.suspect ? V_cat::suspect : V_cat::violation;

		// `:`(상속·생성자 멤버초기화·enum 기반 타입)와 `->`(후행반환)는 §5.5 가상 괄호의 여는
		// 키워드다. 가상 괄호가 다중행이면 그 여는 쪽은 행의 마지막에 와야 하므로(§5.4), 행 끝에
		// 선 것이 곧 정본이다. 이 두 토큰의 자리는 §5.5 검사가 맡는다.
		bool const opens_vbracket = t.text == ":" || t.text == "->";

		if(t.cls == Adj_cls::bidir && line_final && !opens_vbracket){
			Violation  
				v{
					t.row, t.col, "9.1",
					"'" + t.text + "': binary operator must lead the next line, not trail this one"
				}
			;

			v.cat = cat;
			out.push_back(v);

			continue;
		}

		if(t.cls == Adj_cls::comma && line_first){
			out.push_back(
				{ t.row, t.col, "9.1", "',': separator must stay on the line it ends" }
			);

			continue;
		}

		if(
			t.cls == Adj_cls::bidir && line_first && Is_no_space_bidir(t.text)
			&& !opens_vbracket
			&& i + 1 < n && toks[i + 1].row == t.row && t.gr == 0
		){
			Violation  
				v{
					t.row, t.col, "9.1",
					"'" + t.text + "': the space inserted for the break must be kept after it"
				}
			;

			v.cat = cat;
			out.push_back(v);
		}
	}
}

// 다중행 타입의 닫는 행에서, 타입이 끝난 지점(from) 뒤가 단일행 변수 선언 가상 괄호(선언자
// 나열 + 같은 행의 종결 `;`)이면 그 `;` 의 열을 돌려준다. 아니면 -1. 선언자 머리는 식별자와
// `*`·`&`·`(` 만 인정하고, `;` 는 괄호 깊이 0 에서 찾는다.
auto sak::Var_decl_close_semi(std::string const &m, int const from)->int{
	int const n = static_cast<int>(m.size());
	int i = from, ws = 0;

	while( i < n && (m[i] == ' ' || m[i] == '\t') ){
		++i;
		++ws;
	}

	bool const  
		head
		= ws != 0 && i < n
		&& ( is_word_char(m[i]) || m[i] == '*' || m[i] == '&' || m[i] == '(' )
	;

	if(!head){
		return -1;
	}

	for(int depth = 0; i < n; ++i){
		char const c = m[i];

		if(c == '(' || c == '[' || c == '{'){
			++depth;
		}
		else if(c == ')' || c == ']' || c == '}'){
			if(depth == 0){
				return -1;
			}

			--depth;
		}
		else if(c == ';' && depth == 0){
			return i;
		}
	}

	return -1;
}

// §6.2 — 단일행 괄호(실괄호·가상 괄호)의 안은 통째로 하나의 피연산 토큰이라 개행 경쟁에
// 노출되지 않는다. 경쟁 판정이 보지 말아야 할 안쪽 구간들을 차폐면으로 모은다.
auto sak::Build_shields(
	Lines const &mask, std::vector<Adj_tok> const &toks,
	std::vector<Bk_pair> const &pairs, std::vector<Angle_pair> const &angles
)->std::vector<Shield>{
	int const n = static_cast<int>(toks.size());
	std::vector<Shield> shields;

	for(Bk_pair const &p : pairs){
		if(p.o_row == p.c_row){
			shields.push_back({ p.o_row, p.o_col, p.c_col });
		}
	}

	for(Angle_pair const &a : angles){
		if(a.o_row == a.c_row){
			shields.push_back({ a.o_row, a.o_col, a.c_col });
		}
	}

	// 변수 선언문의 단일행 가상 괄호 — 다중행 타입(인라인 타입 정의)이 끝난 `}` 뒤부터 같은
	// 행의 종결 `;` 까지가 하나의 피연산 토큰이다. 안의 구분자·초기화자는 경쟁에 노출되지
	// 않는다.
	for(Bk_pair const &p : pairs){
		if( p.o_row != p.c_row && Is_inline_type_close(mask, p) ){
			int const semi = Var_decl_close_semi(mask[p.c_row], p.c_col + p.c_len);

			if(semi >= 0){
				shields.push_back({ p.c_row, p.c_col, semi });
			}
		}
	}

	// 다중행 꺾쇠 타입(`>::사슬`)의 변수 선언문도 동일하다 — 사슬이 끝난 지점 뒤부터 종결
	// `;` 까지가 단일행 가상 괄호다.
	for(Angle_pair const &a : angles){
		if(a.o_row == a.c_row){
			continue;
		}

		int const chain = Angle_chain_end(mask[a.c_row], a.c_col + 1);

		if(chain < 0){
			continue;
		}

		int const semi = Var_decl_close_semi(mask[a.c_row], chain);

		if(semi >= 0){
			shields.push_back({ a.c_row, a.c_col, semi });
		}
	}

	// 단일행 콜론 가상 괄호 — `:` 다음부터 그 행을 닫는 `{` 까지가 하나의 피연산 토큰이다.
	for(int i = 0; i < n; ++i){
		if(toks[i].cls != Adj_cls::bidir || toks[i].text != ":"){
			continue;
		}

		int last = i;

		while(last + 1 < n && toks[last + 1].row == toks[i].row){
			++last;
		}

		if(last > i && toks[last].cls == Adj_cls::open_b && toks[last].text == "{"){
			shields.push_back({ toks[i].row, toks[i].col, toks[last].col });
		}
	}

	return shields;
}

// §9.2 — 개행 경쟁 범위.
//
// 다중행 상태의 기호형 토큰·괄호는, 자기 경쟁 범위 안에 **자기보다 개행 우선순위가 높은 단일행
// 상태의** 토큰·괄호를 가져선 안 된다. 경쟁 범위는 기호형 토큰이면 인접한 개행이 있는 행과 그
// 다음 행, 다중행 괄호면 여는 행과 닫는 행, 세미콜론이면 자기 행뿐이다.
//
// 단일행 괄호 안은 통째로 하나의 피연산 토큰이라(§6.2) 경쟁에 노출되지 않는다 — 생성자
// 멤버초기화 리스트·상속 리스트·변수 선언문처럼 단일행 가상 괄호를 이루는 자리도
// 마찬가지다(§5.5).
void sak::Check_break_competition(
	Lines const &mask, std::vector<Adj_tok> const &toks,
	std::vector<Bk_pair> const &pairs, std::vector<Angle_pair> const &angles,
	std::vector<Violation> &out
){
	int const n = static_cast<int>(toks.size());

	if(n == 0){
		return;
	}

	std::vector<Shield> const shields = Build_shields(mask, toks, pairs, angles);

	auto const  
		shielded
		= [&shields](Adj_tok const &t)->bool{
			for(Shield const &s : shields){
				if(t.row == s.row && t.col > s.lo && t.col < s.hi){
					return true;
				}
			}

			return false;
		}
	;

	// 다중행 괄호의 여닫는 토큰만 괄호 우선순위를 갖는다(§6.2 — 단일행 괄호는 갖지 않는다).
	auto const  
		ml_bracket_tok
		= [&pairs, &angles](Adj_tok const &t)->bool{
			for(Bk_pair const &p : pairs){
				bool const  
					hit
					= (t.row == p.o_row && t.col == p.o_col)
					|| (t.row == p.c_row && t.col == p.c_col)
				;

				if(hit && p.o_row != p.c_row){
					return true;
				}
			}

			for(Angle_pair const &a : angles){
				bool const  
					hit
					= (t.row == a.o_row && t.col == a.o_col)
					|| (t.row == a.c_row && t.col == a.c_col)
				;

				if(hit && a.o_row != a.c_row){
					return true;
				}
			}

			return false;
		}
	;

	auto const  
		is_bracket
		= [](Adj_tok const &t)->bool{
			return
				t.cls == Adj_cls::open_b || t.cls == Adj_cls::close_b
				|| t.cls == Adj_cls::angle_open || t.cls == Adj_cls::angle_close
			;
		}
	;

	// 그 토큰의 유효 우선순위 — 단일행 괄호는 없는 것으로 본다.
	auto const  
		eff_prio
		= [&is_bracket, &ml_bracket_tok](Adj_tok const &t)->Prio{
			if( is_bracket(t) ){
				return ml_bracket_tok(t) ? Prio::bracket : Prio::none;
			}

			return t.prio;
		}
	;

	// 다중행 상태 — 괄호는 그 짝이 여러 행에 걸쳤는지로, 나머지는 인접한 개행이 있는지로 본다.
	// 행의 마지막 토큰 뒤에는 개행이 있다 — 파일의 마지막 토큰도 마찬가지다.
	auto const  
		multiline
		= [&toks, n, &is_bracket, &ml_bracket_tok](int const i)->bool{
			if( is_bracket(toks[i]) ){
				return ml_bracket_tok(toks[i]);
			}

			bool const  
				lead = i > 0 ? toks[i - 1].row != toks[i].row : toks[i].row > 0,
				trail = i + 1 < n ? toks[i + 1].row != toks[i].row : true
			;

			return lead || trail;
		}
	;

	auto const  
		report
		= [&toks, n, &out, &shielded, &eff_prio, &multiline](
			int const owner, int const lo_row, int const hi_row
		)->void{
			Prio const own = eff_prio(toks[owner]);

			for(int j = 0; j < n; ++j){
				if(j == owner || toks[j].row < lo_row || toks[j].row > hi_row){
					continue;
				}

				Prio const p = eff_prio(toks[j]);

				bool const  
					competes
					= p != Prio::none && p < own && !multiline(j) && !shielded(toks[j])
				;

				if(!competes){
					continue;
				}

				Violation  
					v{
						toks[j].row, toks[j].col, "9.2",
						"'" + toks[j].text
						+ "': single-line token outranks the multi-line '"
						+ toks[owner].text + "' sharing its break-competition range"
					}
				;

				if(toks[j].suspect || toks[owner].suspect){
					v.cat = V_cat::suspect;
				}

				out.push_back(v);
			}
		}
	;

	for(int i = 0; i < n; ++i){
		if( eff_prio(toks[i]) == Prio::none || !multiline(i) ){
			continue;
		}

		// 세미콜론은 예외적으로 자기 행만을 경쟁 범위로 갖는다.
		if(toks[i].cls == Adj_cls::semi){
			report(i, toks[i].row, toks[i].row);

			continue;
		}

		if( is_bracket(toks[i]) ){
			continue;   // 괄호는 짝을 아는 아래 루프에서 본다
		}

		if(i > 0 && toks[i - 1].row != toks[i].row){
			report(i, toks[i].row - 1, toks[i].row);
		}

		if(i + 1 < n && toks[i + 1].row != toks[i].row){
			report(i, toks[i].row, toks[i].row + 1);
		}
	}

	// 다중행 괄호 — 경쟁 범위는 여는 괄호가 있는 행과 닫는 괄호가 있는 행이다.
	auto const  
		tok_at
		= [&toks, n](int const row, int const col)->int{
			for(int i = 0; i < n; ++i){
				if(toks[i].row == row && toks[i].col == col){
					return i;
				}
			}

			return -1;
		}
	;

	for(Bk_pair const &p : pairs){
		int const i = p.o_row != p.c_row ? tok_at(p.o_row, p.o_col) : -1;

		if(i >= 0){
			report(i, p.o_row, p.o_row);
			report(i, p.c_row, p.c_row);
		}
	}

	for(Angle_pair const &a : angles){
		int const i = a.o_row != a.c_row ? tok_at(a.o_row, a.o_col) : -1;

		if(i >= 0){
			report(i, a.o_row, a.o_row);
			report(i, a.c_row, a.c_row);
		}
	}
}

// §4.3 닫는 ')' 뒤의 개행이 단어로 이어지는 형태 — 그 개행의 발생원은 가상연산자 '▽' 뿐이다.
// ')' 가 다중행 괄호의 닫음이면 적법한 ▽ 개행이라 침묵하고(그 짝의 §9.4 는 별도 검사),
// 단일행 ')' 이면 ▽ 의 개행 경쟁(§9.2)으로 가른다 — ▽ 는 다중행 괄호보다 높고 이항 산술
// '/' 보다 낮으므로(§6.1 ◆), 두 행(')' 행·단어 행)에 그보다 높은 단일행 연산 토큰이 차폐면
// 밖에 서 있으면 그 개행은 성립할 수 없다.
//   · 경쟁자가 있으면 확정 위반 — 응집하거나 재배치해야 한다. (문장 매크로가 이 꼴을 겹쳐
//     쓰지만, 그 면제는 종전 분업대로 서브에이전트 몫이다.)
//   · 경쟁자가 없으면 적법한 ▽ 개행 — 침묵한다.
//   · 사이에 공행이 끼었거나 들여쓰기가 어긋난 형태(전개 변수선언 후보)는 표기만으로 못
//     가린다 — 종전대로 용의로 지목한다.
void sak::Check_word_paren_vop(
	Lines const &lines, Lines const &mask, std::vector<Adj_tok> const &toks,
	std::vector<Bk_pair> const &pairs, std::vector<Angle_pair> const &angles,
	std::vector<Violation> &out
){
	int const  
		rows = static_cast<int>(mask.size()),
		n = static_cast<int>(toks.size())
	;

	std::vector<Shield> const shields = Build_shields(mask, toks, pairs, angles);

	for(int r = 0; r < rows; ++r){
		int const l = Last_significant_col(mask[r]);

		if(l < 0 || mask[r][l] != ')'){
			continue;
		}

		bool crossed = false;
		int const nr = Next_code_row_over_blanks(lines, mask, r, crossed);

		if(nr < 0){
			continue;
		}

		int const nc = First_significant_col(mask[nr]);

		if( nc < 0 || !is_word_char(mask[nr][nc]) ){
			continue;
		}

		int pr = r, pc = l;

		if( Match_bracket_back(mask, '(', ')', pr, pc) && pr != r ){
			continue;
		}

		if( crossed || Indent_depth(lines[nr]) != Indent_depth(lines[r]) ){
			out.push_back(
				{
					nr, nc, "4.3",
					crossed
					? "newline after single-line ')' before a word — blank lines do not license it"
					: "newline after single-line ')' before a word: cohere it,"
					" or it is a statement macro",
					Fix_kind::none, 0, 0, V_cat::suspect
				}
			);

			continue;
		}

		// ▽ 경쟁 — 두 행에서 ▽ 보다 높은 단일행 연산 토큰을 찾는다. 괄호류는 단일행이면
		// 우선순위가 없고 다중행이면 ▽ 보다 낮으니(§6.2) 경쟁자가 아니다.
		int hit = -1;
		bool solid = false;

		for(int j = 0; j < n && !solid; ++j){
			Adj_tok const &t = toks[j];

			bool const  
				bracket_cls
				= t.cls == Adj_cls::open_b || t.cls == Adj_cls::close_b
				|| t.cls == Adj_cls::angle_open || t.cls == Adj_cls::angle_close
			;

			if( (t.row != r && t.row != nr) || bracket_cls || t.prio >= Prio::vop ){
				continue;
			}

			bool const  
				lead = j > 0 ? toks[j - 1].row != t.row : t.row > 0,
				trail = j + 1 < n ? toks[j + 1].row != t.row : true
			;

			if(lead || trail){
				continue;
			}

			bool inside = false;

			for(Shield const &s : shields){
				if(t.row == s.row && t.col > s.lo && t.col < s.hi){
					inside = true;

					break;
				}
			}

			if(inside){
				continue;
			}

			hit = j;
			solid = !t.suspect;
		}

		if(hit < 0){
			continue;
		}

		Violation  
			v{
				nr, nc, "4.3",
				"newline after single-line ')' before a word: single-line '" + toks[hit].text
				+ "' outranks the virtual operator (9.2) — cohere it, or it is a statement macro"
			}
		;

		if(!solid){
			v.cat = V_cat::suspect;
		}

		out.push_back(v);
	}
}

// §9.4 잔여 — 다중행 기호형 토큰이 형성한 개행 경쟁 범위에 인접한 위·아래 행은 공행이어야 하고,
// 그 범위 안에는 공행이 있을 수 없다. 다중행 괄호 쪽은 Check_bracket_blank_line 이 맡으므로,
// 여기서는 괄호와 세미콜론을 뺀 나머지 토큰(곧 다중행 이항 연산자)이 만든 범위만 본다.
void sak::Check_operator_blank_line(
	Lines const &lines, Lines const &mask, std::vector<Adj_tok> const &toks,
	std::vector<Bk_pair> const &pairs, std::vector<Violation> &out
){
	int const  
		n = static_cast<int>(toks.size()),
		rows = static_cast<int>(lines.size())
	;

	// 표현식 괄호(`(` `[` `[[`)의 안쪽 행들은 문장이 아니라 괄호의 이음줄이다 — 공행이 설 자리가
	// 아니며, 그 괄호의 공행은 Check_bracket_blank_line 이 이미 본다. 중괄호는 다르다: 그 안에는
	// 문장이 살기 때문에 §9.4 가 그대로 적용된다.
	std::vector<char> inner(rows, 0);

	for(Bk_pair const &p : pairs){
		if(p.kind == '{'){
			continue;
		}

		for(int r = p.o_row + 1; r < p.c_row && r < rows; ++r){
			inner[r] = 1;
		}
	}

	std::vector<char> in_range(rows, 0);

	for(int i = 0; i < n; ++i){
		Adj_tok const &t = toks[i];

		bool const  
			governed_elsewhere
			= t.prio == Prio::none || t.cls == Adj_cls::semi
			|| t.cls == Adj_cls::open_b || t.cls == Adj_cls::close_b
			|| t.cls == Adj_cls::angle_open || t.cls == Adj_cls::angle_close
		;

		if(governed_elsewhere){
			continue;
		}

		if(i > 0 && toks[i - 1].row != t.row){
			for(int r = toks[i - 1].row + 1; r < t.row; ++r){
				if( Is_blank_row(lines[r]) ){
					out.push_back(
						{ r, 0, "9.4", "blank line inside a break-competition range" }
					);
				}
			}

			in_range[ toks[i - 1].row ] = 1;
			in_range[t.row] = 1;
		}

		if(i + 1 < n && toks[i + 1].row != t.row){
			in_range[t.row] = 1;
			in_range[ toks[i + 1].row ] = 1;
		}
	}

	for(int r = 0; r < rows; ++r){
		if(in_range[r] == 0){
			continue;
		}

		int lo = r, hi = r;

		while(hi + 1 < rows && in_range[hi + 1] != 0){
			++hi;
		}

		r = hi;

		if(inner[lo] != 0 || inner[hi] != 0){
			continue;
		}

		if(lo > 0){
			int const nr = lo - 1;

			bool const  
				collides
				= !Is_blank_row(lines[nr]) && Has_code(mask[nr])
				&& Indent_depth(lines[nr]) == Indent_depth(lines[lo])
				&& (
					Last_code_char(mask[nr]) == ';'
					|| Last_code_char(mask[nr]) == '}'
				)
				&& !Continues_statement(mask[lo], false)
			;

			if(collides){
				out.push_back(
					{ lo, 0, "9.4", "missing blank line above multi-line operator" }
				);
			}
		}

		if(hi + 1 < rows){
			int const nr = hi + 1;

			bool const  
				collides
				= !Is_blank_row(lines[nr]) && Has_code(mask[nr])
				&& Indent_depth(lines[nr]) == Indent_depth(lines[hi])
				&& Last_code_char(mask[hi]) == ';'
				&& !Continues_statement(mask[nr], true)
			;

			if(collides){
				out.push_back(
					{ hi, 0, "9.4", "missing blank line below multi-line operator" }
				);
			}
		}
	}
}

// §3 단항 연산자 병기 — 첫 부호가 단항으로 판정되면 그 뒤에 같은 부호가 이어질 수 없다.
// 이중 부정은 `- -x` 가 아니라 `-(-x)` 다. 문맥 불변 자리는 Check_unary_juxtaposition 이 이미
// 보므로, 여기서는 판정이 있어야만 보이는 자리(앞이 키워드인 경우 등)를 마저 본다.
void sak::Check_unary_pair(std::vector<Adj_tok> const &toks, std::vector<Violation> &out){
	int const n = static_cast<int>(toks.size());

	for(int i = 0; i + 1 < n; ++i){
		Adj_tok const &t = toks[i];

		bool const  
			pair
			= t.cls == Adj_cls::unidir && (t.text == "-" || t.text == "+")
			&& toks[i + 1].text == t.text && toks[i + 1].row == t.row
		;

		if(pair){
			out.push_back(
				{ t.row, t.col, "3", "unary operators need parentheses, not space" }
			);
		}
	}
}

// §3 키워드 위치 — `const`·`volatile`·`constexpr` 은 언제나 수식할 대상의 **뒤**에 온다.
// 그러므로 이 키워드들의 왼쪽에는 반드시 수식받는 대상(타입 이름·`*`·닫는 괄호)이 있어야 한다.
// 왼쪽이 문장 경계이거나 `static`·`inline` 같은 앞쪽 한정자면, 그 키워드는 앞에 놓인 것이다.
void sak::Check_qualifier_prefix(
	std::vector<Adj_tok> const &toks, std::vector<Violation> &out
){
	static char const * const  
		Modifiers[]
		= {
			"static", "inline", "virtual", "explicit", "friend", "extern", "mutable",
			"thread_local", "register", "typedef", "constexpr", "consteval", "constinit"
		}
	;

	int const n = static_cast<int>(toks.size());

	for(int i = 0; i < n; ++i){
		Adj_tok const &t = toks[i];

		bool const  
			qualifier
			= t.cls == Adj_cls::word
			&& (t.text == "const" || t.text == "volatile" || t.text == "constexpr")
		;

		if(!qualifier){
			continue;
		}

		int const p = i - 1;

		bool prefixed = p < 0;

		if(p >= 0){
			Adj_cls const lc = toks[p].cls;

			prefixed
			= lc == Adj_cls::semi || lc == Adj_cls::comma
			|| (lc == Adj_cls::open_b && toks[p].text != "[");

			for(char const * const m : Modifiers){
				if(lc == Adj_cls::word && toks[p].text == m){
					prefixed = true;
				}
			}
		}

		if(prefixed){
			out.push_back(
				{
					t.row, t.col, "3",
					"'" + t.text + "' must follow what it qualifies, not precede it"
				}
			);
		}
	}
}

// §5.5 · §9.3 — 마커 없이 단어에서 단어로 넘어가는 개행.
//
// 행이 단어로 끝나고 다음 코드 행이 단어로 시작하는 자리는, 그 사이에 가상 괄호가 열릴 때만
// 적법하다. 그런데 가상 괄호가 열리는 자리는 모두 눈에 보인다 — `return`·`throw`·`using`·`case`
// 는 키워드고, 후행반환은 `->`, 인라인 타입은 `}`, 그리고 **변수 선언문은 2칸 마커**(§5.5,
// v2.2.0 부터 의무)다. 그러므로 키워드도 마커도 없는 단어↔단어 개행은 **어느 해석으로도
// 위반**이다 — 변수 선언이면 마커를 빠뜨린 것(§5.5)이고, 아니면 인접한 두 비기호형 토큰 사이에
// 개행을 둔 것(§9.3 — 발생원 목록 밖의 개행)이다. 어느 쪽이든 위반이므로 위양성 없이 확정한다.
// 두 행 사이의 순수 공행은 건너서 본다 — 공행은 발생원이 아니라 인가된 자리에 쌓인 형상(§9.4)
// 이므로, 공행을 끼워도 무허가 개행은 합법이 되지 않는다. 공행 판정은 raw 행으로 한다(컷마스크
// 에서는 주석 전용 행이 잘려 공백행처럼 보이기 때문).
void sak::Check_unmarked_wrap(
	Lines const &lines, Lines const &cut_lines, Lines const &cut_mask,
	std::vector<Violation> &out
){
	static char const * const  
		Openers[]
		= { "return", "throw", "using", "case", "co_return", "co_yield", "else", "do", "try" }
	;

	int const rows = static_cast<int>(cut_mask.size());

	for(int r = 0; r < rows; ++r){
		std::string const &a = cut_mask[r];

		if( !Has_code(a) ){
			continue;
		}

		bool crossed = false;
		int const nr = Next_code_row_over_blanks(lines, cut_mask, r, crossed);

		if(nr < 0){
			continue;
		}

		std::string const &b = cut_mask[nr];

		int const  
			a_end = Last_significant_col(a),
			b_beg = First_significant_col(b)
		;

		if(a_end < 0 || b_beg < 0){
			continue;
		}

		bool const  
			word_pair
			= is_word_char(a[a_end])
			&& is_word_char(b[b_beg]) && !std::isdigit( static_cast<unsigned char>(b[b_beg]) )
		;

		if(!word_pair){
			continue;
		}

		// 앞 행을 끝맺은 단어를 떠 본다 — 가상 괄호를 여는 키워드면 적법하다.
		std::string const last_word = Word_ending_at(a, a_end);
		bool opener = false;

		for(char const * const w : Openers){
			if(last_word == w){
				opener = true;
			}
		}

		if(opener){
			continue;
		}

		// §5.5 2칸 마커 — 주석을 걷어낸 행의 꼬리에 정확히 2칸.
		if( Tail_spaces(cut_lines[r]) == 2 ){
			continue;
		}

		out.push_back(
			{
				r, a_end, "5.5",
				std::string(
					"word wraps to word without the two-space marker: a variable declaration is"
					" missing its marker (5.5), or two adjacent tokens are split by a newline (9.3)"
				)
				+ (crossed ? " — blank lines do not license the split" : "")
			}
		);
	}
}

// §5.5 — 이름이 타입에 붙은 채 `=` 로 이어지는 다중행 변수 선언.
//
// M1(Check_unmarked_wrap)은 타입 행 다음이 **단어**로 시작하는 자리를 본다. 그 형제가 여기다 —
// 타입과 이름이 한 행에 붙어 있고 다음 행이 **`=`** 로 시작하는 자리. 변수 선언문의 가상 괄호가
// 다중행이므로 타입 표현이 끝나는 행에 2칸 마커가 있어야 하고 이름은 한 단계 깊게 내려가야
// 하는데(§5.5), 그 둘을 한 행에 붙여 두면 마커가 설 자리조차 없다.
//
// **선언인지 어떻게 아는가.** 대입문의 좌변은 하나의 표현식이라 **피연산자가 둘 나란히 설 수
// 없다.** 그러므로 `=` 앞 행의 마지막 토큰이 단어이고 그 앞이 (같은 행의) 단어·닫는 꺾쇠·앞에
// 단어를 둔 데코레이터라면, 그 행은 무언가를 선언한 것이다.
//
//   result / obj.field / arr[i] / *p     → 표현식 (앞이 없거나 양방향 연산자·괄호)
//   int x / vector<int> v / Foo *p / ns::Type v3  → 선언 (앞이 단어·닫는 꺾쇠·데코레이터)
//
// 빼는 자리 셋: ① 함수 기본설정·삭제·순수가상(`auto f() const` `= default;`) — 마지막 단어 앞이
// 닫는 괄호라 술어가 저절로 비켜간다. ② 템플릿 기본 인자·함수 기본 매개변수 — 표현식 괄호 안이라
// 아래 가드로 걸러진다. ③ C++20 `concept` — 규약이 아직 다루지 않는다.
void sak::Check_glued_declarator(
	Lines const &cut_lines, std::vector<Adj_tok> const &toks,
	std::vector<Bk_pair> const &pairs, std::vector<Angle_pair> const &angles,
	std::vector<Violation> &out
){
	int const  
		n = static_cast<int>(toks.size()),
		rows = static_cast<int>(cut_lines.size())
	;

	// 표현식 괄호(`(` `[` `[[` 와 꺾쇠)의 안쪽 — 함수 기본 매개변수와 템플릿 기본 인자가 사는
	// 자리다. 중괄호는 다르다: 그 안에는 문장이 살기 때문에 변수 선언이 설 수 있다.
	std::vector<char> inner(rows, 0);

	for(Bk_pair const &p : pairs){
		if(p.kind == '{'){
			continue;
		}

		for(int r = p.o_row + 1; r < p.c_row && r < rows; ++r){
			inner[r] = 1;
		}
	}

	for(Angle_pair const &a : angles){
		for(int r = a.o_row + 1; r < a.c_row && r < rows; ++r){
			inner[r] = 1;
		}
	}

	for(int i = 1; i < n; ++i){
		Adj_tok const &eq = toks[i];

		// 다음 행을 여는 `=` 인가 (그 행의 첫 토큰이고, 앞 토큰은 윗 행에 있다).
		bool const  
			leads_row
			= eq.cls == Adj_cls::bidir && eq.text == "="
			&& toks[i - 1].row == eq.row - 1
		;

		if(!leads_row){
			continue;
		}

		int const r = toks[i - 1].row;

		if(r < 0 || r >= rows || inner[r] != 0){
			continue;
		}

		// 앞 행의 마지막 토큰(=선언 대상 후보)과 그 앞 토큰.
		Adj_tok const &name = toks[i - 1];

		bool const  
			same_row_prev
			= i - 2 >= 0 && toks[i - 2].row == name.row
		;

		if(name.cls != Adj_cls::word || !same_row_prev){
			continue;
		}

		Adj_tok const &lead = toks[i - 2];

		// 데코레이터가 끼어 있으면(`Foo *p`) 그 앞까지 본다 — 앞이 없으면 역참조다(`*p = y`).
		bool decl = false;

		if(lead.cls == Adj_cls::word || lead.cls == Adj_cls::angle_close){
			decl = lead.text != "concept";
		}
		else if(
			lead.cls == Adj_cls::unidir
			&& (lead.text == "*" || lead.text == "&" || lead.text == "&&")
		){
			bool const  
				typed_lead
				= i - 3 >= 0 && toks[i - 3].row == name.row
				&& (
					toks[i - 3].cls == Adj_cls::word
					|| toks[i - 3].cls == Adj_cls::angle_close
				)
			;

			decl = typed_lead;
		}

		if(!decl || cut_lines[r].size() < 2){
			continue;
		}

		// 2칸 마커가 있으면 이 행은 타입 표현이 끝나는 행이다 — 여기 걸릴 일이 없다.
		if( Tail_spaces(cut_lines[r]) == 2 ){
			continue;
		}

		out.push_back(
			{
				name.row, name.col, "5.5",
				"multi-line variable declaration: the declarator is glued to its type."
				" Put the type alone on its line with the two-space marker, the declarator"
				" one level deeper, and the closing ';' on its own line"
			}
		);
	}
}

// 감싸는 `{` 쌍이 문장·선언의 스코프(함수·제어문·람다 본체, namespace, 클래스 본체, extern
// 링키지 블록)인지 — 그 안의 행들은 괄호의 이음줄이 아니라 문장들이라, 인접 문자열 리터럴의
// 개행을 면허하는 "다중행 괄호 안"이 아니다. 판정이 서지 않는 여는 자리(중괄호 초기화 `Foo{`,
// `]`·`>` 뒤 등)는 false 로 두어 보수적으로 면허한다(위양성 0 우선 — 거짓 음성 수용).
auto sak::Brace_opens_statement_scope(Lines const &mask, Bk_pair const &p)->bool{
	int r = p.o_row, c = p.o_col - 1;

	if( !Prev_significant(mask, r, c) ){
		return true; // 파일 첫머리의 나체 블록
	}

	char const ch = mask[r][c];

	if(ch == ')'){
		return true; // 함수·제어문·람다 본체
	}

	if( !is_word_char(ch) ){
		return false; // `=`·`,`·`(`·`>`·`:`·`]` 등 — 초기화·표현식 문맥으로 보수 분류
	}

	std::string const w = Word_ending_at(mask[r], c);
	int const s = c - static_cast<int>(w.size());

	bool const  
		scope_kw
		= w == "do" || w == "else" || w == "try" || w == "namespace" || w == "extern"
		|| w == "struct" || w == "class" || w == "union" || w == "enum"
	;

	if(scope_kw){
		return true;
	}

	std::string const before = Word_before(mask, r, s + 1);

	return
		before == "struct" || before == "class" || before == "union" || before == "enum"
		|| before == "namespace"
	;
}

// §9.1 인접 문자열 리터럴 — 두 리터럴 사이의 개행은 다중행 괄호 안에서만 허용된다.
// 토큰 스트림에서 연속한 두 일반 문자열 리터럴이 정확히 한 행 차이로 이어질 때, 이 자리를
// 엄격히 포함하는 표현 괄호(`(`·`[`·`[[`·꺾쇠·초기화류 `{`)가 하나도 없으면 위반. 문장 스코프의
// `{`(Brace_opens_statement_scope)는 면허가 아니다. 가상 괄호(『)는 pairs 에 없으므로, 문장
// 머리까지 거슬러 올라 그 흔적 — 여는 키워드(return 등)·변수 선언문의 2칸 마커 — 이 보이면
// 보수적으로 침묵한다. 원시 문자열·문자 리터럴·사이에 주석·공행이 낀 자리도 침묵(sak_coverage).
// 접합 자리의 우선순위는 Prio::str_adj(§6.1 ◆) — §9.2 경쟁 통합은 커버리지 밖.
void sak::Check_string_splice_newline(
	Lines const &mask, Lines const &cut_lines, Seg_lines const &segs,
	std::vector<Adj_tok> const &toks, std::vector<Bk_pair> const &pairs,
	std::vector<Angle_pair> const &angles, std::vector<Violation> &out
){
	auto const  
		is_plain_string
		= [&segs](int const row, int const col)->bool{
			if( row >= static_cast<int>(segs.size()) ){
				return false;
			}

			for(Segment const &s : segs[row]){
				if(s.col == col){
					return s.kind == Seg_kind::string_lit;
				}
			}

			return false;
		}
	;

	auto const  
		has_comment
		= [&segs](int const row, int const col, bool const after)->bool{
			if( row >= static_cast<int>(segs.size()) ){
				return false;
			}

			for(Segment const &s : segs[row]){
				if( s.kind == Seg_kind::comment && (after ? s.col > col : s.col < col) ){
					return true;
				}
			}

			return false;
		}
	;

	auto const  
		has_marker_tail
		= [&cut_lines](int const row)->bool{
			if( row < 0 || row >= static_cast<int>(cut_lines.size()) ){
				return false;
			}

			return Tail_spaces(cut_lines[row]) == 2;
		}
	;

	int const n = static_cast<int>(toks.size());

	for(int i = 0; i + 1 < n; ++i){
		Adj_tok const &a = toks[i];
		Adj_tok const &b = toks[i + 1];

		bool const  
			splice
			= a.cls == Adj_cls::lit && b.cls == Adj_cls::lit && b.row == a.row + 1
			&& is_plain_string(a.row, a.col) && is_plain_string(b.row, b.col)
			&& !has_comment(a.row, a.col, true) && !has_comment(b.row, b.col, false)
		;

		if(!splice){
			continue;
		}

		bool licensed = false;

		for(Bk_pair const &p : pairs){
			bool const  
				contains
				= ( p.o_row < a.row || (p.o_row == a.row && p.o_col < a.col) )
				&& ( p.c_row > b.row || (p.c_row == b.row && p.c_col > b.col + b.len - 1) )
			;

			if(  contains && ( p.kind != '{' || !Brace_opens_statement_scope(mask, p) )  ){
				licensed = true;

				break;
			}
		}

		for(Angle_pair const &g : angles){
			if(licensed){
				break;
			}

			bool const  
				contains
				= ( g.o_row < a.row || (g.o_row == a.row && g.o_col < a.col) )
				&& ( g.c_row > b.row || (g.c_row == b.row && g.c_col > b.col + b.len - 1) )
			;

			if(contains){
				licensed = true;
			}
		}

		if(licensed){
			continue;
		}

		// 문장 머리까지 거슬러 가상 괄호의 흔적을 찾는다 — 있으면 침묵(『 는 pairs 밖이다).
		bool virtual_trace = false;
		int stmt_lo = a.row;

		for(int j = i - 1; j >= 0 && i - j < 400; --j){
			Adj_tok const &t = toks[j];

			bool const  
				boundary
				= t.cls == Adj_cls::semi
				|| (
					(t.cls == Adj_cls::open_b || t.cls == Adj_cls::close_b)
					&& (t.text == "{" || t.text == "}")
				)
			;

			if(boundary){
				break;
			}

			stmt_lo = t.row;

			bool const  
				opener
				= t.cls == Adj_cls::word
				&& (
					t.text == "return" || t.text == "throw" || t.text == "using"
					|| t.text == "case" || t.text == "co_return" || t.text == "co_yield"
				)
			;

			if(opener){
				virtual_trace = true;

				break;
			}
		}

		for(int r = stmt_lo; !virtual_trace && r <= a.row; ++r){
			virtual_trace = has_marker_tail(r);
		}

		if(virtual_trace){
			continue;
		}

		out.push_back(
			{
				b.row, b.col, "9.1",
				"adjacent string literals: the newline between them is allowed only"
				" inside a multi-line bracket — wrap them in parentheses"
			}
		);
	}
}

// §9.3 — `}` 로 끝난 행 다음 코드 행이 `(`·`[` 로 시작하는 자리. IIFE 의 호출 괄호가 절단된
// 것이면 위반(부착 자리 — §4.2 틈 조건)이고, 괄호문(`(*fp)();`)·람다문 같은 새 문장의 머리면
// 적법하다. 렉서 수준에서는 갈리지 않아 용의로만 지목한다(속성 `[[` 는 제외 — 적법한 머리).
void sak::Check_brace_paren_newline(
	Lines const &lines, Lines const &mask, std::vector<Violation> &out
){
	int const rows = static_cast<int>(mask.size());

	for(int r = 0; r < rows; ++r){
		int const l = Last_significant_col(mask[r]);

		if(l < 0 || mask[r][l] != '}'){
			continue;
		}

		bool crossed = false;
		int const nr = Next_code_row_over_blanks(lines, mask, r, crossed);

		if(nr < 0){
			continue;
		}

		int const nc = First_significant_col(mask[nr]);

		if(nc < 0){
			continue;
		}

		char const ch = mask[nr][nc];

		bool const  
			cand
			= ch == '('
			|| (
				ch == '['
				&& ( nc + 1 >= static_cast<int>(mask[nr].size()) || mask[nr][nc + 1] != '[' )
			)
		;

		if(!cand){
			continue;
		}

		Violation  
			v{
				nr, nc, "9.3",
				std::string("newline between '}' and '") + ch
				+ "': a call attachment (IIFE) or a new statement? notation cannot settle this"
			}
		;

		v.cat = V_cat::suspect;
		out.push_back(v);
	}
}

// 충돌 사각 — 확정 위반이 아니라 용의로 지목해 사람의 판정에 넘긴다.
void sak::Check_suspects(std::vector<Adj_tok> const &toks, std::vector<Violation> &out){
	for(Adj_tok const &t : toks){
		if(!t.suspect){
			continue;
		}

		Violation  
			v{
				t.row, t.col, "8.4",
				"'" + t.text + "': notation cannot settle this — declaration or operator?"
			}
		;

		v.cat = V_cat::suspect;
		out.push_back(v);
	}
}
