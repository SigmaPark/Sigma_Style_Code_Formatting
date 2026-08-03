/*  SPDX-FileCopyrightText: (c) 2020 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "check_internal.hpp"

#include <string>
#include <vector>

namespace sak{
	static auto Closer_of(char const open)->char;
	static auto is_basic_type(std::string const &w)->bool;
	static auto Token_text(std::string const &mask, Tok_8_3 const &t)->std::string;
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

// §1.1 행 표시 폭 100 초과 (raw 행).
void sak::Check_width(std::string const &line, int const row, std::vector<Violation> &out){
	std::size_t const w = Display_width(line);

	if(w > 100){
		std::string const msg = "width " + std::to_string(w) + " > 100";

		out.push_back({ row, 100, "1.1", msg });
	}
}

// §1.3 들여쓰기 (raw 행) — 행 머리는 들여쓰기 단위(탭 하나 또는 공백 4칸, §8.2)의 나열이다.
// 단위에 못 든 남은 공백(1~3칸)은 위반. 공행은 들여쓰기가 아니므로(§9.4) 건너뛴다.
void sak::Check_indent(std::string const &line, int const row, std::vector<Violation> &out){
	if( Is_blank_row(line) ){
		return;
	}

	int const n = static_cast<int>(line.size());
	int p = 0;

	while(true){
		int const len = Indent_unit_at(line, p);

		if(len == 0){
			break;
		}

		p += len;
	}

	if(p < n && line[p] == ' '){
		out.push_back({ row, p, "1.3", "space in indentation" });
	}
}

// §8.3 잉여공백 — 개행 앞 공백은 잉여라 보존한다(§5.5 2칸 마커의 토대). v2.10 에서 §8 은
// 가상문자를 배제(§8.1)하고, 옛 '무효공백'은 소멸했다: 행 시작 공백은 §1.3 Check_indent,
// 4연속+는 §8.2 Check_space_run, 꼬리 탭은 §8.2 Check_tab_use 가 각각 담당하므로
// §8.3 는 별도 후행-공백 검사를 두지 않는다.

// §8.2 들여쓰기 이외 용도의 탭 (@마스크). 행 머리(탭·공백 4칸이 섞일 수 있다) 이후의 탭은 위반.
void sak::Check_tab_use(std::string const &mask, int const row, std::vector<Violation> &out){
	if( Is_blank_row(mask) ){
		return;
	}

	int const n = static_cast<int>(mask.size());
	int p = 0;

	while( p < n && (mask[p] == '\t' || mask[p] == ' ') ){
		++p;
	}

	while(p < n){
		if(mask[p] == '\t'){
			out.push_back({ row, p, "8.2", "tab outside indentation" });
		}

		++p;
	}
}

// §8.2 공백 4칸 이상 연속 (@마스크). 선두 들여쓰기 구역은 §1.3 소관이라 건너뛴다.
void sak::Check_space_run(std::string const &mask, int const row, std::vector<Violation> &out){
	if( Is_blank_row(mask) ){
		return;
	}

	int const n = static_cast<int>(mask.size());
	int p = 0;

	while( p < n && (mask[p] == '\t' || mask[p] == ' ') ){
		++p;
	}

	while(p < n){
		if(mask[p] != ' '){
			++p;
			continue;
		}

		int const begin = p;

		while(p < n && mask[p] == ' '){
			++p;
		}

		if(p - begin >= 4){
			out.push_back({ row, begin, "8.2", "4 or more consecutive spaces" });
		}
	}
}

// 여는 괄호에 대응하는 닫는 괄호.
auto sak::Closer_of(char const open)->char{
	if(open == '('){
		return ')';
	}

	if(open == '['){
		return ']';
	}

	return '}';
}

// §8.5 한 행 안 중첩 괄호의 안쪽 공백 (@마스크). ( ) [ ] { } 만 대상.
// 같은 종류 중첩 단계로 n 결정: ()[] 는 n=단계, {} 는 n=단계+1.
// < > 와 [[ ]] 와 다중행 괄호는 제외(에이전트 몫).
// 내용 없는 괄호쌍(§8.5) — 안이 모두 공백이면 그 공백을 지우고(n=0, 중괄호도 예외 없음)
// 중첩 단계에도 세지 않는다. 문자가 곧바로 인접한 빈 쌍은 검사할 것이 없어 그대로 지나친다.
void sak::Check_inner_space(std::string const &mask, int const row, std::vector<Violation> &out){
	struct Frame{
		char open;
		int col;
		int child;   // 안에 든 같은 종류 비어있지 않은 짝의 최대 단계 (없으면 -1)
	};

	struct Pair{
		char open;
		int from, to;
		int stage;
		bool blank;   // 안이 모두 공백 — 지워야 할 자리
	};

	int const n = static_cast<int>(mask.size());
	std::vector<Frame> stack;
	std::vector<Pair> pairs;
	bool aborted = false;
	int i = 0;

	while(i < n){
		char const c = mask[i];

		// [[ ... ]] 어트리뷰트는 별도 괄호류라 §8.5 공식 밖 → 통째로 건너뛴다.
		if(c == '[' && i + 1 < n && mask[i + 1] == '['){
			int j = i + 2;

			while( j + 1 < n && !(mask[j] == ']' && mask[j + 1] == ']') ){
				++j;
			}

			if(j + 1 >= n){
				aborted = true;

				break;
			}

			i = j + 2;

			continue;
		}

		if(c == '(' || c == '[' || c == '{'){
			stack.push_back({ c, i, -1 });
			++i;

			continue;
		}

		if(c == ')' || c == ']' || c == '}'){
			if(stack.empty()){
				++i;

				continue;
			}

			if( c != Closer_of(stack.back().open) ){
				aborted = true;

				break;
			}

			Frame const top = stack.back();
			stack.pop_back();

			if(i > top.col + 1){
				bool blank = true;

				for(int cc = top.col + 1; cc < i && blank; ++cc){
					blank = mask[cc] == ' ';
				}

				int const stage = blank || top.child < 0 ? 0 : top.child + 1;

				pairs.push_back({ top.open, top.col, i, stage, blank });

				int k = blank ? -1 : static_cast<int>(stack.size()) - 1;

				while(k >= 0){
					if(stack[k].open == top.open){
						if(stage > stack[k].child){
							stack[k].child = stage;
						}

						break;
					}

					--k;
				}
			}

			++i;

			continue;
		}

		++i;
	}

	if(aborted){
		return;
	}

	for(Pair const &pr : pairs){
		int const want = pr.blank ? 0 : pr.open == '{' ? pr.stage + 1 : pr.stage;

		std::string const  
			msg
			= pr.blank
			? std::string("empty bracket pair must have no inner space")
			: "inner space must be " + std::to_string(want)
		;

		int after = 0;

		while(pr.from + 1 + after < n && mask[pr.from + 1 + after] == ' '){
			++after;
		}

		int before = 0;

		while(pr.to - 1 - before > pr.from && mask[pr.to - 1 - before] == ' '){
			++before;
		}

		if(after != want){
			Push_fix(out, { row, pr.from, "8.5", msg }, Fix_kind::gap_right, pr.from + 1, want);
		}

		if(before != want){
			Push_fix(out, { row, pr.to, "8.5", msg }, Fix_kind::gap_left, pr.to, want);
		}
	}
}

// §3 제어문 중괄호 강제 (@마스크 + 인접 행). 키워드 다음 본문이 '{' 인지 본다.
// if/for/while/switch 는 조건 ')' 다음을, do/else 는 키워드 다음을 본다.
// while(...); 는 직전 '}' 가 do 블록을 닫으면 do-while 꼬리라 합법, 아니면 위반(빈본문 while).
void sak::Check_ctrl_brace(Lines const &mask, int const row, std::vector<Violation> &out){
	std::string const &line = mask[row];
	int const len = static_cast<int>(line.size()), last = static_cast<int>(mask.size()) - 1;
	int const max_row = row + 4 < last ? row + 4 : last;

	for(int i = 0; i < len;){
		if( !Word_starts_at(line, i) ){
			++i;

			continue;
		}

		std::string const word = Word_at(line, i);
		int const e = i + static_cast<int>(word.size());

		auto const  
			[br, bc, body_found]
			= [row, e, &mask, max_row, &word](bool const has_cond){
				struct{ int _0, _1; bool _2; } res{ row, e, false };
				auto &[br, bc, body_found] = res;

				if(has_cond){
					if(
						int pr = row, pc = e;
						Next_code(mask, max_row, pr, pc) && mask[pr][pc] == '('
					){
						if( Match_paren(mask, max_row, pr, pc) ){
							br = pr;
							bc = pc + 1;
							body_found = Next_code(mask, max_row, br, bc);
						}
					}
				}
				else if(word == "do" || word == "else"){
					body_found = Next_code(mask, max_row, br, bc);
				}

				return res;
			}(word == "if" || word == "for" || word == "while" || word == "switch")
		;

		if(body_found){
			char const body = mask[br][bc];

			bool const  
				legal
				= word == "while" && body == ';' ? Is_do_tail(mask, row, i)
				: word == "else" && body != '{' ? Word_at(mask[br], bc) == "if"
				: body == '{'
			;

			if(!legal){
				out.push_back({ row, i, "3", word + " needs braces" });
			}
		}

		i = e;
	}
}

// §8.4 부착 법칙 — 비기호형 토큰과 괄호의 간격 (@마스크, 같은 행).
// 여는 괄호는 앞의 피연산 표현을 피연산자로 갖는 단방향 기호형 토큰이다(§4 여는 괄호의
// 신분) — 아래 규칙들은 전부 그 신분의 귀결이라 키워드·이름·닫는 괄호를 가리지 않는 일반형이다.
// 단어 다음 여는괄호 ((·[·{) 사이의 공백 = 위반.
// 닫는괄호 ())·]·}) 다음에 단어가 무공백으로 붙으면 = 위반.
// 인접한 두 단어 사이 공백은 정확히 한 칸(v2.2.0).
// 닫는괄호 다음에 여는괄호가 오면 그 사이 공백 = 위반(v2.2.0) — 앞의 닫는 괄호까지를 하나의
// 피연산 토큰으로 보고 그 뒤에서 새 괄호가 열리는 자리다(호출·첨자의 연쇄, 람다).
// 꺾쇠 < > 는 비교/꺾쇠 모호성 탓에 sak 보수 영역에서 제외(§5.1·범주 4).
void sak::Check_word_paren_space(
	std::string const &mask, int const row, std::vector<Violation> &out
){
	auto const is_open = [](char const ch)->bool{ return ch == '(' || ch == '[' || ch == '{'; };
	auto const is_close = [](char const ch)->bool{ return ch == ')' || ch == ']' || ch == '}'; };

	int const n = static_cast<int>(mask.size());
	int p = 0;

	while( p < n && (mask[p] == '\t' || mask[p] == ' ') ){
		++p;
	}

	while(p < n){
		char const c = mask[p];

		if( is_word_char(c) ){
			int const  
				e
				= [n, &mask](int res){
					for( ; res < n && is_word_char(mask[res]); ++res ){}

					return res;
				}(p),
				q
				= [n, &mask](int res){
					for(; res < n && mask[res] == ' '; ++res){}

					return res;
				}(e)
			;

			if( q > e && q < n && is_open(mask[q]) ){
				Push_fix(
					out, { row, e, "8.4", "space between word and opening bracket" },
					Fix_kind::gap_right, e, 0
				);
			}

			if( q - e > 1 && q < n && is_word_char(mask[q]) ){
				Push_fix(
					out, { row, e, "8.4", "words must be separated by exactly one space" },
					Fix_kind::gap_right, e, 1
				);
			}

			p = e;

			continue;
		}

		if( is_close(c) ){
			if( p + 1 < n && is_word_char(mask[p + 1]) ){
				Push_fix(
					out, { row, p + 1, "8.4", "missing space between closing bracket and word" },
					Fix_kind::gap_left, p + 1, 1
				);
			}

			int const  
				q
				= [n, &mask](int res){
					for(; res < n && mask[res] == ' '; ++res){}

					return res;
				}(p + 1)
			;

			if( q > p + 1 && q < n && is_open(mask[q]) ){
				Push_fix(
					out, { row, p + 1, "8.4", "no space between closing and opening bracket" },
					Fix_kind::gap_right, p + 1, 0
				);
			}
		}

		++p;
	}
}

// §4.3 절 연쇄의 응집·분리 (@마스크). `else`·`catch`, 그리고 do 블록 꼬리 `while` 은 앞 블록의
// 닫는 `}` 와 가상연산자 `▽` 로 이어진다. 본체가 다중행이면 `▽` 가 개행하므로 이 토큰은 `}` 의
// 다음 행 머리에 와야 하고(같은 행 응집=위반), 본체가 단일행이면 `▽` 가 개행하지 않으므로 `}` 와
// 한 행에 응집해야 한다(다음 행 분리=위반). 판정은 그 `}` 의 짝 `{` 이 다른 행인지(다중행 본체)로만
// 한다 — 짝을 못 찾거나 앞이 `}` 가 아니면 보수적으로 침묵한다(매크로·불완전).
void sak::Check_continuation_cohesion(
	Lines const &mask, int const row, std::vector<Violation> &out
){
	std::string const &m = mask[row];
	int const n = static_cast<int>(m.size());

	for(int c = 0; c < n; ++c){
		if( !Word_starts_at(m, c) ){
			continue;
		}

		std::string const w = Word_at(m, c);
		bool is_clause = w == "else" || w == "catch";

		if( w == "while" && Is_do_tail(mask, row, c) ){
			is_clause = true;
		}

		if(!is_clause){
			continue;
		}

		int pr = row, pc = c - 1;

		if( !Prev_significant(mask, pr, pc) || mask[pr][pc] != '}' ){
			continue;
		}

		bool const glued = pr == row;

		// 행머리 이어감인데 행머리에 주석·리터럴 꼬리(@)가 끼면 §2 인접 — 보수적 침묵.
		if(!glued){
			bool at_head = true;

			for(int i = 0; i < c; ++i){
				if(m[i] != ' ' && m[i] != '\t'){
					at_head = false;

					break;
				}
			}

			if(!at_head){
				continue;
			}
		}

		int br = pr, bc = pc;

		if( !Match_brace_back(mask, br, bc) ){
			continue;
		}

		bool const multiline_body = br != pr;

		if(glued && multiline_body){
			out.push_back(
				{ row, c, "4.3", "'" + w + "' after a multi-line body: move it to the next line" }
			);
		}
		else if(!glued && !multiline_body){
			out.push_back(
				{ row, c, "4.3", "'" + w + "' after a single-line body: cohere it on the '}' line" }
			);
		}
	}
}

// @마스크 한 행을 토큰열로 변환. 더 긴 모양 우선 매칭, 의심 자리는 모두 skip.
auto sak::Tokenize_8_3(std::string const &mask)->std::vector<Tok_8_3>{
	std::vector<Tok_8_3> out;
	int const n = static_cast<int>(mask.size());
	int i = 0;

	while(i < n){
		char const c = mask[i];

		if(c == ' ' || c == '\t' || c == '@'){
			++i;

			continue;
		}

		if( is_word_char(c) ){
			int const s = i;

			while( i < n && is_word_char(mask[i]) ){
				++i;
			}

			out.push_back({ s, i - s, Tk_cls::word });

			continue;
		}

		// CUDA <<< >>> 는 한 덩이 괄호로 본다(§5). 토큰화 단계에서 통째 잡아 skip.
		if(c == '<' && i + 2 < n && mask[i + 1] == '<' && mask[i + 2] == '<'){
			out.push_back({ i, 3, Tk_cls::open_b });
			i += 3;

			continue;
		}

		if(c == '>' && i + 2 < n && mask[i + 1] == '>' && mask[i + 2] == '>'){
			out.push_back({ i, 3, Tk_cls::close_b });
			i += 3;

			continue;
		}

		// [[ ]] 속성괄호도 한 덩이로.
		if(c == '[' && i + 1 < n && mask[i + 1] == '['){
			out.push_back({ i, 2, Tk_cls::open_b });
			i += 2;

			continue;
		}

		if(c == ']' && i + 1 < n && mask[i + 1] == ']'){
			out.push_back({ i, 2, Tk_cls::close_b });
			i += 2;

			continue;
		}

		if(c == '(' || c == '[' || c == '{'){
			out.push_back({ i, 1, Tk_cls::open_b });
			++i;

			continue;
		}

		if(c == ')' || c == ']' || c == '}'){
			out.push_back({ i, 1, Tk_cls::close_b });
			++i;

			continue;
		}

		// 3-char 기호형 (긴 모양 우선)
		if(i + 2 < n){
			char const c2 = mask[i + 1], c3 = mask[i + 2];

			if(c == '-' && c2 == '>' && c3 == '*'){
				out.push_back({ i, 3, Tk_cls::bin_ns });
				i += 3;

				continue;
			}

			if(c == '<' && c2 == '=' && c3 == '>'){
				out.push_back({ i, 3, Tk_cls::bin_s });
				i += 3;

				continue;
			}

			if( (c == '<' || c == '>') && c2 == c && c3 == '=' ){
				out.push_back({ i, 3, Tk_cls::bin_s });
				i += 3;

				continue;
			}

			if(c == '.' && c2 == '.' && c3 == '.'){
				out.push_back({ i, 3, Tk_cls::skip });
				i += 3;

				continue;
			}
		}

		// 2-char 기호형
		if(i + 1 < n){
			char const c2 = mask[i + 1];

			// :: 는 범위해결(양방향)과 전역(단방향) 분리가 의미적이라 skip.
			if(c == ':' && c2 == ':'){
				out.push_back({ i, 2, Tk_cls::skip });
				i += 2;

				continue;
			}

			if( (c == '=' || c == '!' || c == '<' || c == '>') && c2 == '=' ){
				out.push_back({ i, 2, Tk_cls::bin_s });
				i += 2;

				continue;
			}

			if(c == '|' && c2 == '|'){
				out.push_back({ i, 2, Tk_cls::bin_s });
				i += 2;

				continue;
			}

			if( (c == '+' && c2 == '+') || (c == '-' && c2 == '-') ){
				out.push_back({ i, 2, Tk_cls::inc_dec });
				i += 2;

				continue;
			}

			if(
				c2 == '='
				&& (
					c == '+' || c == '-' || c == '*' || c == '/' || c == '%'
					|| c == '&' || c == '^' || c == '|'
				)
			){
				out.push_back({ i, 2, Tk_cls::bin_s });
				i += 2;

				continue;
			}

			if(c == '-' && c2 == '>'){
				out.push_back({ i, 2, Tk_cls::bin_ns });
				i += 2;

				continue;
			}

			if(c == '.' && c2 == '*'){
				out.push_back({ i, 2, Tk_cls::bin_ns });
				i += 2;

				continue;
			}

			// && << >> 는 분류 모호(우값참조·템플릿 닫힘·스트림 vs 시프트)라 skip.
			if(
				(c == '&' && c2 == '&')
				|| (c == '<' && c2 == '<')
				|| (c == '>' && c2 == '>')
			){
				out.push_back({ i, 2, Tk_cls::skip });
				i += 2;

				continue;
			}
		}

		// 1-char 기호형
		switch(c){
		case ';':
		case ',':
			out.push_back({ i, 1, Tk_cls::sep });

			break;

		case '.':
			out.push_back({ i, 1, Tk_cls::bin_ns });

			break;

		case '?':
		case '=':
		case '/':
		case '%':
		case '|':
		case '^':
			out.push_back({ i, 1, Tk_cls::bin_s });

			break;

		default:
			// * & + - < > : ! ~ 등 분류 의존 토큰은 모두 skip.
			out.push_back({ i, 1, Tk_cls::skip });

			break;
		}

		++i;
	}

	return out;
}

// §8.4 문맥 불변 토큰의 단일행 공백 검사 (@마스크).
// 그룹별 규칙:
//   sep `;` `,`     — 앞 공백 0, 뒤 공백 ≥ 1 (다음 토큰이 close_b·sep이면 skip).
//   bin_ns . -> .* ->*  — 양쪽 공백 0. 단 좌/우 토큰이 같은 행에 없거나 분류가
//                          word/괄호가 아니면 그 쪽은 검사 제외(낫괄호·다중행 영역 양보).
//   bin_s 양쪽 공백 토큰 — 양쪽 공백 ≥ 1. 좌/우가 word·close_b/open_b 가 아닌
//                          기호형이면 그 쪽 검사 제외(연쇄·단항 영역 양보).
//   inc_dec ++ --    — 한 쪽은 0(피연산자 부착). 양쪽 모두 공백 > 0 이면 위반.
// 괄호 경계 투명성(§5.3): 단일행 괄호 안 첫·마지막 토큰은 감싸는 괄호를 인접 토큰으로 보지 않음.
void sak::Check_token_space(
	std::string const &mask, int const row, std::vector<Violation> &out
){
	auto const toks = Tokenize_8_3(mask);
	int const n = static_cast<int>(toks.size()), mask_n = static_cast<int>(mask.size());

	if(n < 2){
		return;
	}

	auto const  
		gap_before
		= [&mask, &toks](int const i)->int{
			int g = 0;

			while(toks[i].col - 1 - g >= 0 && mask[ toks[i].col - 1 - g ] == ' '){
				++g;
			}

			return g;
		}
	;

	auto const  
		gap_after
		= [&mask, &toks, mask_n](int const i)->int{
			int const e = toks[i].col + toks[i].len;
			int g = 0;

			while(e + g < mask_n && mask[e + g] == ' '){
				++g;
			}

			return g;
		}
	;

	auto const  
		word_eq
		= [&mask, &toks](int const i, char const *kw)->bool{
			if(toks[i].cls != Tk_cls::word){
				return false;
			}

			int j = 0;

			while(kw[j] != '\0' && j < toks[i].len && mask[ toks[i].col + j ] == kw[j]){
				++j;
			}

			return kw[j] == '\0' && j == toks[i].len;
		}
	;

	for(int i = 0; i < n; ++i){
		Tok_8_3 const &t = toks[i];
		bool const has_l = i > 0, has_r = i + 1 < n;

		Tk_cls const  
			left_cls = has_l ? toks[i - 1].cls : Tk_cls::skip,
			right_cls = has_r ? toks[i + 1].cls : Tk_cls::skip
		;

		// `operator =`·`operator ==`·`operator,` 등: `operator` 키워드 뒤 첫 기호형은
		// 오버로드 함수명의 일부이므로 §8.4 검사 영역 밖이다.
		if( has_l && word_eq(i - 1, "operator") ){
			continue;
		}

		// 투명성: 좌측 open_b·우측 close_b 는 인접 토큰으로 보지 않는다.
		bool const  
			eff_l = has_l && left_cls != Tk_cls::open_b,
			eff_r = has_r && right_cls != Tk_cls::close_b,
			l_operand = left_cls == Tk_cls::word || left_cls == Tk_cls::close_b,
			r_operand = right_cls == Tk_cls::word || right_cls == Tk_cls::open_b
		;

		switch(t.cls){
		case Tk_cls::sep:
			{
				// `for(init;;++itr2)` 처럼 `;` 두 개가 연속하면 그 사이는 공백 1 필수 —
				// 종결할 내용이 비어 부착 요구가 소멸하고 기본 간격이 남는다(§8.4).
				// `,` 끼리·`,`+`;` 혼합은 일반 SEP 룰을 그대로 따른다.
				bool const  
					semi_chain
					= t.len == 1 && mask[t.col] == ';'
					&& has_l && toks[i - 1].cls == Tk_cls::sep
					&& toks[i - 1].len == 1 && mask[ toks[i - 1].col ] == ';'
				;

				if( eff_l && !semi_chain && gap_before(i) > 0 ){
					Push_fix(
						out, { row, t.col, "8.4", "no space before separator" },
						Fix_kind::gap_left, t.col, 0
					);
				}

				if( eff_l && semi_chain && gap_before(i) == 0 ){
					Push_fix(
						out, { row, t.col, "8.4", "space required between consecutive ';'" },
						Fix_kind::gap_left, t.col, 1
					);
				}

				if( eff_r && right_cls != Tk_cls::sep && gap_after(i) == 0 ){
					Push_fix(
						out, { row, t.col + t.len, "8.4", "space required after separator" },
						Fix_kind::gap_right, t.col + t.len, 1
					);
				}
			}

			break;

		case Tk_cls::bin_ns:
			if(!has_l || !has_r){
				break;
			}

			if( eff_l && l_operand && gap_before(i) > 0 ){
				Push_fix(
					out, { row, t.col, "8.4", "no space before '.','->','.*','->*'" },
					Fix_kind::gap_left, t.col, 0
				);
			}

			if( eff_r && r_operand && gap_after(i) > 0 ){
				Push_fix(
					out, { row, t.col + t.len, "8.4", "no space after '.','->','.*','->*'" },
					Fix_kind::gap_right, t.col + t.len, 0
				);
			}

			break;

		case Tk_cls::bin_s:
			if( eff_l && l_operand && gap_before(i) == 0 ){
				Push_fix(
					out, { row, t.col, "8.4", "space required before binary operator" },
					Fix_kind::gap_left, t.col, 1
				);
			}

			if( eff_r && r_operand && gap_after(i) == 0 ){
				Push_fix(
					out, { row, t.col + t.len, "8.4", "space required after binary operator" },
					Fix_kind::gap_right, t.col + t.len, 1
				);
			}

			break;

		case Tk_cls::inc_dec:
			if( eff_l && eff_r && gap_before(i) > 0 && gap_after(i) > 0 ){
				out.push_back({ row, t.col, "8.4", "'++'/'--' must attach to operand" });
			}

			break;

		default:
			break;
		}
	}
}

// §3 금지 키워드 typedef/goto (@마스크, 단어 경계).
void sak::Check_banned(std::string const &mask, int const row, std::vector<Violation> &out){
	static std::string const Banned[] = { "typedef", "goto" };

	for(std::string const &kw : Banned){
		for(
			std::size_t pos = mask.find(kw);
			pos != std::string::npos;
			pos = mask.find(kw, pos + 1)
		){
			if(
				std::size_t const end = pos + kw.size();
				( pos == 0 || !is_word_char(mask[pos - 1]) )
				&& ( end >= mask.size() || !is_word_char(mask[end]) )
			){
				int const col = static_cast<int>(pos);

				out.push_back({ row, col, "3", "banned keyword: " + kw });
			}
		}
	}
}

// §3 키워드 위치 후보 — 기본 타입 키워드(닫힌 집합).
auto sak::is_basic_type(std::string const &w)->bool{
	static char const * const  
		Types[]
		= {
			"void", "bool", "char", "char8_t", "char16_t", "char32_t", "wchar_t",
			"short", "int", "long", "float", "double", "signed", "unsigned", "auto"
		}
	;

	for(char const * const t : Types){
		if(w == t){
			return true;
		}
	}

	return false;
}

// 토큰 t 의 @마스크상 텍스트.
auto sak::Token_text(std::string const &mask, Tok_8_3 const &t)->std::string{
	return mask.substr(t.col, t.len);
}

// §3 키워드 위치 — const/volatile/constexpr 후위, static/inline 전위 (@마스크, 단일행).
// 무위양성 유지를 위해 *기본 타입 키워드와 인접한* 자동 확정분만 잡는다(사용자 정의 타입
// 인접·다중행 선언은 서브에이전트 몫).
//   `const|volatile|constexpr` + 기본타입  → 한정자가 타입 앞 = 서향 위반.
//   기본타입 + `static|inline`             → 스토리지 지정자가 타입 뒤 = 위반.
// `if constexpr` 는 constexpr 뒤가 '(' 라 단어쌍이 아니어서 자연히 제외된다.
void sak::Check_keyword_position(
	std::string const &mask, int const row, std::vector<Violation> &out
){
	auto const toks = Tokenize_8_3(mask);
	int const n = static_cast<int>(toks.size());

	for(int i = 0; i + 1 < n; ++i){
		if(toks[i].cls != Tk_cls::word || toks[i + 1].cls != Tk_cls::word){
			continue;
		}

		std::string const w0 = Token_text(mask, toks[i]), w1 = Token_text(mask, toks[i + 1]);
		bool const qual0 = w0 == "const" || w0 == "volatile" || w0 == "constexpr";
		bool const stor1 = w1 == "static" || w1 == "inline";

		if( qual0 && is_basic_type(w1) ){
			out.push_back(
				{ row, toks[i].col, "3", "const/volatile/constexpr must follow its type" }
			);
		}
		else if( is_basic_type(w0) && stor1 ){
			out.push_back({ row, toks[i + 1].col, "3", "static/inline must precede its type" });
		}
	}
}

// §3 단항 연산자 병기 — `- -x`/`+ +x` 처럼 같은 단항 부호가 공백으로 갈린 자리 (@마스크, 단일행).
// 붙이면 `--`/`++` 가 되어 의미가 바뀌므로 괄호로 구분해야 한다(`-(-x)`).
// 첫 부호가 단항임이 어휘적으로 확실한 자리(직전이 여는괄호·구분자·양쪽공백 이항연산자)에서만
// 확정한다. 직전이 피연산자(식별자·닫는괄호 등)면 이항일 수 있어 서브에이전트 몫.
void sak::Check_unary_juxtaposition(
	std::string const &mask, int const row, std::vector<Violation> &out
){
	auto const toks = Tokenize_8_3(mask);
	int const n = static_cast<int>(toks.size());

	for(int i = 1; i + 1 < n; ++i){
		char const sign = mask[ toks[i].col ];

		bool const  
			same_unary_pair
			= toks[i].len == 1 && toks[i + 1].len == 1
			&& (sign == '-' || sign == '+') && mask[ toks[i + 1].col ] == sign
		;

		if(!same_unary_pair){
			continue;
		}

		Tk_cls const left = toks[i - 1].cls;

		bool const  
			first_is_unary
			= left == Tk_cls::open_b || left == Tk_cls::sep || left == Tk_cls::bin_s
		;

		if(first_is_unary){
			out.push_back(
				{ row, toks[i].col, "3", "unary operators need parentheses, not space" }
			);
		}
	}
}
