/*  SPDX-FileCopyrightText: (c) 2020 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "lexer.hpp"

#include <cctype>
#include <cstddef>
#include <string>

enum class Mode{ normal, line_comment, block_comment, string_lit, char_lit, raw_string };

// 행 사이로 넘기는 스캔 상태. raw_string 은 구분자를, preproc 은 '\' 줄연장을 이어간다.
struct Scan_state{
	Mode mode;
	std::string raw_delim;
	bool preproc_cont;
};

struct Line_result{
	std::vector<Segment> segs;
	Scan_state out_state;
};
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

// '가 문자 리터럴 시작이 아니라 숫자 구분자(1'000, 0xa'b)인지 어림한다.
static auto is_digit_sep(std::string const &line, int const i)->bool{
	if( i <= 0 || i + 1 >= static_cast<int>(line.size()) ){
		return false;
	}

	auto const  
		prev = static_cast<unsigned char>(line[i - 1]),
		next = static_cast<unsigned char>(line[i + 1])
	;

	return std::isxdigit(prev) && std::isxdigit(next);
}

// '"'(위치 quote) 앞의 식별자 런 시작 위치.
static auto Word_start(std::string const &line, int const quote)->int{
	int j = quote - 1;

	while(j >= 0){
		unsigned char const ch = static_cast<unsigned char>(line[j]);

		if( !std::isalnum(ch) && ch != '_' ){
			break;
		}

		--j;
	}

	return j + 1;
}

// '"'(위치 quote) 가 raw 문자열 접두사(R/LR/uR/UR/u8R) 뒤인지 판정한다.
static auto is_raw_prefix(std::string const &line, int const quote)->bool{
	int const start = ::Word_start(line, quote);
	std::string const pfx = line.substr(start, quote - start);

	return pfx == "R" || pfx == "LR" || pfx == "uR" || pfx == "UR" || pfx == "u8R";
}

// 위치 i 의 ')' 에서 raw 문자열이 )delim" 로 닫히는지.
static auto raw_closes_at(std::string const &line, int const i, std::string const &delim)->bool{
	if(line[i] != ')'){
		return false;
	}

	int const dlen = static_cast<int>(delim.size()), n = static_cast<int>(line.size());

	if(i + 1 + dlen >= n){
		return false;
	}

	bool const delim_ok = line.compare(i + 1, dlen, delim) == 0;

	return delim_ok && line[i + 1 + dlen] == '"';
}

// 행의 첫 비공백 문자가 '#' 인지(전처리 지시행).
static auto is_preproc_line(std::string const &line)->bool{
	for(char const c : line){
		if(c == ' ' || c == '\t'){
			continue;
		}

		return c == '#';
	}

	return false;
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

// 한 행을 in 상태로 시작해 좌->우로 스캔, 세그먼트와 다음 행으로 넘길 상태를 낸다.
static auto Scan_one_line(
	std::string const &line, int const row, Scan_state const &in
)->Line_result{
	int const n = static_cast<int>(line.size());
	std::vector<Segment> segs;

	// 직전 preproc 행이 '\'로 이어졌으면 이 행 전체가 preproc.
	if(in.preproc_cont){
		if(n > 0){
			segs.push_back(Segment{ Seg_kind::preproc, row, 0, n });
		}

		bool const cont = n > 0 && line[n - 1] == '\\';

		return{  std::move(segs), Scan_state{ Mode::normal, "", cont }  };
	}

	Mode mode = in.mode;
	std::string raw_delim = in.raw_delim;

	// normal 에서 시작하는 행이 '#' 지시행이면 통째로 preproc.
	if( mode == Mode::normal && ::is_preproc_line(line) ){
		if(n > 0){
			segs.push_back(Segment{ Seg_kind::preproc, row, 0, n });
		}

		bool const cont = n > 0 && line[n - 1] == '\\';

		return{  std::move(segs), Scan_state{ Mode::normal, "", cont }  };
	}

	int seg_start = 0;
	Seg_kind kind = Seg_kind::code;

	if(mode == Mode::block_comment){
		kind = Seg_kind::comment;
	}
	else if(mode == Mode::raw_string){
		kind = Seg_kind::raw_string;
	}

	auto  
		flush
		= [&seg_start, &segs, &kind, &row](int const end){
			if(end > seg_start){
				segs.push_back(Segment{ kind, row, seg_start, end - seg_start });
			}
		}
	;

	int i = 0;

	while(i < n){
		char const c = line[i];

		if(mode == Mode::normal){
			if(c == '/' && i + 1 < n && line[i + 1] == '/'){
				flush(i);
				kind = Seg_kind::comment;
				seg_start = i;
				mode = Mode::line_comment;
				i += 2;
			}
			else if(c == '/' && i + 1 < n && line[i + 1] == '*'){
				flush(i);
				kind = Seg_kind::comment;
				seg_start = i;
				mode = Mode::block_comment;
				i += 2;
			}
			else if( c == '"' && ::is_raw_prefix(line, i) ){
				int const pfx = ::Word_start(line, i);

				flush(pfx);
				kind = Seg_kind::raw_string;
				seg_start = pfx;
				mode = Mode::raw_string;

				// 구분자: '"' 다음부터 '(' 전까지.
				raw_delim.clear();
				i += 1;

				while(i < n && line[i] != '('){
					raw_delim += line[i];
					++i;
				}

				if(i < n){
					i += 1;
				}
			}
			else if(c == '"'){
				flush(i);
				kind = Seg_kind::string_lit;
				seg_start = i;
				mode = Mode::string_lit;
				i += 1;
			}
			else if( c == '\'' && !::is_digit_sep(line, i) ){
				flush(i);
				kind = Seg_kind::char_lit;
				seg_start = i;
				mode = Mode::char_lit;
				i += 1;
			}
			else{
				i += 1;
			}
		}
		else if(mode == Mode::line_comment){
			i = n;
		}
		else if(mode == Mode::block_comment){
			if(c == '*' && i + 1 < n && line[i + 1] == '/'){
				i += 2;
				flush(i);
				kind = Seg_kind::code;
				seg_start = i;
				mode = Mode::normal;
			}
			else{
				i += 1;
			}
		}
		else if(mode == Mode::raw_string){
			if( ::raw_closes_at(line, i, raw_delim) ){
				i += static_cast<int>(raw_delim.size()) + 2;
				flush(i);
				kind = Seg_kind::code;
				seg_start = i;
				mode = Mode::normal;
			}
			else{
				i += 1;
			}
		}
		else if(mode == Mode::string_lit){
			if(c == '\\' && i + 1 < n){
				i += 2;
			}
			else if(c == '"'){
				i += 1;
				flush(i);
				kind = Seg_kind::code;
				seg_start = i;
				mode = Mode::normal;
			}
			else{
				i += 1;
			}
		}
		else{
			if(c == '\\' && i + 1 < n){
				i += 2;
			}
			else if(c == '\''){
				i += 1;
				flush(i);
				kind = Seg_kind::code;
				seg_start = i;
				mode = Mode::normal;
			}
			else{
				i += 1;
			}
		}
	}

	flush(n);

	bool const carry_on = mode == Mode::block_comment || mode == Mode::raw_string;
	Mode const out_mode = carry_on ? mode : Mode::normal;

	std::string out_delim;

	if(mode == Mode::raw_string){
		out_delim = std::move(raw_delim);
	}

	return{  std::move(segs), Scan_state{ out_mode, std::move(out_delim), false }  };
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

auto scan_lines(Lines const &lines)->Seg_lines{
	Scan_state carry{ Mode::normal, "", false };
	int row = 0;
	Seg_lines result;
	result.reserve(lines.size());

	for(std::string const &line : lines){
		Line_result r = ::Scan_one_line(line, row, carry);

		carry = r.out_state;
		result.emplace_back( std::move(r.segs) );
		++row;
	}

	return result;
}

// 무효 세그먼트를 같은 길이의 '@' 로 덮고, code 는 원문 그대로 둔다(기하 1:1).
auto render_mask(Lines const &lines, Seg_lines const &segs)->Lines{
	int row = 0;
	Lines out;
	out.reserve(lines.size());

	for(std::string const &line : lines){
		std::string masked;

		for(Segment const &s : segs[row]){
			if(s.kind == Seg_kind::code){
				masked += line.substr(s.col, s.len);
			}
			else{
				masked += std::string( static_cast<std::size_t>(s.len), '@' );
			}
		}

		out.emplace_back( std::move(masked) );
		++row;
	}

	return out;
}

static auto Kind_name(Seg_kind const k)->std::string{
	switch(k){
	case Seg_kind::comment:
		return "COMMENT";
	case Seg_kind::string_lit:
		return "STRING";
	case Seg_kind::char_lit:
		return "CHAR";
	case Seg_kind::raw_string:
		return "RAW_STRING";
	case Seg_kind::preproc:
		return "PREPROC";
	case Seg_kind::code:
		return "CODE";
	default:
		return "?";
	}
}

// 무효 세그먼트를 $KIND[고유번호][길이]$ 로, code 는 원문 그대로 직렬화한다.
auto render_dump(Lines const &lines, Seg_lines const &segs)->Lines{
	int idx = 0;
	int row = 0;
	Lines out;
	out.reserve(lines.size());

	for(std::string const &line : lines){
		std::string text;

		for(Segment const &s : segs[row]){
			if(s.kind == Seg_kind::code){
				text += line.substr(s.col, s.len);
			}
			else{
				text += "$";
				text += ::Kind_name(s.kind);
				text += "[" + std::to_string(idx) + "]";
				text += "[" + std::to_string(s.len) + "]$";
				++idx;
			}
		}

		out.emplace_back( std::move(text) );
		++row;
	}

	return out;
}
