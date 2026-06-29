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

	for(int row = 0; row < rows; ++row){
		::Check_width(lines[row], row, out);
		::Check_indent(lines[row], row, out);
		::Check_trailing(mask[row], row, out);
		::Check_tab_use(mask[row], row, out);
		::Check_space_run(mask[row], row, out);
		::Check_inner_space(mask[row], row, out);
		::Check_ctrl_brace(mask, row, out);
		::Check_banned(mask[row], row, out);
	}

	return out;
}
