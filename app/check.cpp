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
	struct Range{
		unsigned long lo;
		unsigned long hi;
	};

	static Range const Wide_ranges[] = {
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
	};

	for(Range const &r : Wide_ranges){
		if(cp >= r.lo && cp <= r.hi){
			return true;
		}
	}

	return false;
}

// 행의 표시 폭 (§1.1): 탭=4, 전각=2, 그 외=1. UTF-8 을 코드포인트로 디코드해 잰다.
static auto Display_width(std::string const &line)->std::size_t{
	std::size_t width = 0;
	std::size_t i = 0;
	std::size_t const n = line.size();

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

		out.push_back(Violation{ row, 100, "1.1", msg });
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
		out.push_back(Violation{ row, p, "1.2", "space in indentation" });
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
		out.push_back(Violation{ row, p, "8.2", "trailing whitespace" });
	}
}

// §3 금지 키워드 typedef/goto (@마스크, 단어 경계).
static void Check_banned(std::string const &mask, int const row, std::vector<Violation> &out){
	static std::string const Banned[] = { "typedef", "goto" };

	for(std::string const &kw : Banned){
		std::size_t pos = mask.find(kw);
		while(pos != std::string::npos){
			std::size_t const end = pos + kw.size();
			bool const left_ok = pos == 0 || !::is_word_char(mask[pos - 1]);
			bool const right_ok = end >= mask.size() || !::is_word_char(mask[end]);

			if(left_ok && right_ok){
				int const col = static_cast<int>(pos);

				out.push_back(Violation{ row, col, "3", "banned keyword: " + kw });
			}

			pos = mask.find(kw, pos + 1);
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
		::Check_banned(mask[row], row, out);
	}

	return out;
}
