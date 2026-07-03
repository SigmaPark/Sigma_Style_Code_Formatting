/*  SPDX-FileCopyrightText: (c) 2020 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "check.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <vector>

// 코드포인트가 동아시아 와이드/전각 근사 범위에 드는지.
static auto is_wide(unsigned long const cp)->bool{
	struct Range{ unsigned long lo, hi; } constexpr  
		Wide_ranges[]
		= {
			{ 0x1100, 0x115F },
			{ 0x2E80, 0x303E },
			{ 0x3041, 0x33FF },
			{ 0x3400, 0x4DBF },
			{ 0x4E00, 0x9FFF },
			{ 0xA000, 0xA4CF },
			{ 0xAC00, 0xD7A3 },
			{ 0xF900, 0xFAFF },
			{ 0xFE30, 0xFE4F },
			{ 0xFF00, 0xFF60 },
			{ 0xFFE0, 0xFFE6 },
			{ 0x20000, 0x3FFFD }
		}
	;

	for(Range const &r : Wide_ranges){
		if(cp >= r.lo && cp <= r.hi){
			return true;
		}
	}

	return false;
}

// 행의 표시 폭 (§1.1): 탭=4, 전각=2, 그 외=1. UTF-8 을 코드포인트로 디코드해 잰다.
static auto Display_width(std::string const &line)->std::size_t{
	std::size_t const n = line.size();
	std::size_t width = 0, i = 0;

	while(i < n){
		unsigned char const lead = static_cast<unsigned char>(line[i]);

		if(lead == '\t'){
			width += 4;
			i += 1;

			continue;
		}

		std::size_t bytes = 1;
		unsigned long cp = lead;

		if( (lead >> 5) == 0x6 ){
			bytes = 2;
			cp = lead & 0x1F;
		} else if( (lead >> 4) == 0xE ){
			bytes = 3;
			cp = lead & 0x0F;
		} else if( (lead >> 3) == 0x1E ){
			bytes = 4;
			cp = lead & 0x07;
		}

		bool valid = i + bytes <= n;

		for(std::size_t k = 1; valid && k < bytes; ++k){
			unsigned char const cont = static_cast<unsigned char>(line[i + k]);

			if( (cont >> 6) != 0x2 ){
				valid = false;
			} else{
				cp = (cp << 6) | (cont & 0x3F);
			}
		}

		if(!valid){
			width += 1;
			i += 1;

			continue;
		}

		width += ::is_wide(cp) ? 2 : 1;
		i += bytes;
	}

	return width;
}

static auto is_word_char(char const c)->bool{
	unsigned char const u = static_cast<unsigned char>(c);

	return std::isalnum(u) || c == '_';
}

// 위반을 내면서 edit 모드용 수정 힌트(§8.2/§11.2 토대)를 함께 싣는다.
// (집합체 초기화를 한 행에 유지하고자 힌트는 여기서 붙인다.)
static void Push_fix(
	std::vector<Violation> &out, Violation v,
	Fix_kind const kind, int const fix_col, int const fix_val
){
	v.fix = kind;
	v.fix_col = fix_col;
	v.fix_val = fix_val;
	out.push_back(v);
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

// §1.1 행 표시 폭 100 초과 (raw 행).
static void Check_width(std::string const &line, int const row, std::vector<Violation> &out){
	std::size_t const w = ::Display_width(line);

	if(w > 100){
		std::string const msg = "width " + std::to_string(w) + " > 100";

		out.push_back({ row, 100, "1.1", msg });
	}
}

// §1.2 들여쓰기에 공백 (raw 행).
static void Check_indent(std::string const &line, int const row, std::vector<Violation> &out){
	int const n = static_cast<int>(line.size());
	int p = 0;

	while(p < n && line[p] == '\t'){
		++p;
	}

	if(p < n && line[p] == ' '){
		out.push_back({ row, p, "1.2", "space in indentation" });
	}
}

// §8.2 무효/잉여 공백 — 개행 앞 유효 공백은 잉여라 보존한다(§8.2, §11.2 2칸 마커의 토대).
// 행 시작 공백은 §1.2 Check_indent, 4연속+는 §8.1 Check_space_run, 꼬리 탭은 §8.1 Check_tab_use
// 가 각각 담당하므로 §8.2 는 별도 후행-공백 검사를 두지 않는다.

// §8.1 들여쓰기 이외 용도의 탭 (@마스크). 선두 들여쓰기 탭 이후의 탭은 위반.
static void Check_tab_use(std::string const &mask, int const row, std::vector<Violation> &out){
	int const n = static_cast<int>(mask.size());
	int p = 0;

	while(p < n && mask[p] == '\t'){
		++p;
	}

	while(p < n){
		if(mask[p] == '\t'){
			out.push_back({ row, p, "8.1", "tab outside indentation" });
		}

		++p;
	}
}

// §8.1 공백 4칸 이상 연속 (@마스크). 선두 들여쓰기 구역은 §1.2 소관이라 건너뛴다.
static void Check_space_run(std::string const &mask, int const row, std::vector<Violation> &out){
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
			out.push_back({ row, begin, "8.1", "4 or more consecutive spaces" });
		}
	}
}

// 여는 괄호에 대응하는 닫는 괄호.
static auto Closer_of(char const open)->char{
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
// < > 와 [[ ]] 와 다중행 괄호는 제외(에이전트 몫). 빈 괄호는 단계에서 뺀다.
static void Check_inner_space(std::string const &mask, int const row, std::vector<Violation> &out){
	struct Frame{
		char open;
		int col;
		int child;   // 안에 든 같은 종류 비어있지 않은 짝의 최대 단계 (없으면 -1)
	};

	struct Pair{
		char open;
		int from, to;
		int stage;
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

			if( c != ::Closer_of(stack.back().open) ){
				aborted = true;

				break;
			}

			Frame const top = stack.back();
			stack.pop_back();

			if(i > top.col + 1){
				int const stage = top.child < 0 ? 0 : top.child + 1;

				pairs.push_back({ top.open, top.col, i, stage });

				int k = static_cast<int>(stack.size()) - 1;

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
		int const want = pr.open == '{' ? pr.stage + 1 : pr.stage;
		std::string const msg = "inner space must be " + std::to_string(want);

		int after = 0;

		while(pr.from + 1 + after < n && mask[pr.from + 1 + after] == ' '){
			++after;
		}

		int before = 0;

		while(pr.to - 1 - before > pr.from && mask[pr.to - 1 - before] == ' '){
			++before;
		}

		if(after != want){
			::Push_fix(out, { row, pr.from, "8.5", msg }, Fix_kind::gap_right, pr.from + 1, want);
		}

		if(before != want){
			::Push_fix(out, { row, pr.to, "8.5", msg }, Fix_kind::gap_left, pr.to, want);
		}
	}
}

// (row,col)부터 공백·탭·@(주석/문자열)·개행을 건너뛴 다음 코드 문자를 찾는다(최대 max_row 행).
static auto Next_code(Lines const &mask, int const max_row, int &row, int &col)->bool{
	for(; row <= max_row; ++row, col = 0){
		for( int const len = static_cast<int>(mask[row].size()); col < len; ++col ){
			if(mask[row][col] != ' ' && mask[row][col] != '\t' && mask[row][col] != '@'){
				return true;
			}
		}
	}

	return false;
}

// (row,col)의 '(' 에서 짝이 맞는 ')' 를 찾는다(최대 max_row 행). 깊이만 센다.
static auto Match_paren(Lines const &mask, int const max_row, int &row, int &col)->bool{
	for(int depth = 0; row <= max_row; ++row, col = 0){
		for( int const len = static_cast<int>(mask[row].size()); col < len; ++col ){
			if(char const c = mask[row][col]; c == '('){
				++depth;
			} else if(c == ')'){
				if(--depth == 0){
					return true;
				}
			}
		}
	}

	return false;
}

// (row,col) 직전의 의미 토큰 위치를 찾는다(공백·탭·@·개행 건너뜀). 찾으면 true.
static auto Prev_significant(Lines const &mask, int &row, int &col)->bool{
	while(row >= 0){
		for(; col >= 0; --col){
			if(mask[row][col] != ' ' && mask[row][col] != '\t' && mask[row][col] != '@'){
				return true;
			}
		}

		if(--row >= 0){
			col = static_cast<int>(mask[row].size()) - 1;
		}
	}

	return false;
}

// (row,col)의 '}' 에서 짝이 맞는 '{' 를 역방향으로 찾는다. 찾으면 true 와 '{' 위치를 채운다.
static auto Match_brace_back(Lines const &mask, int &row, int &col)->bool{
	for(int depth = 0; row >= 0;){
		for(; col >= 0; --col){
			if(char const ch = mask[row][col]; ch == '}'){
				++depth;
			} else if(ch == '{'){
				if(--depth == 0){
					return true;
				}
			}
		}

		if(--row >= 0){
			col = static_cast<int>(mask[row].size()) - 1;
		}
	}

	return false;
}

// (row,col)의 닫는 괄호 문자(close)에서 짝이 맞는 여는 괄호 문자(open)를 역방향으로
// 찾는다. 찾으면 true 와 open 위치를 (row,col) 에 채운다. `)`↔`(`, `]`↔`[` 에 재사용.
static auto Match_bracket_back(
	Lines const &mask, char const open, char const close, int &row, int &col
)->bool{
	for(int depth = 0; row >= 0;){
		for(; col >= 0; --col){
			if(char const ch = mask[row][col]; ch == close){
				++depth;
			} else if(ch == open){
				if(--depth == 0){
					return true;
				}
			}
		}

		if(--row >= 0){
			col = static_cast<int>(mask[row].size()) - 1;
		}
	}

	return false;
}

// (row,col) 직전의 의미 토큰이 식별자면 그 식별자를, 아니면 빈 문자열을 돌려준다.
static auto Word_before(Lines const &mask, int const row, int const col)->std::string{
	int r = row, c = col - 1;

	if( !::Prev_significant(mask, r, c) || !::is_word_char(mask[r][c]) ){
		return "";
	}

	int s = c;

	while( s >= 0 && ::is_word_char(mask[r][s]) ){
		--s;
	}

	return mask[r].substr(s + 1, c - s);
}

// while(...); 가 do-while 꼬리인지: 직전 '}' 가 'do' 블록을 닫는지 역방향으로 확인.
static auto Is_do_tail(Lines const &mask, int const row, int const col)->bool{
	int r = row, c = col - 1;

	if( !::Prev_significant(mask, r, c) || mask[r][c] != '}' ){
		return false;
	}

	if( !::Match_brace_back(mask, r, c) ){
		return false;
	}

	return ::Word_before(mask, r, c) == "do";
}

// (line,col)에서 시작하는 식별자 런을 돌려준다(식별자가 아니면 빈 문자열).
static auto Word_at(std::string const &line, int const col)->std::string{
	int const len = static_cast<int>(line.size());
	int e = col;

	while( e < len && ::is_word_char(line[e]) ){
		++e;
	}

	return line.substr(col, e - col);
}

// (line,col) 이 식별자 런의 시작인지(왼쪽 경계가 단어문자가 아님).
static auto Word_starts_at(std::string const &line, int const i)->bool{
	return ::is_word_char(line[i]) && ( i == 0 || !::is_word_char(line[i - 1]) );
}

// §3 제어문 중괄호 강제 (@마스크 + 인접 행). 키워드 다음 본문이 '{' 인지 본다.
// if/for/while/switch 는 조건 ')' 다음을, do/else 는 키워드 다음을 본다.
// while(...); 는 직전 '}' 가 do 블록을 닫으면 do-while 꼬리라 합법, 아니면 위반(빈본문 while).
static void Check_ctrl_brace(Lines const &mask, int const row, std::vector<Violation> &out){
	std::string const &line = mask[row];
	int const len = static_cast<int>(line.size()), last = static_cast<int>(mask.size()) - 1;
	int const max_row = row + 4 < last ? row + 4 : last;

	for(int i = 0; i < len;){
		if( !::Word_starts_at(line, i) ){
			++i;

			continue;
		}

		std::string const word = ::Word_at(line, i);
		int const e = i + static_cast<int>(word.size());

		auto const  
			[br, bc, body_found]
			= [row, e, &mask, max_row, &word](bool const has_cond){
				struct{ int _0, _1; bool _2; } res{ row, e, false };
				auto &[br, bc, body_found] = res;

				if(has_cond){
					if(
						int pr = row, pc = e;
						::Next_code(mask, max_row, pr, pc) && mask[pr][pc] == '('
					){
						if( ::Match_paren(mask, max_row, pr, pc) ){
							br = pr;
							bc = pc + 1;
							body_found = ::Next_code(mask, max_row, br, bc);
						}
					}
				} else if(word == "do" || word == "else"){
					body_found = ::Next_code(mask, max_row, br, bc);
				}

				return res;
			}(word == "if" || word == "for" || word == "while" || word == "switch")
		;

		if(body_found){
			char const body = mask[br][bc];

			bool const  
				legal
				= word == "while" && body == ';' ? ::Is_do_tail(mask, row, i)
				: word == "else" && body != '{' ? ::Word_at(mask[br], bc) == "if"
				: body == '{'
			;

			if(!legal){
				out.push_back({ row, i, "3", word + " needs braces" });
			}
		}

		i = e;
	}
}

// §8.4 비기호형↔괄호 경계 공백 (@마스크, 같은 행).
// 단어 다음 여는괄호 ((·[·{) 사이의 공백 = 위반.
// 닫는괄호 ())·]·}) 다음에 단어가 무공백으로 붙으면 = 위반.
// 꺾쇠 < > 는 비교/꺾쇠 모호성 탓에 sak 보수 영역에서 제외(§5.1·범주 4).
static void Check_word_paren_space(
	std::string const &mask, int const row, std::vector<Violation> &out
){
	int const n = static_cast<int>(mask.size());
	int p = 0;

	while( p < n && (mask[p] == '\t' || mask[p] == ' ') ){
		++p;
	}

	while(p < n){
		char const c = mask[p];

		if( ::is_word_char(c) ){
			int const  
				e
				= [n, &mask](int res){
					for( ; res < n && ::is_word_char(mask[res]); ++res ){}

					return res;
				}(p),
				q
				= [n, &mask](int res){
					for(; res < n && mask[res] == ' '; ++res){}

					return res;
				}(e)
			;

			if( q > e && q < n && (mask[q] == '(' || mask[q] == '[' || mask[q] == '{') ){
				::Push_fix(
					out, { row, e, "8.4", "space between word and opening bracket" },
					Fix_kind::gap_right, e, 0
				);
			}

			p = e;

			continue;
		}

		if( (c == ')' || c == ']' || c == '}') && p + 1 < n && ::is_word_char(mask[p + 1]) ){
			::Push_fix(
				out, { row, p + 1, "8.4", "missing space between closing bracket and word" },
				Fix_kind::gap_left, p + 1, 1
			);
		}

		++p;
	}
}

// row 의 마지막 의미 토큰 열(@마스크). 없으면 -1.
static auto Last_significant_col(std::string const &mask_row)->int{
	int p = static_cast<int>(mask_row.size()) - 1;

	while( p >= 0 && (mask_row[p] == ' ' || mask_row[p] == '\t' || mask_row[p] == '@') ){
		--p;
	}

	return p;
}

// row 의 첫 의미 토큰 열(@마스크). 없으면 -1.
static auto First_significant_col(std::string const &mask_row)->int{
	int const n = static_cast<int>(mask_row.size());
	int p = 0;

	while( p < n && (mask_row[p] == ' ' || mask_row[p] == '\t' || mask_row[p] == '@') ){
		++p;
	}

	return p < n ? p : -1;
}

// §9.3 비기호형 토큰의 개행 제한 — 닫는 ')' + 단어 다음 행 형태만 (@마스크).
// row 의 마지막 의미 토큰이 ')' 이고 *다음 행*(공행 건너지 않음) 첫 의미 토큰이 단어면 위반.
// 그 외 형태(단어 + 단어, 단어 + 여는괄호, '}' + 단어, 공행으로 분리된 두 토큰,
//   가상괄호 발현 자리 등)는 §5.5 변수선언/using/return 등의 다중행 합법 발현과
//   공행으로 단락이 갈린 정상 케이스를 가르려면 의미 해석이 필요해 sak 보수 영역 밖이다.
static void Check_word_paren_newline(
	Lines const &mask, int const row, std::vector<Violation> &out
){
	int const n_rows = static_cast<int>(mask.size());

	if(row + 1 >= n_rows){
		return;
	}

	int const l = ::Last_significant_col(mask[row]);

	if(l < 0 || mask[row][l] != ')'){
		return;
	}

	int const nc = ::First_significant_col(mask[row + 1]);

	if( nc < 0 || !::is_word_char(mask[row + 1][nc]) ){
		return;
	}

	out.push_back({ row + 1, nc, "9.3", "newline between closing ')' and word" });
}

// 선두 탭 개수 = 들여쓰기 깊이.
static auto Indent_depth(std::string const &line)->int{
	int const n = static_cast<int>(line.size());
	int p = 0;

	while(p < n && line[p] == '\t'){
		++p;
	}

	return p;
}

// 다중행 괄호 매처 (§5.4·§5.7).
// 짝지은 괄호 한 쌍의 여닫는 위치와 종류. kind = '(', '{', '[', 'A'([[ ]]).
struct Bk_pair{
	int o_row, o_col, o_len;
	int c_row, c_col, c_len;
	char kind;
};

// @마스크 전체를 스캔해 ()/{}/[]/[[ ]] 짝을 모은다. 짝이 깨지면 그 자리는 버린다.
// 단일행 빈 괄호(())/{}/[]/[[]]는 §5.1에 따라 괄호 표현이 아니므로 제외한다.
// (다중행 빈 괄호는 거의 없으니 보수적으로 통과시킨다.)
static auto Match_brackets(Lines const &mask)->std::vector<Bk_pair>{
	std::vector<Bk_pair> out;

	struct Frame{
		int r, c, len;
		char kind;
	};

	std::vector<Frame> st;
	int const rows = static_cast<int>(mask.size());

	for(int r = 0; r < rows; ++r){
		std::string const &line = mask[r];
		int const n = static_cast<int>(line.size());
		int c = 0;

		while(c < n){
			char const ch = line[c];

			if(ch == '[' && c + 1 < n && line[c + 1] == '['){
				st.push_back({ r, c, 2, 'A' });
				c += 2;

				continue;
			}

			if(
				ch == ']' && c + 1 < n && line[c + 1] == ']'
				&& !st.empty() && st.back().kind == 'A'
			){
				Frame const f = st.back();
				st.pop_back();

				bool const same_line = f.r == r;
				bool empty_pair = false;

				if(same_line){
					empty_pair = true;

					for(int cc = f.c + f.len; cc < c; ++cc){
						char const ic = line[cc];

						if(ic != ' ' && ic != '\t' && ic != '@'){
							empty_pair = false;

							break;
						}
					}
				}

				if(!empty_pair){
					out.push_back({ f.r, f.c, f.len, r, c, 2, 'A' });
				}

				c += 2;

				continue;
			}

			if(ch == '(' || ch == '{' || ch == '['){
				st.push_back({ r, c, 1, ch });
				++c;

				continue;
			}

			if(ch == ')' || ch == '}' || ch == ']'){
				char const want = ch == ')' ? '(' : ch == '}' ? '{' : '[';

				if(!st.empty() && st.back().kind == want){
					Frame const f = st.back();
					st.pop_back();

					bool const same_line = f.r == r;
					bool empty_pair = false;

					if(same_line){
						empty_pair = true;

						for(int cc = f.c + f.len; cc < c; ++cc){
							char const ic = line[cc];

							if(ic != ' ' && ic != '\t' && ic != '@'){
								empty_pair = false;

								break;
							}
						}
					}

					if(!empty_pair){
						out.push_back({ f.r, f.c, f.len, r, c, 1, want });
					}
				}

				++c;

				continue;
			}

			++c;
		}
	}

	return out;
}

// 마스크상 코드 토큰을 하나라도 포함하는 행인지(전처리기·주석·문자열 only 는 false).
static auto Has_code(std::string const &mask_row)->bool{
	for(char const c : mask_row){
		if(c != ' ' && c != '\t' && c != '@'){
			return true;
		}
	}

	return false;
}

// §9.4 공행 기본 유효성. (§9.2 종속의 강제 공행은 범주 4 → 검사 외.)
// 길이 0 의 공행은: 파일 경계가 아니어야 하고, 위·아래 인접 행도 공행이 아니어야 한다.
// 들여쓰기 비교는 인접 행이 "코드 토큰을 포함하는 행"일 때만 의미가 있다.
// 인접 행이 전처리 본문(§2 제외 — `\` 연장 행 포함)·주석 only·문자열 only 면
// 그 행의 raw 들여쓰기는 spec 상 들여쓰기가 아니므로 비교 대상에서 뺀다.
static void Check_blank_line(
	Lines const &lines, Lines const &mask, int const row, std::vector<Violation> &out
){
	if(!lines[row].empty()){
		return;
	}

	int const last = static_cast<int>(lines.size()) - 1;

	if(row == 0 || row == last){
		out.push_back({ row, 0, "9.4", "blank line at file boundary" });

		return;
	}

	if(lines[row - 1].empty() || lines[row + 1].empty()){
		out.push_back({ row, 0, "9.4", "consecutive blank lines" });

		return;
	}

	if( !::Has_code(mask[row - 1]) || !::Has_code(mask[row + 1]) ){
		return;
	}

	if( ::Indent_depth(lines[row - 1]) != ::Indent_depth(lines[row + 1]) ){
		out.push_back({ row, 0, "9.4", "neighbors differ in indentation" });
	}
}

// 마스크 행의 첫 코드 문자.
static auto First_code_char(std::string const &mask_line)->char{
	int const n = static_cast<int>(mask_line.size());

	for(int i = 0; i < n; ++i){
		char const c = mask_line[i];

		if(c != ' ' && c != '\t' && c != '@'){
			return c;
		}
	}

	return '\0';
}

// 마스크 행의 마지막 코드 문자.
static auto Last_code_char(std::string const &mask_line)->char{
	for( int i = static_cast<int>(mask_line.size()) - 1; i >= 0; --i ){
		char const c = mask_line[i];

		if(c != ' ' && c != '\t' && c != '@'){
			return c;
		}
	}

	return '\0';
}

// 마스크 행이 특정 키워드로 시작하는지(그 뒤가 단어경계).
static auto Starts_with_keyword(std::string const &mask_line, char const *kw)->bool{
	int const n = static_cast<int>(mask_line.size());
	int i = 0;

	while( i < n && (mask_line[i] == ' ' || mask_line[i] == '\t' || mask_line[i] == '@') ){
		++i;
	}

	int const kl = static_cast<int>( std::string(kw).size() );

	if(i + kl > n){
		return false;
	}

	if( mask_line.compare(i, kl, kw) != 0 ){
		return false;
	}

	int const nxt = i + kl;

	return nxt >= n || !::is_word_char(mask_line[nxt]);
}

// 이항·삼항 연산자·구두점으로 시작하는 행 — 이전 표현식의 연장.
static auto Starts_with_continuation_op(std::string const &mask_line)->bool{
	char const c = ::First_code_char(mask_line);

	return
		c == '=' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%'
		|| c == '&' || c == '|' || c == '^' || c == '<' || c == '>' || c == '!'
		|| c == '~' || c == '?' || c == ':' || c == ',' || c == '.'
	;
}

// 닫는 괄호 뒤 같은 행에 세미콜론 아닌 코드 토큰이 남아 있는지(표현식 종속의 증거).
// `}(p),` `}.method()` `}` +쉼표 같은 자리는 문장 종결이 아니므로 아래쪽 공행 요구 유보.
static auto Has_nonsemi_code_after(std::string const &mask_line, int const pos)->bool{
	int const n = static_cast<int>(mask_line.size());

	for(int i = pos; i < n; ++i){
		char const c = mask_line[i];

		if(c == ' ' || c == '\t' || c == '@'){
			continue;
		}

		if(c == ';'){
			return false;
		}

		return true;
	}

	return false;
}

// §9.4 확장 — 다중행 괄호(세미콜론 제외)가 형성한 개행 경쟁 범위(여는 행·닫는 행)의
// 바로 위·아래 인접행은 공행이어야 한다. 단 그 자리에 공행을 두어도 §9.4 유효성
// (인접 두 코드행의 들여쓰기 일치)이 성립할 때만 요구한다.
// 판정 기준 — "이 자리가 실제 문장 경계인가":
//   위쪽 — 인접 위 행의 마지막 코드 문자가 `;` 또는 `}` 여야 하며(문장 종결),
//   여는 행이 이항 연산자로 시작하거나 `else`/`catch` 로 시작하면 이전 문장의 연장이라 유보.
//   아래쪽 — 닫는 행에 `;` 아닌 코드가 남거나(표현식 종속),
//   인접 아래 행이 이항 연산자·`else`/`catch`/`while` 로 시작하면 유보.
static void Check_bracket_blank_line(
	Lines const &lines, Lines const &mask, Bk_pair const &p, std::vector<Violation> &out
){
	if(p.o_row == p.c_row){
		return;
	}

	int const rows = static_cast<int>(lines.size());
	int const o_ind = ::Indent_depth(lines[p.o_row]);
	int const c_ind = ::Indent_depth(lines[p.c_row]);

	if(p.o_row > 0){
		int const nr = p.o_row - 1;
		std::string const &above = mask[nr];

		if( !lines[nr].empty() && ::Has_code(above) && ::Indent_depth(lines[nr]) == o_ind ){
			char const above_last = ::Last_code_char(above);

			bool const  
				at_stmt_boundary
				= (above_last == ';' || above_last == '}')
				&& !::Starts_with_continuation_op(mask[p.o_row])
				&& !::Starts_with_keyword(mask[p.o_row], "else")
				&& !::Starts_with_keyword(mask[p.o_row], "catch")
			;

			if(at_stmt_boundary){
				out.push_back({ p.o_row, 0, "9.4", "missing blank line above multi-line bracket" });
			}
		}
	}

	if(p.c_row + 1 < rows){
		int const nr = p.c_row + 1;
		std::string const &below = mask[nr];

		if( !lines[nr].empty() && ::Has_code(below) && ::Indent_depth(lines[nr]) == c_ind ){
			bool const  
				at_stmt_boundary
				= !::Has_nonsemi_code_after(mask[p.c_row], p.c_col + p.c_len)
				&& !::Starts_with_continuation_op(below)
				&& !::Starts_with_keyword(below, "else")
				&& !::Starts_with_keyword(below, "catch")
				&& !::Starts_with_keyword(below, "while")
			;

			if(at_stmt_boundary){
				out.push_back({ p.c_row, 0, "9.4", "missing blank line below multi-line bracket" });
			}
		}
	}
}

// 문맥 불변 토큰 분류 (§8.3 단계 2 — 분류가 모양만으로 결정되는 것만).
// 문맥 의존 토큰(* & + - < > : && ! ~ 등)과 분류 모호(<< >> ::)는 모두 skip.
enum class Tk_cls{
	skip,
	word,
	open_b, close_b,
	sep, // ; ,
	bin_ns, // . -> .* ->* (양쪽 무공백)
	bin_s, // = == != <= >= <=> || ? / % | ^ 와 모든 복합대입 (양쪽 공백)
	inc_dec, // ++ --
};

struct Tok_8_3{
	int col, len;
	Tk_cls cls;
};

// @마스크 한 행을 토큰열로 변환. 더 긴 모양 우선 매칭, 의심 자리는 모두 skip.
static auto Tokenize_8_3(std::string const &mask)->std::vector<Tok_8_3>{
	std::vector<Tok_8_3> out;
	int const n = static_cast<int>(mask.size());
	int i = 0;

	while(i < n){
		char const c = mask[i];

		if(c == ' ' || c == '\t' || c == '@'){
			++i;

			continue;
		}

		if( ::is_word_char(c) ){
			int const s = i;

			while( i < n && ::is_word_char(mask[i]) ){
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

// §8.3 문맥 불변 토큰의 단일행 공백 검사 (@마스크).
// 그룹별 규칙:
//   sep `;` `,`     — 앞 공백 0, 뒤 공백 ≥ 1 (다음 토큰이 close_b·sep이면 skip).
//   bin_ns . -> .* ->*  — 양쪽 공백 0. 단 좌/우 토큰이 같은 행에 없거나 분류가
//                          word/괄호가 아니면 그 쪽은 검사 제외(가상괄호·다중행 영역 양보).
//   bin_s 양쪽 공백 토큰 — 양쪽 공백 ≥ 1. 좌/우가 word·close_b/open_b 가 아닌
//                          기호형이면 그 쪽 검사 제외(연쇄·단항 영역 양보).
//   inc_dec ++ --    — 한 쪽은 0(피연산자 부착). 양쪽 모두 공백 > 0 이면 위반.
// 괄호 경계 투명성(§5.3): 단일행 괄호 안 첫·마지막 토큰은 감싸는 괄호를 인접 토큰으로 보지 않음.
static void Check_token_space(
	std::string const &mask, int const row, std::vector<Violation> &out
){
	auto const toks = ::Tokenize_8_3(mask);
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
		// 오버로드 함수명의 일부이므로 §8.3 검사 영역 밖이다.
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
				// `for(init;;++itr2)` 처럼 `;` 두 개가 연속하면 그 사이는 공백 1 필수
				// (spec §8.3 SEP 항 예외). `,` 끼리·`,`+`;` 혼합은 일반 SEP 룰을 그대로 따른다.
				bool const  
					semi_chain
					= t.len == 1 && mask[t.col] == ';'
					&& has_l && toks[i - 1].cls == Tk_cls::sep
					&& toks[i - 1].len == 1 && mask[ toks[i - 1].col ] == ';'
				;

				if( eff_l && !semi_chain && gap_before(i) > 0 ){
					::Push_fix(
						out, { row, t.col, "8.3", "no space before separator" },
						Fix_kind::gap_left, t.col, 0
					);
				}

				if( eff_l && semi_chain && gap_before(i) == 0 ){
					::Push_fix(
						out, { row, t.col, "8.3", "space required between consecutive ';'" },
						Fix_kind::gap_left, t.col, 1
					);
				}

				if( eff_r && right_cls != Tk_cls::sep && gap_after(i) == 0 ){
					::Push_fix(
						out, { row, t.col + t.len, "8.3", "space required after separator" },
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
				::Push_fix(
					out, { row, t.col, "8.3", "no space before '.','->','.*','->*'" },
					Fix_kind::gap_left, t.col, 0
				);
			}

			if( eff_r && r_operand && gap_after(i) > 0 ){
				::Push_fix(
					out, { row, t.col + t.len, "8.3", "no space after '.','->','.*','->*'" },
					Fix_kind::gap_right, t.col + t.len, 0
				);
			}

			break;
		case Tk_cls::bin_s:
			if( eff_l && l_operand && gap_before(i) == 0 ){
				::Push_fix(
					out, { row, t.col, "8.3", "space required before binary operator" },
					Fix_kind::gap_left, t.col, 1
				);
			}

			if( eff_r && r_operand && gap_after(i) == 0 ){
				::Push_fix(
					out, { row, t.col + t.len, "8.3", "space required after binary operator" },
					Fix_kind::gap_right, t.col + t.len, 1
				);
			}

			break;
		case Tk_cls::inc_dec:
			if( eff_l && eff_r && gap_before(i) > 0 && gap_after(i) > 0 ){
				out.push_back({ row, t.col, "8.3", "'++'/'--' must attach to operand" });
			}

			break;
		default:
			break;
		}
	}
}

// 마스크 한 행에서 col 위치에 코드 토큰이 있는지(공백·탭·@가 아니면 코드).
static auto Is_code_char(char const ch)->bool{
	return ch != ' ' && ch != '\t' && ch != '@';
}

// §5.4 다중행 괄호의 위치·들여쓰기 검사.
// (1) 여닫는 행 들여쓰기 동일, (2) 여는 괄호 다음 같은 행에 코드 토큰 없음(행 끝),
// (3) 닫는 괄호 직전 같은 행에 코드 토큰 없음(행 처음), (4) 중간 코드 행의 들여쓰기 ≥ 외곽+1.
static void Check_multiline_bracket(
	Lines const &lines, Lines const &mask, Bk_pair const &p,
	std::vector<Violation> &out
){
	if(p.o_row == p.c_row){
		return;
	}

	// (1) 여닫는 행 들여쓰기 동일
	int const o_ind = ::Indent_depth(lines[p.o_row]), c_ind = ::Indent_depth(lines[p.c_row]);

	if(o_ind != c_ind){
		::Push_fix(
			out, { p.c_row, 0, "5.4", "open/close indent differ" },
			Fix_kind::indent, 0, o_ind
		);
	}

	// (2) 여는 괄호가 행 마지막 코드 토큰인지
	std::string const &o_line = mask[p.o_row];
	int const o_n = static_cast<int>(o_line.size()), o_end = p.o_col + p.o_len;

	for(int cc = o_end; cc < o_n; ++cc){
		if( ::Is_code_char(o_line[cc]) ){
			out.push_back({ p.o_row, cc, "5.4", "opening bracket not last token" });

			break;
		}
	}

	// (3) 닫는 괄호가 행 첫 코드 토큰인지
	std::string const &c_line = mask[p.c_row];

	for(int cc = 0; cc < p.c_col; ++cc){
		if( ::Is_code_char(c_line[cc]) ){
			out.push_back({ p.c_row, cc, "5.4", "closing bracket not first token" });

			break;
		}
	}

	// (4) 중간 코드 행 들여쓰기 ≥ 외곽+1.
	// 단 §5.6 숨은 중괄호로 닫혔다 열리는 자리 — case/default/public/private/protected —
	// 는 외곽과 같은 들여쓰기가 정상이므로 검사 제외한다.
	for(int r = p.o_row + 1; r < p.c_row; ++r){
		if( lines[r].empty() || !::Has_code(mask[r]) ){
			continue;
		}

		int const ind = ::Indent_depth(lines[r]);

		std::string const &m = mask[r];
		int first = 0;
		int const m_n = static_cast<int>(m.size());

		while( first < m_n && (m[first] == ' ' || m[first] == '\t' || m[first] == '@') ){
			++first;
		}

		std::string const head = first < m_n ? ::Word_at(m, first) : "";

		bool const  
			hidden_close
			= head == "case" || head == "default"
			|| head == "public" || head == "private" || head == "protected"
		;

		// §5.6 숨은 중괄호 자리(case/default/접근지정자)는 +1 룰을 적용하지 않는다.
		// 들여쓰기는 가장 안쪽 brace 와 비교해야 정확하므로 Check_hidden_brace 가 다룬다.
		if(hidden_close){
			continue;
		}

		// 첫 코드 토큰이 닫는 괄호이면 내부 짝의 close 라인이라 외곽 middle 검사에서 빠진다
		// (자기 짝의 §5.4 검사에서 다룬다).
		char const first_c = first < m_n ? m[first] : '\0';

		if(first_c == ')' || first_c == ']' || first_c == '}'){
			continue;
		}

		if(ind < o_ind + 1){
			::Push_fix(
				out, { r, 0, "5.4", "middle line indent insufficient" },
				Fix_kind::indent, 0, o_ind + 1
			);
		}
	}
}

// §5.6 숨은 중괄호 — case/default/접근지정자(public/private/protected) 라인은
// 그 라인을 *직접 둘러싼 가장 안쪽 `{ }` brace* 와 같은 들여쓰기여야 한다.
// 외곽 함수 본체나 namespace 본체와의 들여쓰기는 비교 대상이 아니다.
static void Check_hidden_brace(
	Lines const &lines, Lines const &mask,
	std::vector<Bk_pair> const &pairs, std::vector<Violation> &out
){
	int const rows = static_cast<int>(lines.size());

	for(int r = 0; r < rows; ++r){
		if( lines[r].empty() || !::Has_code(mask[r]) ){
			continue;
		}

		std::string const &m = mask[r];
		int first = 0;
		int const m_n = static_cast<int>(m.size());

		while( first < m_n && (m[first] == ' ' || m[first] == '\t' || m[first] == '@') ){
			++first;
		}

		if(first >= m_n){
			continue;
		}

		std::string const head = ::Word_at(m, first);

		bool const  
			is_case_or_default
			= head == "case" || head == "default"
		;

		bool const  
			is_access_keyword
			= head == "public" || head == "private" || head == "protected"
		;

		if(!is_case_or_default && !is_access_keyword){
			continue;
		}

		// 접근지시자는 `public`/`private`/`protected` 바로 뒤 (공백 skip) 가 `:` 여야 한다.
		// 아니면 상속 리스트의 `public Base` 등 다른 문맥이라 §5.6 대상 아님.
		if(is_access_keyword){
			int p = first + static_cast<int>(head.size());

			while( p < m_n && (m[p] == ' ' || m[p] == '\t' || m[p] == '@') ){
				++p;
			}

			if(p >= m_n || m[p] != ':'){
				continue;
			}
		}

		Bk_pair const *inner = nullptr;

		for(Bk_pair const &p : pairs){
			if(p.kind != '{' || p.o_row >= r || p.c_row <= r){
				continue;
			}

			if(!inner || p.o_row > inner->o_row){
				inner = &p;
			}
		}

		if(!inner){
			continue;
		}

		int const r_ind = ::Indent_depth(lines[r]), o_ind = ::Indent_depth(lines[inner->o_row]);

		if(r_ind != o_ind){
			::Push_fix(
				out, { r, 0, "5.6", "hidden close: indent must equal enclosing brace" },
				Fix_kind::indent, 0, o_ind
			);
		}
	}
}

// §5.7 다중행 [[ ]] 의 닫는 ']]' 가 행 마지막 코드 토큰인지.
static void Check_attribute_close(
	Lines const &mask, Bk_pair const &p, std::vector<Violation> &out
){
	if(p.kind != 'A' || p.o_row == p.c_row){
		return;
	}

	std::string const &c_line = mask[p.c_row];
	int const c_n = static_cast<int>(c_line.size()), c_end = p.c_col + p.c_len;

	for(int cc = c_end; cc < c_n; ++cc){
		if( ::Is_code_char(c_line[cc]) ){
			out.push_back({ p.c_row, cc, "5.7", "']]' not last token" });

			break;
		}
	}
}

// §5.5 가상괄호 — return/throw/using 앵커 (같은 구조: 여는 키워드 ~ 닫는 ';').
// 세 조건 검사: (a) 여는 키워드가 행 마지막 코드 토큰인지, (b) 짝 ';' 가 그 행 첫 코드
// 토큰인지, (c) 다음 코드 행 들여쓰기 ≥ cur+1 (내용 +1). 세 키워드는 정본 §5.5 표에서
// 동일 구조(open=keyword, close=';')이므로 단일 매처로 통합. 그 외 앵커(`->`·`case`·
// 변수선언·`:`)는 의미적 판정이 더 필요해 별도.
static void Check_anchor_keyword_semicolon(
	Lines const &lines, Lines const &mask, std::vector<Violation> &out
){
	int const rows = static_cast<int>(lines.size());

	for(int r = 0; r < rows; ++r){
		std::string const &m = mask[r];
		int const n = static_cast<int>(m.size());
		int c = 0;

		while(c < n){
			if( !::Word_starts_at(m, c) ){
				++c;

				continue;
			}

			std::string const w = ::Word_at(m, c);
			int const e = c + static_cast<int>(w.size());

			if(w != "return" && w != "throw" && w != "using"){
				c = e;

				continue;
			}

			// (r, e) 부터 행을 넘어가며 깊이 추적으로 짝 ; 를 찾는다.
			int depth = 0;
			int close_row = -1, close_col = -1;

			for(int rr = r; rr < rows && close_row < 0; ++rr){
				std::string const &cm = mask[rr];
				int const cn = static_cast<int>(cm.size());
				int const start = rr == r ? e : 0;

				for(int cc = start; cc < cn; ++cc){
					char const ch = cm[cc];

					if(ch == '(' || ch == '[' || ch == '{'){
						++depth;
					} else if(ch == ')' || ch == ']' || ch == '}'){
						--depth;
					} else if(ch == ';' && depth == 0){
						close_row = rr;
						close_col = cc;

						break;
					}
				}
			}

			// 같은 행에서 ; 를 찾았으면 단일행 return/throw — 가상괄호 미발현.
			if(close_row == r){
				c = e;

				continue;
			}

			// (a) 여는 키워드는 행 마지막 코드 토큰이어야 한다.
			bool keyword_last = true;

			for(int cc = e; cc < n; ++cc){
				if( ::Is_code_char(m[cc]) ){
					keyword_last = false;

					out.push_back({ r, cc, "5.5", "return/throw/using: keyword not last" });

					break;
				}
			}

			if(close_row >= 0){
				std::string const &cm = mask[close_row];

				for(int cc = 0; cc < close_col; ++cc){
					if( ::Is_code_char(cm[cc]) ){
						out.push_back(
							{ close_row, cc, "5.5", "return/throw/using: ';' not first" }
						);

						break;
					}
				}
			}

			// 여는 키워드가 행 마지막일 때에만 다음 행 들여쓰기를 검사한다.
			// (키워드 뒤에 표현식이 이어지는 형태는 위 (a) 가 이미 잡았고,
			//  그 경우 "다음 코드 행"이 가상괄호 내용의 첫 행이 아니라
			//  중간 괄호의 닫는 행이 될 수 있어 위양성 위험.)
			if(!keyword_last){
				c = e;

				continue;
			}

			int const cur = ::Indent_depth(lines[r]);
			int nr = r + 1;

			while(nr < rows){
				if( !lines[nr].empty() && ::Has_code(mask[nr]) ){
					break;
				}

				++nr;
			}

			if(nr >= rows){
				c = e;

				continue;
			}

			if( ::Indent_depth(lines[nr]) < cur + 1 ){
				::Push_fix(
					out, { nr, 0, "5.5", "virtual bracket: continuation underindented" },
					Fix_kind::indent, 0, cur + 1
				);
			}

			c = e;
		}
	}
}

// 다음 코드 행을 찾고, 그 들여쓰기 ≥ cur+1 이 아니면 §5.5 위반으로 기록.
static void Push_anchor_indent_check(
	Lines const &lines, Lines const &mask, int const cur_row, std::vector<Violation> &out
){
	int const rows = static_cast<int>(lines.size());
	int nr = cur_row + 1;

	while(nr < rows){
		if( !lines[nr].empty() && ::Has_code(mask[nr]) ){
			break;
		}

		++nr;
	}

	if(nr >= rows){
		return;
	}

	int const cur = ::Indent_depth(lines[cur_row]);

	if( ::Indent_depth(lines[nr]) < cur + 1 ){
		::Push_fix(
			out, { nr, 0, "5.5", "virtual bracket: continuation underindented" },
			Fix_kind::indent, 0, cur + 1
		);
	}
}

// §5.5 후행반환 `->` 가상괄호.
// `->` 직전 비공백이 `)` 이면 후행반환 자리로 본다. 같은 행에 `{` 또는 `;` 가 (괄호 깊이 0)
// 없으면 다중행 가상괄호 발현 — 다음 코드 행 들여쓰기 ≥ `->` 행 +1.
static void Check_anchor_trailing_return(
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

			int p = c - 1;

			while( p >= 0 && (m[p] == ' ' || m[p] == '\t' || m[p] == '@') ){
				--p;
			}

			bool trailing = false;

			if(p >= 0){
				trailing = m[p] == ')';
			} else{
				// `->` 가 행 시작 — 이전 코드 행 마지막 비공백 토큰을 본다.
				int pr = r - 1;

				while(pr >= 0){
					std::string const &pm = mask[pr];
					int pc = static_cast<int>(pm.size()) - 1;

					while( pc >= 0 && (pm[pc] == ' ' || pm[pc] == '\t' || pm[pc] == '@') ){
						--pc;
					}

					if(pc >= 0){
						trailing = pm[pc] == ')';

						break;
					}

					--pr;
				}
			}

			if(!trailing){
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
				} else if(ch == ')' || ch == ']' || ch == '}'){
					--depth;
				} else if(ch == ';' && depth == 0){
					found = true;

					break;
				}

				++cc;
			}

			if(found){
				continue;
			}

			::Push_anchor_indent_check(lines, mask, r, out);
		}
	}
}

// `}` 의 짝 `{` 직전(혹은 그 직전 식별자 직전)이 struct/class/union/enum 인지 확인.
static auto Is_inline_type_close(Lines const &mask, Bk_pair const &p)->bool{
	if(p.kind != '{'){
		return false;
	}

	std::string const &m_o = mask[p.o_row];
	int q = p.o_col - 1;

	while( q >= 0 && (m_o[q] == ' ' || m_o[q] == '\t' || m_o[q] == '@') ){
		--q;
	}

	if( q < 0 || !::is_word_char(m_o[q]) ){
		return false;
	}

	int s = q;

	while( s >= 0 && ::is_word_char(m_o[s]) ){
		--s;
	}

	std::string const w = m_o.substr(s + 1, q - s);

	if(w == "struct" || w == "class" || w == "union" || w == "enum"){
		return true;
	}

	int q2 = s;

	while( q2 >= 0 && (m_o[q2] == ' ' || m_o[q2] == '\t' || m_o[q2] == '@') ){
		--q2;
	}

	if( q2 < 0 || !::is_word_char(m_o[q2]) ){
		return false;
	}

	int s2 = q2;

	while( s2 >= 0 && ::is_word_char(m_o[s2]) ){
		--s2;
	}

	std::string const w2 = m_o.substr(s2 + 1, q2 - s2);

	return w2 == "struct" || w2 == "class" || w2 == "union" || w2 == "enum";
}

// §5.5 인라인 타입 정의 가상괄호 — `struct{…}『var…』;` 패턴.
// 매처 결과의 `{ }` 짝 중 인라인 타입 정의의 close 인 것을 골라, close `}` 가 행 마지막이고
// 다음 코드 행이 식별자로 시작하면 다중행 가상괄호 발현 — 다음 코드 행 들여쓰기 ≥ `}` 행 +1.
static void Check_anchor_inline_type(
	Lines const &lines, Lines const &mask,
	std::vector<Bk_pair> const &pairs, std::vector<Violation> &out
){
	int const rows = static_cast<int>(lines.size());

	for(Bk_pair const &p : pairs){
		if( !::Is_inline_type_close(mask, p) ){
			continue;
		}

		std::string const &m_c = mask[p.c_row];
		int const end = p.c_col + p.c_len, c_n = static_cast<int>(m_c.size());
		bool last_token = true;

		for(int cc = end; cc < c_n; ++cc){
			if( ::Is_code_char(m_c[cc]) ){
				last_token = false;

				break;
			}
		}

		if(!last_token){
			continue;
		}

		int nr = p.c_row + 1;

		while(nr < rows){
			if( !lines[nr].empty() && ::Has_code(mask[nr]) ){
				break;
			}

			++nr;
		}

		if(nr >= rows){
			continue;
		}

		std::string const &m_n = mask[nr];
		int first = 0;
		int const n_n = static_cast<int>(m_n.size());

		while( first < n_n && (m_n[first] == ' ' || m_n[first] == '\t' || m_n[first] == '@') ){
			++first;
		}

		if( first >= n_n || !::is_word_char(m_n[first]) ){
			continue;
		}

		int const cur = ::Indent_depth(lines[p.c_row]);

		if( ::Indent_depth(lines[nr]) < cur + 1 ){
			::Push_fix(
				out, { nr, 0, "5.5", "inline type: var-list underindented" },
				Fix_kind::indent, 0, cur + 1
			);
		}
	}
}

// §5.5 case 라벨 가상괄호.
// `case` 단어 같은 행에 (괄호 깊이 0, `::` 제외) `:` 없으면 다중행 발현 —
// 다음 코드 행 들여쓰기 ≥ `case` 행 +1.
static void Check_anchor_case(
	Lines const &lines, Lines const &mask, std::vector<Violation> &out
){
	int const rows = static_cast<int>(lines.size());

	for(int r = 0; r < rows; ++r){
		std::string const &m = mask[r];
		int const n = static_cast<int>(m.size());
		int c = 0;

		while(c < n){
			if( !::Word_starts_at(m, c) ){
				++c;

				continue;
			}

			std::string const w = ::Word_at(m, c);
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
				} else if(ch == ')' || ch == ']' || ch == '}'){
					--depth;
				} else if(ch == ':' && depth == 0){
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
				::Push_anchor_indent_check(lines, mask, r, out);
			}

			c = e;
		}
	}
}

// §5.5 콜론 가상괄호 — 상속·enum 기반 타입·생성자 init list 세 자리 공통 레이아웃 검사.
// 앵커 ':' 위치를 받아, 짝지어질 body '{' 를 전방 스캔으로 찾고, 다중행이면 세 조건 검사:
//   (a) ':' 이 여는 행 마지막 코드 토큰인지
//   (b) '{' 이 닫는 행 첫 코드 토큰인지
//   (c) 사이 첫 코드 행 들여쓰기 ≥ ind(':') + 1
// ctor init 자리에선 'mem_{val}' 형태의 braced-init 를 body '{' 로 오인하지 않도록,
// '{' 직전 코드 문자가 식별자면 braced-init 로 간주해 짝 '}' 까지 skip.
enum class Colon_vb_kind{ inherit_or_enum, ctor_init };

static void Check_colon_vbracket_layout(
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
			} else if(ch == ')'){
				--p_depth;
			} else if(ch == '['){
				++sq_depth;
			} else if(ch == ']'){
				--sq_depth;
			} else if(ch == '<'){
				++ang_depth;
			} else if(ch == '>' && ang_depth > 0){
				--ang_depth;
			} else if(ch == '{'){
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

					while(rprev >= 0){
						if(cprev < 0){
							--rprev;

							if(rprev < 0){
								break;
							}

							cprev = static_cast<int>(mask[rprev].size()) - 1;

							continue;
						}

						char const pc = mask[rprev][cprev];

						if(pc == ' ' || pc == '\t' || pc == '@'){
							--cprev;

							continue;
						}

						if( ::is_word_char(pc) ){
							is_body = false;
						}

						break;
					}
				}

				if(is_body){
					close_row = rr;
					close_col = cc;

					break;
				}

				++c_depth;
			} else if(ch == '}'){
				if(p_depth > 0 || sq_depth > 0 || ang_depth > 0){
					continue;
				}

				if(c_depth > 0){
					--c_depth;
				}
			} else if(
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
		if( ::Is_code_char(am[cc]) ){
			colon_last = false;

			out.push_back({ a_row, cc, "5.5", "colon vbracket: ':' not last" });

			break;
		}
	}

	// (b) '{' 이 닫는 행 첫 코드 토큰.
	std::string const &cm = mask[close_row];

	for(int cc = 0; cc < close_col; ++cc){
		if( ::Is_code_char(cm[cc]) ){
			out.push_back(
				{ close_row, cc, "5.5", "colon vbracket: '{' not first" }
			);

			break;
		}
	}

	if(!colon_last){
		return;
	}

	// (c) ':' 다음 첫 코드 행 들여쓰기 ≥ ind(':') + 1.
	int const cur = ::Indent_depth(lines[a_row]);
	int nr = a_row + 1;

	while(nr < rows && nr < close_row){
		if( !lines[nr].empty() && ::Has_code(mask[nr]) ){
			break;
		}

		++nr;
	}

	if(nr >= close_row || nr >= rows){
		return;
	}

	if( ::Indent_depth(lines[nr]) < cur + 1 ){
		::Push_fix(
			out, { nr, 0, "5.5", "colon vbracket: content underindented" },
			Fix_kind::indent, 0, cur + 1
		);
	}
}

// §5.5 콜론 가상괄호 스캐너 (A) — 상속·enum 기반 타입.
// class/struct/union/enum 키워드 앵커에서 전방 스캔, (), <>, [] 깊이 추적으로
// depth-0 ':' 이 나오면 앵커 확정. 앞서 '{' 나 ';' 을 만나면 무시 (본체 시작 or 전방 선언).
static void Scan_type_decl_colon(
	Lines const &lines, Lines const &mask, std::vector<Violation> &out
){
	int const rows = static_cast<int>(lines.size());

	for(int r = 0; r < rows; ++r){
		std::string const &m = mask[r];
		int const n = static_cast<int>(m.size());
		int c = 0;

		while(c < n){
			if( !::Word_starts_at(m, c) ){
				++c;

				continue;
			}

			std::string const w = ::Word_at(m, c);
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

				if( pc >= 0 && ::is_word_char(m[pc]) ){
					int ps = pc;

					while( ps > 0 && ::is_word_char(m[ps - 1]) ){
						--ps;
					}

					std::string const prev(m, ps, pc - ps + 1);

					if(prev == "enum"){
						c = e;

						continue;
					}
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
					} else if(ch == ')'){
						--p_depth;
					} else if(ch == '['){
						++sq_depth;
					} else if(ch == ']'){
						--sq_depth;
					} else if(ch == '<'){
						++ang_depth;
					} else if(ch == '>' && ang_depth > 0){
						--ang_depth;
					} else if(p_depth == 0 && sq_depth == 0 && ang_depth == 0){
						if(ch == ';' || ch == '{'){
							stopped = true;

							break;
						} else if(ch == ':'){
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
				::Check_colon_vbracket_layout(
					lines, mask, colon_row, colon_col,
					Colon_vb_kind::inherit_or_enum, out
				);
			}

			c = e;
		}
	}
}

// §5.5 콜론 가상괄호 스캐너 (B) — 생성자 멤버초기화 리스트.
// ':' 좌측 근접 코드 문자가 ')' 이고, 문장 시작부터 여기까지 '?' 스택이 balanced 면 ctor init.
// 문장 시작 = 역방향으로 depth-0 의 ';' / '{' / '}' 를 만난 지점 (또는 파일 시작).
static void Scan_ctor_init_colon(
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

			// 좌측 근접 코드 문자 확인 — ')' 여야 함.
			bool preceded_by_close_paren = false;

			{
				int lr = r, lc = c - 1;

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

					if(pc == ' ' || pc == '\t' || pc == '@'){
						--lc;

						continue;
					}

					if(pc == ')'){
						preceded_by_close_paren = true;
					}

					break;
				}
			}

			if(!preceded_by_close_paren){
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
					} else if(pc == '('){
						--p_d;
					} else if(pc == ']'){
						++s_d;
					} else if(pc == '['){
						--s_d;
					} else if(p_d == 0 && s_d == 0){
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
						} else if(sc == ')'){
							--p_d;
						} else if(sc == '['){
							++s_d;
						} else if(sc == ']'){
							--s_d;
						} else if(p_d == 0 && s_d == 0){
							if(sc == '?'){
								++q_count;
							} else if(sc == ':'){
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

			::Check_colon_vbracket_layout(
				lines, mask, r, c, Colon_vb_kind::ctor_init, out
			);
		}
	}
}

// §5.5 콜론 가상괄호 — 두 스캐너 병치 진입점.
static void Check_anchor_colon_vbracket(
	Lines const &lines, Lines const &mask, std::vector<Violation> &out
){
	::Scan_type_decl_colon(lines, mask, out);
	::Scan_ctor_init_colon(lines, mask, out);
}

// §5.5 변수 선언문 가상괄호 — §11.2 권장 2칸 마커로 반자동 감지.
// 마지막 top-notorious(vexing parse)라 파서 없이는 잡기 어려운 자리를, 사용자가 놓은
// "타입 끝·개행 직전 잉여공백 2칸"(§11.2)을 신뢰하고 닫는 `;` 로 재검증해 잡는다.
// 판정: 어떤 행이 (마스크 기준) 정확히 공백 2칸으로 끝나고, 그 앞 마지막 코드 문자가 타입
// 표현의 꼬리로 볼 수 있으면(‹`; { } ) ] , ( [`› 아님) 여는 가상괄호 후보. 후보부터 통합
// 깊이 추적으로 depth-0 `;`(중첩 `()[]{}`·람다 본문 skip)을 찾으면 변수선언 가상괄호로 확정.
// 확정 시 §5.5 레이아웃을 검사한다: (a) 타입이 마커 행 마지막 — 마커가 보장하므로 생략,
// (b) `;` 이 닫는 행 첫 코드 토큰, (c) 이음줄 들여쓰기 ≥ 마커 행 +1. 미확정이면 조용히 넘긴다
// (무해한 거짓음성 → 서브에이전트 폴백). 위양성 0 계약은 사용자 표식을 신뢰하는 형태로 지킨다.
static void Check_anchor_var_decl_marker(
	Lines const &lines, Lines const &mask, std::vector<Violation> &out
){
	int const rows = static_cast<int>(lines.size());

	for(int r = 0; r < rows; ++r){
		std::string const &m = mask[r];
		int const n = static_cast<int>(m.size());

		// 꼬리 공백 개수 (탭은 §8.1 소관이라 세지 않는다).
		int t = n;

		while(t > 0 && m[t - 1] == ' '){
			--t;
		}

		if(n - t != 2){
			continue;
		}

		// 2칸 바로 앞은 코드 문자여야 하고, 타입 표현의 꼬리로 볼 수 있어야 한다.
		int const p = t - 1;

		if( p < 0 || !::Is_code_char(m[p]) ){
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
		int depth = 0;
		int close_row = -1, close_col = -1;

		for(int rr = r; rr < rows && close_row < 0; ++rr){
			std::string const &cm = mask[rr];
			int const cn = static_cast<int>(cm.size());
			int const start = rr == r ? t : 0;

			for(int cc = start; cc < cn; ++cc){
				char const ch = cm[cc];

				if(ch == '(' || ch == '[' || ch == '{'){
					++depth;
				} else if(ch == ')' || ch == ']' || ch == '}'){
					--depth;
				} else if(ch == ';' && depth == 0){
					close_row = rr;
					close_col = cc;

					break;
				}
			}
		}

		// 짝 `;` 를 못 찾았거나 같은 행이면 변수선언 가상괄호로 확정하지 않는다.
		if(close_row <= r){
			continue;
		}

		// (b) `;` 이 닫는 행 첫 코드 토큰인지.
		std::string const &cm = mask[close_row];

		for(int cc = 0; cc < close_col; ++cc){
			if( ::Is_code_char(cm[cc]) ){
				out.push_back({ close_row, cc, "5.5", "var-decl marker: ';' not first" });

				break;
			}
		}

		// (c) 다음 코드 행 들여쓰기 ≥ 마커 행 +1.
		int const cur = ::Indent_depth(lines[r]);
		int nr = r + 1;

		while(nr < rows){
			if( !lines[nr].empty() && ::Has_code(mask[nr]) ){
				break;
			}

			++nr;
		}

		if(nr >= rows){
			continue;
		}

		if( ::Indent_depth(lines[nr]) < cur + 1 ){
			::Push_fix(
				out, { nr, 0, "5.5", "var-decl marker: continuation underindented" },
				Fix_kind::indent, 0, cur + 1
			);
		}
	}
}

// §5·§8 꺾쇠괄호 매처·검사 — template<...> · *_cast<...> 자리 (문법 확정).
// well-formed C++ 은 template argument list 안의 나체 `<`/`>` 를 template 구분자로만
// 허용한다(연산자로 쓰려면 괄호 필수) → 렉서 수준에서 위양성 0 으로 짝을 잡을 수 있다.
struct Angle_pair{
	int o_row, o_col;
	int c_row, c_col;
};

// Angle_pair 벡터를 위치 기준으로 정렬해 완전히 동일한 중복 쌍을 제거한다(여러 매처의
// 결과를 병합할 때·중첩 template 안 static_cast 가 두 번 잡히는 자리를 정리).
static void Dedup_angles(std::vector<Angle_pair> &v){
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
static auto Match_template_cast_angles(Lines const &mask)->std::vector<Angle_pair>{
	std::vector<Angle_pair> out;
	int const rows = static_cast<int>(mask.size());

	for(int r = 0; r < rows; ++r){
		std::string const &m = mask[r];
		int const n = static_cast<int>(m.size());
		int c = 0;

		while(c < n){
			if( !::Word_starts_at(m, c) ){
				++c;

				continue;
			}

			std::string const w = ::Word_at(m, c);
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

					if( ::is_word_char(ch) ){
						while( p_c < row_n && ::is_word_char(row_m[p_c]) ){
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
			} else{
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
						} else if(cc == close_ch){
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
	::Dedup_angles(out);

	return out;
}

// `>` 뒤 첫 의미 토큰이 "표현식을 시작할 수 없는 토큰"(닫힘 신호)인지 판정한다. 이런
// 자리의 `>` 는 이항 비교/시프트일 수 없어 닫는 꺾쇠로 확정된다((col) 은 그 토큰의 시작).
static auto Is_closer_signal(Lines const &mask, int const row, int const col)->bool{
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
	if( ::Word_starts_at(m, col) ){
		std::string const w = ::Word_at(m, col);

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
static auto Match_closer_anchored_angles(Lines const &mask)->std::vector<Angle_pair>{
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

			if( ::Word_before(mask, r, w) == "operator" ){
				continue;
			}

			// `>` 다음 첫 의미 토큰이 닫힘 신호인지.
			int sr = r;
			int sc = c + 1;

			if( !::Next_code(mask, rows - 1, sr, sc) ){
				continue;
			}

			if( !::Is_closer_signal(mask, sr, sc) ){
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

					if( !::Match_bracket_back(mask, op, ch, br, bc) ){
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
					if( ::Word_before(mask, br, bc) == "operator" ){
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

	::Dedup_angles(out);

	return out;
}

// §5.4 다중행 꺾쇠 레이아웃 — 여는 `<` 이 여는 행 마지막·닫는 `>` 이 닫는 행 첫·여닫는
// 들여쓰기 동일·중간 코드 행 들여쓰기 ≥ 여는 행 +1.
// `>>` 로 분해된 두 pair 는 각각 검사되지만 c_col 이 인접해 자연히 성립한다.
static void Check_multiline_angle(
	Lines const &lines, Lines const &mask, Angle_pair const &p, std::vector<Violation> &out
){
	if(p.o_row == p.c_row){
		return;
	}

	int const o_ind = ::Indent_depth(lines[p.o_row]);
	int const c_ind = ::Indent_depth(lines[p.c_row]);

	if(o_ind != c_ind){
		::Push_fix(
			out, { p.c_row, 0, "5.4", "angle: open/close indent differ" },
			Fix_kind::indent, 0, o_ind
		);
	}

	std::string const &o_line = mask[p.o_row];
	int const o_n = static_cast<int>(o_line.size());

	for(int cc = p.o_col + 1; cc < o_n; ++cc){
		if( ::Is_code_char(o_line[cc]) ){
			out.push_back({ p.o_row, cc, "5.4", "angle: opening '<' not last token" });

			break;
		}
	}

	std::string const &c_line = mask[p.c_row];

	for(int cc = 0; cc < p.c_col; ++cc){
		if( ::Is_code_char(c_line[cc]) ){
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
		if( lines[r].empty() || !::Has_code(mask[r]) ){
			continue;
		}

		if( ::Indent_depth(lines[r]) < o_ind + 1 ){
			::Push_fix(
				out, { r, 0, "5.4", "angle: middle line indent insufficient" },
				Fix_kind::indent, 0, o_ind + 1
			);
		}
	}
}

// §5.7 특수괄호 닫힘 — 다중행 닫는 `>` 뒤에 개행 외 코드 토큰 없음.
static void Check_angle_close_last(
	Lines const &mask, Angle_pair const &p, std::vector<Violation> &out
){
	if(p.o_row == p.c_row){
		return;
	}

	std::string const &c_line = mask[p.c_row];
	int const cn = static_cast<int>(c_line.size());

	for(int cc = p.c_col + 1; cc < cn; ++cc){
		if( ::Is_code_char(c_line[cc]) ){
			// `>>` 로 분해된 첫 `>` 뒤 바로 다음 `>` 는 자기 짝의 c_col 이라 예외.
			if(cc == p.c_col + 1 && c_line[cc] == '>'){
				break;
			}

			out.push_back({ p.c_row, cc, "5.7", "angle: '>' not last token on line" });

			break;
		}
	}
}

// §8.4 경계 공백 — 여는 `<` 직전 word 는 무공백, 닫는 `>` 직후 word 는 공백·`(` `[` 은 무공백.
static void Check_angle_boundary(
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

			if( q >= 0 && ::is_word_char(o_line[q]) ){
				::Push_fix(
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
			::Push_fix(
				out, { p.c_row, p.c_col + 1, "8.4", "angle: space between '>' and '(' or '['" },
				Fix_kind::gap_right, p.c_col + 1, 0
			);
		}
	} else if( ::is_word_char(nx) ){
		if(!has_space){
			::Push_fix(
				out, { p.c_row, p.c_col + 1, "8.4", "angle: '>' and word need one space" },
				Fix_kind::gap_right, p.c_col + 1, 1
			);
		}
	}
}

// §8.5 안쪽 공백 n — 단일행 꺾쇠는 자기 안 최대 중첩 단계 +1 (자기 자리 포함).
// pairs 전체를 참조해 이 pair 안에 몇 겹의 단일행 꺾쇠가 있는지 센다.
static void Check_angle_inner_space(
	Lines const &mask, std::vector<Angle_pair> const &pairs,
	Angle_pair const &p, std::vector<Violation> &out
){
	if(p.o_row != p.c_row){
		return;
	}

	// 최대 중첩 단계: 이 pair 안에 포함된 단일행 pair 들의 최대 자체 중첩 +1.
	// 재귀 없이 iterative — 자기가 포함하는 pair 들의 depth 를 recursion 으로 계산.
	// (pairs 는 정렬되어 있고 크기 작음 — O(N^2) 로 충분.)
	auto nest_depth
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
	};

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
		::Push_fix(
			out, { p.o_row, p.o_col + 1, "8.5", "angle: inner space must be N" },
			Fix_kind::gap_right, p.o_col + 1, n
		);
	}

	if(right != n){
		::Push_fix(
			out, { p.o_row, p.c_col - right, "8.5", "angle: inner space must be N" },
			Fix_kind::gap_left, p.c_col, n
		);
	}
}

// §3 금지 키워드 typedef/goto (@마스크, 단어 경계).
static void Check_banned(std::string const &mask, int const row, std::vector<Violation> &out){
	static std::string const Banned[] = { "typedef", "goto" };

	for(std::string const &kw : Banned){
		for(
			std::size_t pos = mask.find(kw);
			pos != std::string::npos;
			pos = mask.find(kw, pos + 1)
		){
			if(
				std::size_t const end = pos + kw.size();
				( pos == 0 || !::is_word_char(mask[pos - 1]) )
				&& ( end >= mask.size() || !::is_word_char(mask[end]) )
			){
				int const col = static_cast<int>(pos);

				out.push_back({ row, col, "3", "banned keyword: " + kw });
			}
		}
	}
}

// §3 키워드 위치 후보 — 기본 타입 키워드(닫힌 집합).
static auto is_basic_type(std::string const &w)->bool{
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
static auto Token_text(std::string const &mask, Tok_8_3 const &t)->std::string{
	return mask.substr(t.col, t.len);
}

// §3 키워드 위치 — const/volatile/constexpr 후위, static/inline 전위 (@마스크, 단일행).
// 무위양성 유지를 위해 *기본 타입 키워드와 인접한* 자동 확정분만 잡는다(사용자 정의 타입
// 인접·다중행 선언은 서브에이전트 몫).
//   `const|volatile|constexpr` + 기본타입  → 한정자가 타입 앞 = 서향 위반.
//   기본타입 + `static|inline`             → 스토리지 지정자가 타입 뒤 = 위반.
// `if constexpr` 는 constexpr 뒤가 '(' 라 단어쌍이 아니어서 자연히 제외된다.
static void Check_keyword_position(
	std::string const &mask, int const row, std::vector<Violation> &out
){
	auto const toks = ::Tokenize_8_3(mask);
	int const n = static_cast<int>(toks.size());

	for(int i = 0; i + 1 < n; ++i){
		if(toks[i].cls != Tk_cls::word || toks[i + 1].cls != Tk_cls::word){
			continue;
		}

		std::string const w0 = ::Token_text(mask, toks[i]), w1 = ::Token_text(mask, toks[i + 1]);
		bool const qual0 = w0 == "const" || w0 == "volatile" || w0 == "constexpr";
		bool const stor1 = w1 == "static" || w1 == "inline";

		if( qual0 && ::is_basic_type(w1) ){
			out.push_back(
				{ row, toks[i].col, "3", "const/volatile/constexpr must follow its type" }
			);
		} else if( ::is_basic_type(w0) && stor1 ){
			out.push_back({ row, toks[i + 1].col, "3", "static/inline must precede its type" });
		}
	}
}

// §3 단항 연산자 병기 — `- -x`/`+ +x` 처럼 같은 단항 부호가 공백으로 갈린 자리 (@마스크, 단일행).
// 붙이면 `--`/`++` 가 되어 의미가 바뀌므로 괄호로 구분해야 한다(`-(-x)`).
// 첫 부호가 단항임이 어휘적으로 확실한 자리(직전이 여는괄호·구분자·양쪽공백 이항연산자)에서만
// 확정한다. 직전이 피연산자(식별자·닫는괄호 등)면 이항일 수 있어 서브에이전트 몫.
static void Check_unary_juxtaposition(
	std::string const &mask, int const row, std::vector<Violation> &out
){
	auto const toks = ::Tokenize_8_3(mask);
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
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

auto check_lines(Lines const &lines, Seg_lines const &segs)->std::vector<Violation>{
	std::vector<Violation> out;
	Lines const mask = ::render_mask(lines, segs);
	int const rows = static_cast<int>(lines.size());

	std::vector<Bk_pair> const pairs = ::Match_brackets(mask);

	for(Bk_pair const &p : pairs){
		::Check_multiline_bracket(lines, mask, p, out);
		::Check_attribute_close(mask, p, out);
		::Check_bracket_blank_line(lines, mask, p, out);
	}

	::Check_hidden_brace(lines, mask, pairs, out);
	::Check_anchor_keyword_semicolon(lines, mask, out);
	::Check_anchor_trailing_return(lines, mask, out);
	::Check_anchor_inline_type(lines, mask, pairs, out);
	::Check_anchor_case(lines, mask, out);
	::Check_anchor_colon_vbracket(lines, mask, out);
	::Check_anchor_var_decl_marker(lines, mask, out);

	std::vector<Angle_pair> angles = ::Match_template_cast_angles(mask);
	std::vector<Angle_pair> const closer_angles = ::Match_closer_anchored_angles(mask);
	angles.insert(angles.end(), closer_angles.begin(), closer_angles.end());
	::Dedup_angles(angles);

	for(Angle_pair const &a : angles){
		::Check_multiline_angle(lines, mask, a, out);
		::Check_angle_close_last(mask, a, out);
		::Check_angle_boundary(mask, a, out);
		::Check_angle_inner_space(mask, angles, a, out);
	}

	for(int row = 0; row < rows; ++row){
		::Check_width(lines[row], row, out);
		::Check_indent(lines[row], row, out);
		::Check_tab_use(mask[row], row, out);
		::Check_space_run(mask[row], row, out);
		::Check_inner_space(mask[row], row, out);
		::Check_ctrl_brace(mask, row, out);
		::Check_word_paren_space(mask[row], row, out);
		::Check_word_paren_newline(mask, row, out);
		::Check_blank_line(lines, mask, row, out);
		::Check_token_space(mask[row], row, out);
		::Check_keyword_position(mask[row], row, out);
		::Check_unary_juxtaposition(mask[row], row, out);
		::Check_banned(mask[row], row, out);
	}

	return out;
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

// 한 행에서 공백·탭을 모두 뺀 문자열 — 회귀 게이트용(비공백·줄 구조 불변 증명).
static auto Strip_ws(std::string const &line)->std::string{
	std::string out;

	for(char const ch : line){
		if(ch != ' ' && ch != '\t'){
			out += ch;
		}
	}

	return out;
}

// edit 한 항목: 어느 행·열의 위반을 어떤 수정으로 처리했는지.
struct Edit_op{
	int row, col, val;
	Fix_kind kind;
};

// 한 행에 수정을 적용한다. gap 은 fix_col 경계의 공백·탭 런을 val 개의 공백으로,
// indent 는 선두 공백·탭 구역을 val 개의 탭으로 맞춘다(모두 공백·탭만 건드린다).
static void Apply_edit_op(std::string &line, Edit_op const &op){
	int const n = static_cast<int>(line.size());

	if(op.kind == Fix_kind::indent){
		int q = 0;

		while( q < n && (line[q] == ' ' || line[q] == '\t') ){
			++q;
		}

		line = std::string(op.val, '\t') + line.substr(q);

		return;
	}

	if(op.kind == Fix_kind::gap_left){
		int s = op.col;

		while( s > 0 && (line[s - 1] == ' ' || line[s - 1] == '\t') ){
			--s;
		}

		line = line.substr(0, s) + std::string(op.val, ' ') + line.substr(op.col);

		return;
	}

	if(op.kind == Fix_kind::gap_right){
		int e = op.col;

		while( e < n && (line[e] == ' ' || line[e] == '\t') ){
			++e;
		}

		line = line.substr(0, op.col) + std::string(op.val, ' ') + line.substr(e);
	}
}

auto edit_lines(Lines const &input, int const lo, int const hi)->Edit_result{
	Lines lines = input;
	std::vector<Edit_note> fixed_notes;

	// 고정점까지 반복 — 매 패스 재검사해 자동교정 힌트(fix != none)를 모아 적용한다.
	for(int pass = 0; pass < 8; ++pass){
		Seg_lines const segs = ::scan_lines(lines);
		std::vector<Violation> const viol = ::check_lines(lines, segs);

		std::vector<Edit_op> ops;

		for(Violation const &v : viol){
			if(v.fix == Fix_kind::none || v.row < lo || v.row > hi){
				continue;
			}

			ops.push_back({ v.row, v.fix_col, v.fix_val, v.fix });
			fixed_notes.push_back({ v.row, v.col, v.rule, v.message, true });
		}

		if(ops.empty()){
			break;
		}

		// 한 행 안에서는 오른쪽(높은 열)부터 적용해 앞선 수정이 뒤 열을 밀지 않게 한다.
		// indent 는 fix_col 이 0 이라 자연히 마지막에 적용된다.
		std::sort(
			ops.begin(), ops.end(),
			[](Edit_op const &a, Edit_op const &b)->bool{
				return a.row != b.row ? a.row < b.row : a.col > b.col;
			}
		);

		for(Edit_op const &op : ops){
			::Apply_edit_op(lines[op.row], op);
		}
	}

	// 회귀 게이트 — 공백·탭 제거 후 바이트 동일 + 행 수 불변이어야 자동교정이 정당하다.
	bool ok = lines.size() == input.size();

	for(std::size_t i = 0; ok && i < lines.size(); ++i){
		if( ::Strip_ws(lines[i]) != ::Strip_ws(input[i]) ){
			ok = false;
		}
	}

	Edit_result res;
	res.ok = ok;
	res.lines = ok ? lines : input;

	// 자동교정 기록(중복 제거) — 게이트를 통과했을 때만 유효하다.
	if(ok){
		for(Edit_note const &fn : fixed_notes){
			bool dup = false;

			for(Edit_note const &seen : res.notes){
				if(
					seen.row == fn.row && seen.col == fn.col
					&& seen.rule == fn.rule && seen.message == fn.message
				){
					dup = true;

					break;
				}
			}

			if(!dup){
				res.notes.push_back(fn);
			}
		}
	}

	// 자동교정 밖에 남은 위반(범위 안)을 manual 로 기록 — 사람·AI 인계 목록.
	Seg_lines const segs = ::scan_lines(res.lines);
	std::vector<Violation> const remain = ::check_lines(res.lines, segs);

	for(Violation const &v : remain){
		if(v.row < lo || v.row > hi){
			continue;
		}

		res.notes.push_back({ v.row, v.col, v.rule, v.message, false });
	}

	std::sort(
		res.notes.begin(), res.notes.end(),
		[](Edit_note const &a, Edit_note const &b)->bool{
			return a.row != b.row ? a.row < b.row : a.col < b.col;
		}
	);

	return res;
}
