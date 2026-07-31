/*  SPDX-FileCopyrightText: (c) 2020 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "check_internal.hpp"

#include <cctype>
#include <cstddef>
#include <string>
#include <vector>

namespace sak{
	static auto is_wide(unsigned long const cp)->bool;
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

// 코드포인트가 동아시아 와이드/전각 근사 범위에 드는지.
auto sak::is_wide(unsigned long const cp)->bool{
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
auto sak::Display_width(std::string const &line)->std::size_t{
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
		}
		else if( (lead >> 4) == 0xE ){
			bytes = 3;
			cp = lead & 0x0F;
		}
		else if( (lead >> 3) == 0x1E ){
			bytes = 4;
			cp = lead & 0x07;
		}

		bool valid = i + bytes <= n;

		for(std::size_t k = 1; valid && k < bytes; ++k){
			unsigned char const cont = static_cast<unsigned char>(line[i + k]);

			if( (cont >> 6) != 0x2 ){
				valid = false;
			}
			else{
				cp = (cp << 6) | (cont & 0x3F);
			}
		}

		if(!valid){
			width += 1;
			i += 1;

			continue;
		}

		width += is_wide(cp) ? 2 : 1;
		i += bytes;
	}

	return width;
}

auto sak::is_word_char(char const c)->bool{
	unsigned char const u = static_cast<unsigned char>(c);

	return std::isalnum(u) || c == '_';
}

// §9.4 공행 판정 — 일반문자(개행·백문자·가상문자가 아닌 문자)가 없는 행.
// 빈 행과 공백·탭만 있는 행이 모두 공행이다(v2.10). 그 백문자는 들여쓰기·§8 공백 규칙에
// 참여하지 않으므로(§1.3·§8.3), 공행은 §9.4 만이 다룬다.
auto sak::Is_blank_row(std::string const &line)->bool{
	for(char const c : line){
		if(c != ' ' && c != '\t'){
			return false;
		}
	}

	return true;
}

// 위반을 내면서 edit 모드용 수정 힌트(§8.3/§5.5 토대)를 함께 싣는다.
// (집합체 초기화를 한 행에 유지하고자 힌트는 여기서 붙인다.)
void sak::Push_fix(
	std::vector<Violation> &out, Violation v,
	Fix_kind const kind, int const fix_col, int const fix_val
){
	v.fix = kind;
	v.fix_col = fix_col;
	v.fix_val = fix_val;
	out.push_back(v);
}

// (row,col)부터 공백·탭·@(주석/문자열)·개행을 건너뛴 다음 코드 문자를 찾는다(최대 max_row 행).
auto sak::Next_code(Lines const &mask, int const max_row, int &row, int &col)->bool{
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
auto sak::Match_paren(Lines const &mask, int const max_row, int &row, int &col)->bool{
	for(int depth = 0; row <= max_row; ++row, col = 0){
		for( int const len = static_cast<int>(mask[row].size()); col < len; ++col ){
			if(char const c = mask[row][col]; c == '('){
				++depth;
			}
			else if(c == ')'){
				if(--depth == 0){
					return true;
				}
			}
		}
	}

	return false;
}

// (row,col) 직전의 의미 토큰 위치를 찾는다(공백·탭·@·개행 건너뜀). 찾으면 true.
auto sak::Prev_significant(Lines const &mask, int &row, int &col)->bool{
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
auto sak::Match_brace_back(Lines const &mask, int &row, int &col)->bool{
	for(int depth = 0; row >= 0;){
		for(; col >= 0; --col){
			if(char const ch = mask[row][col]; ch == '}'){
				++depth;
			}
			else if(ch == '{'){
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
auto sak::Match_bracket_back(
	Lines const &mask, char const open, char const close, int &row, int &col
)->bool{
	for(int depth = 0; row >= 0;){
		for(; col >= 0; --col){
			if(char const ch = mask[row][col]; ch == close){
				++depth;
			}
			else if(ch == open){
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

// (line,col)을 마지막 글자로 삼는 식별자 런을 돌려준다(그 자리가 식별자가 아니면 빈 문자열).
auto sak::Word_ending_at(std::string const &line, int const col)->std::string{
	int s = col;

	while( s >= 0 && is_word_char(line[s]) ){
		--s;
	}

	return line.substr(s + 1, col - s);
}

// (row,col) 직전의 의미 토큰이 식별자면 그 식별자를, 아니면 빈 문자열을 돌려준다.
auto sak::Word_before(Lines const &mask, int const row, int const col)->std::string{
	int r = row, c = col - 1;

	if( !Prev_significant(mask, r, c) ){
		return "";
	}

	return Word_ending_at(mask[r], c);
}

// while(...); 가 do-while 꼬리인지: 직전 '}' 가 'do' 블록을 닫는지 역방향으로 확인.
auto sak::Is_do_tail(Lines const &mask, int const row, int const col)->bool{
	int r = row, c = col - 1;

	if( !Prev_significant(mask, r, c) || mask[r][c] != '}' ){
		return false;
	}

	if( !Match_brace_back(mask, r, c) ){
		return false;
	}

	return Word_before(mask, r, c) == "do";
}

// (line,col)에서 시작하는 식별자 런을 돌려준다(식별자가 아니면 빈 문자열).
auto sak::Word_at(std::string const &line, int const col)->std::string{
	int const len = static_cast<int>(line.size());
	int e = col;

	while( e < len && is_word_char(line[e]) ){
		++e;
	}

	return line.substr(col, e - col);
}

// (line,col) 이 식별자 런의 시작인지(왼쪽 경계가 단어문자가 아님).
auto sak::Word_starts_at(std::string const &line, int const i)->bool{
	return is_word_char(line[i]) && ( i == 0 || !is_word_char(line[i - 1]) );
}

// row 의 마지막 의미 토큰 열(@마스크). 없으면 -1.
auto sak::Last_significant_col(std::string const &mask_row)->int{
	int p = static_cast<int>(mask_row.size()) - 1;

	while( p >= 0 && (mask_row[p] == ' ' || mask_row[p] == '\t' || mask_row[p] == '@') ){
		--p;
	}

	return p;
}

// row 의 첫 의미 토큰 열(@마스크). 없으면 -1.
auto sak::First_significant_col(std::string const &mask_row)->int{
	int const n = static_cast<int>(mask_row.size());
	int p = 0;

	while( p < n && (mask_row[p] == ' ' || mask_row[p] == '\t' || mask_row[p] == '@') ){
		++p;
	}

	return p < n ? p : -1;
}

// 순수 공행만 건너 다음 코드 행을 찾는다(공행 판정은 raw 행, 코드 판정은 마스크/컷마스크).
// 공행은 개행의 발생원이 아니라 인가된 자리에 쌓인 형상(§9.4)이므로, 개행 자리 검사는 공행을
// 투명하게 횡단해야 한다. 공행이 아닌 무코드 행(주석·전처리·문자열 전용)을 만나면 -1 —
// 그 행들은 §2 제외 대상으로 그 자체가 발생원이라 보수적으로 포기한다.
auto sak::Next_code_row_over_blanks(
	Lines const &lines, Lines const &mask, int const from, bool &crossed_blank
)->int{
	int const rows = static_cast<int>(mask.size());

	crossed_blank = false;

	for(int r = from + 1; r < rows; ++r){
		if( Is_blank_row(lines[r]) ){
			crossed_blank = true;

			continue;
		}

		if( !Has_code(mask[r]) ){
			return -1;
		}

		return r;
	}

	return -1;
}

// 행 머리에서 위치 p 가 여는 들여쓰기 단위의 문자 길이 — 탭은 1, 공백 4칸은 4(§8.2 등가),
// 단위가 아니면 0. 이 하나로 §1.3 Check_indent 와 아래 Indent_depth 가 같은 눈을 공유한다.
auto sak::Indent_unit_at(std::string const &line, int const p)->int{
	int const n = static_cast<int>(line.size());

	if(p < n && line[p] == '\t'){
		return 1;
	}

	int run = 0;

	while(p + run < n && run < 4 && line[p + run] == ' '){
		++run;
	}

	return run == 4 ? 4 : 0;
}

// 들여쓰기 깊이 = 행 머리를 채우는 들여쓰기 단위(탭 하나 또는 공백 4칸)의 개수.
auto sak::Indent_depth(std::string const &line)->int{
	int p = 0, depth = 0;

	while(true){
		int const len = Indent_unit_at(line, p);

		if(len == 0){
			break;
		}

		++depth;
		p += len;
	}

	return depth;
}

// 마스크상 코드 토큰을 하나라도 포함하는 행인지(전처리기·주석·문자열 only 는 false).
auto sak::Has_code(std::string const &mask_row)->bool{
	for(char const c : mask_row){
		if(c != ' ' && c != '\t' && c != '@'){
			return true;
		}
	}

	return false;
}

// 마스크 행의 첫 코드 문자.
auto sak::First_code_char(std::string const &mask_line)->char{
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
auto sak::Last_code_char(std::string const &mask_line)->char{
	for( int i = static_cast<int>(mask_line.size()) - 1; i >= 0; --i ){
		char const c = mask_line[i];

		if(c != ' ' && c != '\t' && c != '@'){
			return c;
		}
	}

	return '\0';
}

// 마스크 행이 특정 키워드로 시작하는지(그 뒤가 단어경계).
auto sak::Starts_with_keyword(std::string const &mask_line, char const *kw)->bool{
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

	return nxt >= n || !is_word_char(mask_line[nxt]);
}

// 이항·삼항 연산자·구두점으로 시작하는 행 — 이전 표현식의 연장.
auto sak::Starts_with_continuation_op(std::string const &mask_line)->bool{
	char const c = First_code_char(mask_line);

	return
		c == '=' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%'
		|| c == '&' || c == '|' || c == '^' || c == '<' || c == '>' || c == '!'
		|| c == '~' || c == '?' || c == ':' || c == ',' || c == '.'
	;
}

// 닫는 괄호 뒤 같은 행에 세미콜론 아닌 코드 토큰이 남아 있는지(표현식 종속의 증거).
// `}(p),` `}.method()` `}` +쉼표 같은 자리는 문장 종결이 아니므로 아래쪽 공행 요구 유보.
auto sak::Has_nonsemi_code_after(std::string const &mask_line, int const pos)->bool{
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

// 닫는 괄호 뒤 같은 행의 꼬리가 문장 종결로만 이어지는지 — 다른 닫는 괄호가 나오면 그 짝의
// 소관이고(중복 보고 방지), 마지막 코드 문자가 `;` 일 때만 참이다.
auto sak::Stmt_ends_after(std::string const &mask_line, int const pos)->bool{
	int const n = static_cast<int>(mask_line.size());
	char last = 0;

	for(int i = pos; i < n; ++i){
		char const c = mask_line[i];

		if(c == ' ' || c == '\t' || c == '@'){
			continue;
		}

		if(c == ')' || c == ']' || c == '}'){
			return false;
		}

		last = c;
	}

	return last == ';';
}

// 마스크 한 행에서 col 위치에 코드 토큰이 있는지(공백·탭·@가 아니면 코드).
auto sak::Is_code_char(char const ch)->bool{
	return ch != ' ' && ch != '\t' && ch != '@';
}

// 행 r 이 숨은 중괄호의 라벨 행인가 — `case`/`default`, 또는 `:` 를 뒤에 둔 접근 지정자.
// 라벨 행이면 그 첫 의미 열을 head_col 에 채우고 true. (§5.6 리듬·닫힘 판정에 공유.)
auto sak::Label_row(Lines const &mask, int const r, int &head_col)->bool{
	if( r < 0 || r >= static_cast<int>(mask.size()) || !Has_code(mask[r]) ){
		return false;
	}

	std::string const &m = mask[r];
	int first = 0;
	int const m_n = static_cast<int>(m.size());

	while( first < m_n && (m[first] == ' ' || m[first] == '\t' || m[first] == '@') ){
		++first;
	}

	if(first >= m_n){
		return false;
	}

	std::string const head = Word_at(m, first);

	bool const is_case_or_default = head == "case" || head == "default";
	bool const is_access = head == "public" || head == "private" || head == "protected";

	if(!is_case_or_default && !is_access){
		return false;
	}

	// 접근지시자는 `public`/`private`/`protected` 바로 뒤 (공백 skip) 가 `:` 여야 한다.
	// 아니면 상속 리스트의 `public Base` 등 다른 문맥이라 §5.6 대상 아님.
	if(is_access){
		int p = first + static_cast<int>(head.size());

		while( p < m_n && (m[p] == ' ' || m[p] == '\t' || m[p] == '@') ){
			++p;
		}

		if(p >= m_n || m[p] != ':'){
			return false;
		}
	}

	head_col = first;

	return true;
}

// 행의 [col, col+len) 바이트를 뜬다.
auto sak::Slice(std::string const &line, int const col, int const len)->std::string{
	return line.substr( static_cast<std::size_t>(col), static_cast<std::size_t>(len) );
}

// 행끝 잉여공백(§8.3)의 칸 수 — §5.5 2칸 마커 판정의 공용 토대. 탭은 §8.2 소관이라 세지 않는다.
auto sak::Tail_spaces(std::string const &line)->int{
	int t = static_cast<int>(line.size());

	while(t > 0 && line[t - 1] == ' '){
		--t;
	}

	return static_cast<int>(line.size()) - t;
}

// 그 행이 앞 문장의 연장으로 시작하는가 — 이항·삼항 연산자, 그리고 절을 잇는 `else`/`catch`
// (with_while 이면 do-while 꼬리 `while` 포함). §9.4 공행 봉투 검사들이 공유하는 유보 조건.
auto sak::Continues_statement(std::string const &mask_line, bool const with_while)->bool{
	return
		Starts_with_continuation_op(mask_line)
		|| Starts_with_keyword(mask_line, "else")
		|| Starts_with_keyword(mask_line, "catch")
		|| ( with_while && Starts_with_keyword(mask_line, "while") )
	;
}
