/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "sak_case.hpp"

#include <algorithm>
#include <cstddef>
#include <tuple>
#include <utility>

#include "How2use.hpp"

namespace sakt{

	// Decode UTF-8 to a wide, ASCII-safe string for the coverage-doc stream (which cannot encode
	// wide non-ASCII): an em/en dash becomes '-', any other non-ASCII becomes '?'. The rule
	// messages are English, so this loses nothing meaningful.
	static auto Widen(std::string const &s)->std::wstring{
		std::wstring out;
		std::size_t const n = s.size();
		std::size_t i = 0;

		while(i < n){
			unsigned char const lead = static_cast<unsigned char>(s[i]);
			char32_t cp = lead;
			std::size_t len = 1;

			if(lead >= 0xF0){
				cp = lead & 0x07;
				len = 4;
			}
			else if(lead >= 0xE0){
				cp = lead & 0x0F;
				len = 3;
			}
			else if(lead >= 0xC0){
				cp = lead & 0x1F;
				len = 2;
			}

			for(std::size_t k = 1; k < len && i + k < n; ++k){
				unsigned char const cont = static_cast<unsigned char>(s[i + k]);

				cp = (cp << 6) | (cont & 0x3F);
			}

			i += len;

			wchar_t ch = L'?';

			if(cp < 0x80){
				ch = static_cast<wchar_t>(cp);
			}
			else if(cp == 0x2013 || cp == 0x2014){
				ch = L'-';
			}

			out.push_back(ch);
		}

		return out;
	}

	auto run_sak(Lines const &snippet, bool const final_newline)->std::vector<Violation>{
		Seg_lines const segs = scan_lines(snippet);

		return check_lines(snippet, segs, final_newline);
	}

	auto expect_none()->std::vector<Expect>{
		return{};
	}

	auto expect_one(std::string rule, int row)->std::vector<Expect>{
		Expect e;

		e.rule = std::move(rule);
		e.row = row;

		std::vector<Expect> v;

		v.push_back(e);

		return v;
	}

	auto expect_two(std::string rule1, int row1, std::string rule2, int row2)->std::vector<Expect>{
		Expect a;

		a.rule = std::move(rule1);
		a.row = row1;

		Expect b;

		b.rule = std::move(rule2);
		b.row = row2;

		std::vector<Expect> v;

		v.push_back(a);
		v.push_back(b);

		return v;
	}

	auto expect_three(
		std::string rule1, int row1, std::string rule2, int row2, std::string rule3, int row3
	)->std::vector<Expect>{
		Expect a;

		a.rule = std::move(rule1);
		a.row = row1;

		Expect b;

		b.rule = std::move(rule2);
		b.row = row2;

		Expect c;

		c.rule = std::move(rule3);
		c.row = row3;

		std::vector<Expect> v;

		v.push_back(a);
		v.push_back(b);
		v.push_back(c);

		return v;
	}

	auto expect_suspect(std::string rule, int row)->std::vector<Expect>{
		Expect e;

		e.rule = std::move(rule);
		e.row = row;
		e.cat = V_cat::suspect;

		std::vector<Expect> v;

		v.push_back(e);

		return v;
	}
	//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

	auto matches(std::vector<Violation> const &got, std::vector<Expect> const &want)->bool{
		if(got.size() != want.size()){
			return false;
		}

		using Key = std::tuple<std::string, int, int>;

		std::vector<Key> a, b;

		for(Violation const &v : got){
			a.push_back( { v.rule, v.row, static_cast<int>(v.cat) } );
		}

		for(Expect const &e : want){
			b.push_back( { e.rule, e.row, static_cast<int>(e.cat) } );
		}

		std::sort(a.begin(), a.end());
		std::sort(b.begin(), b.end());

		return a == b;
	}
	//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

	auto render_case(
		std::wstring const &title, Lines const &snippet, std::vector<Violation> const &got
	)->void{
		h2u::mdo << h2u::Title(title, 4);

		{
			h2u::md_block_guard const guard(L"cpp");

			for(std::string const &line : snippet){
				h2u::mdo << Widen(line) << L"\n";
			}
		}

		if(got.empty()){
			h2u::mdo << h2u::newl << L"-> no violations (conforms)" << h2u::newl;
		}
		else{
			h2u::mdo << h2u::newl;

			for(Violation const &v : got){
				int const line_no = v.row + 1;

				std::wstring const tail = v.cat == V_cat::suspect ? L" [suspect]" : L"";

				h2u::mdo
				<< L"- `[" << Widen(v.rule) << L"]` line " << line_no << L": "
				<< Widen(v.message) << tail << h2u::newl;
			}
		}

		h2u::mdo << h2u::empty_line;
	}
}
