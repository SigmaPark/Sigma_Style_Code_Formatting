/*  SPDX-FileCopyrightText: (c) 2020 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "check.hpp"

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

// §8.2 후행 공백 (@마스크).
static void Check_trailing(std::string const &mask, int const row, std::vector<Violation> &out){
	int const n = static_cast<int>(mask.size());
	int p = n;

	while( p > 0 && (mask[p - 1] == ' ' || mask[p - 1] == '\t') ){
		--p;
	}

	if(p < n){
		out.push_back({ row, p, "8.2", "trailing whitespace" });
	}
}

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
			out.push_back({ row, pr.from, "8.5", msg });
		}

		if(before != want){
			out.push_back({ row, pr.to, "8.5", msg });
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
				out.push_back({ row, e, "8.4", "space between word and opening bracket" });
			}

			p = e;

			continue;
		}

		if( (c == ')' || c == ']' || c == '}') && p + 1 < n && ::is_word_char(mask[p + 1]) ){
			out.push_back({ row, p + 1, "8.4", "missing space between closing bracket and word" });
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
//   (sak_coverage.md '구현 로드맵' 참조 — 가상괄호 발현은 단계 3 §5.5 위에서 다룬다.)
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

	auto gap_before
		= [&mask, &toks](int const i)->int{
			int g = 0;

			while(toks[i].col - 1 - g >= 0 && mask[ toks[i].col - 1 - g ] == ' '){
				++g;
			}

			return g;
		}
	;

	auto gap_after
		= [&mask, &toks, mask_n](int const i)->int{
			int const e = toks[i].col + toks[i].len;
			int g = 0;

			while(e + g < mask_n && mask[e + g] == ' '){
				++g;
			}

			return g;
		}
	;

	auto word_eq
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
		Tk_cls const left_cls = has_l ? toks[i - 1].cls : Tk_cls::skip;
		Tk_cls const right_cls = has_r ? toks[i + 1].cls : Tk_cls::skip;

		// `operator =`·`operator ==`·`operator,` 등: `operator` 키워드 뒤 첫 기호형은
		// 오버로드 함수명의 일부이므로 §8.3 검사 영역 밖이다.
		if( has_l && word_eq(i - 1, "operator") ){
			continue;
		}
		// 투명성: 좌측 open_b·우측 close_b 는 인접 토큰으로 보지 않는다.
		bool const eff_l = has_l && left_cls != Tk_cls::open_b;
		bool const eff_r = has_r && right_cls != Tk_cls::close_b;
		bool const l_operand = left_cls == Tk_cls::word || left_cls == Tk_cls::close_b;
		bool const r_operand = right_cls == Tk_cls::word || right_cls == Tk_cls::open_b;

		switch(t.cls){
		case Tk_cls::sep:{
			// `for(init;;++itr2)` 처럼 `;` 두 개가 연속하면 그 사이는 공백 1 필수
			// (spec §8.3 SEP 항 예외). `,` 끼리·`,`+`;` 혼합은 일반 SEP 룰을 그대로 따른다.
			bool const
				semi_chain
				= t.len == 1 && mask[t.col] == ';'
				&& has_l && toks[i - 1].cls == Tk_cls::sep
				&& toks[i - 1].len == 1 && mask[ toks[i - 1].col ] == ';'
			;

			if( eff_l && !semi_chain && gap_before(i) > 0 ){
				out.push_back({ row, t.col, "8.3", "no space before separator" });
			}

			if( eff_l && semi_chain && gap_before(i) == 0 ){
				out.push_back({ row, t.col, "8.3", "space required between consecutive ';'" });
			}

			if( eff_r && right_cls != Tk_cls::sep && gap_after(i) == 0 ){
				out.push_back({ row, t.col + t.len, "8.3", "space required after separator" });
			}

			break;
		}
		case Tk_cls::bin_ns:
			if(!has_l || !has_r){
				break;
			}

			if( eff_l && l_operand && gap_before(i) > 0 ){
				out.push_back({ row, t.col, "8.3", "no space before '.','->','.*','->*'" });
			}

			if( eff_r && r_operand && gap_after(i) > 0 ){
				out.push_back(
					{ row, t.col + t.len, "8.3", "no space after '.','->','.*','->*'" }
				);
			}

			break;
		case Tk_cls::bin_s:
			if( eff_l && l_operand && gap_before(i) == 0 ){
				out.push_back({ row, t.col, "8.3", "space required before binary operator" });
			}

			if( eff_r && r_operand && gap_after(i) == 0 ){
				out.push_back(
					{ row, t.col + t.len, "8.3", "space required after binary operator" }
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
	int const o_ind = ::Indent_depth(lines[p.o_row]);
	int const c_ind = ::Indent_depth(lines[p.c_row]);

	if(o_ind != c_ind){
		out.push_back({ p.c_row, 0, "5.4", "open/close indent differ" });
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
		bool const hidden_close
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
			out.push_back({ r, 0, "5.4", "middle line indent insufficient" });
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
		bool const is_hidden
			= head == "case" || head == "default"
			|| head == "public" || head == "private" || head == "protected"
		;

		if(!is_hidden){
			continue;
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

		int const r_ind = ::Indent_depth(lines[r]);
		int const o_ind = ::Indent_depth(lines[inner->o_row]);

		if(r_ind != o_ind){
			out.push_back({ r, 0, "5.6", "hidden close: indent must equal enclosing brace" });
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

// §5.5 가상괄호 — return/throw 앵커.
// 키워드 행 안에 (괄호 깊이 0 의) ';' 가 없으면 다중행 발현이라 보고,
// 다음 코드 행의 들여쓰기가 키워드 행 + 1 이상인지 검사. 그 외 앵커(`->`·`case`·`:`·
// 변수선언)는 의미적 판정이 더 필요해 단계 3 1차 영역 밖이다.
static void Check_anchor_return_throw(
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

			if(w != "return" && w != "throw"){
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
				} else if(ch == ';' && depth == 0){
					found = true;

					break;
				}

				++cc;
			}

			if(found){
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
				out.push_back({ nr, 0, "5.5", "virtual bracket: continuation underindented" });
			}

			c = e;
		}
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
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

auto check_lines(Lines const &lines, Seg_lines const &segs)->std::vector<Violation>{
	std::vector<Violation> out;
	Lines const mask = ::render_mask(lines, segs);
	int const rows = static_cast<int>(lines.size());

	std::vector<Bk_pair> const pairs = ::Match_brackets(mask);

	for(Bk_pair const &p : pairs){
		::Check_multiline_bracket(lines, mask, p, out);
		::Check_attribute_close(mask, p, out);
	}

	::Check_hidden_brace(lines, mask, pairs, out);
	::Check_anchor_return_throw(lines, mask, out);

	for(int row = 0; row < rows; ++row){
		::Check_width(lines[row], row, out);
		::Check_indent(lines[row], row, out);
		::Check_trailing(mask[row], row, out);
		::Check_tab_use(mask[row], row, out);
		::Check_space_run(mask[row], row, out);
		::Check_inner_space(mask[row], row, out);
		::Check_ctrl_brace(mask, row, out);
		::Check_word_paren_space(mask[row], row, out);
		::Check_word_paren_newline(mask, row, out);
		::Check_blank_line(lines, mask, row, out);
		::Check_token_space(mask[row], row, out);
		::Check_banned(mask[row], row, out);
	}

	return out;
}
