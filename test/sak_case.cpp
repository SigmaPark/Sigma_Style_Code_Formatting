/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "sak_case.hpp"

#include <algorithm>
#include <utility>

#include "How2use.hpp"

namespace sakt{

	// ASCII 문자열을 wide 로 넓힌다 — 스니펫·규칙 메시지는 ASCII 라 로케일과 무관히 안전하다.
	static auto Widen(std::string const &s)->std::wstring{
		return{ s.begin(), s.end() };
	}

	auto run_sak(Lines const &snippet)->std::vector<Violation>{
		Seg_lines const segs = scan_lines(snippet);

		return check_lines(snippet, segs);
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
	//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

	auto matches(std::vector<Violation> const &got, std::vector<Expect> const &want)->bool{
		if(got.size() != want.size()){
			return false;
		}

		using Key = std::pair<std::string, int>;

		std::vector<Key> a, b;

		for(Violation const &v : got){
			a.push_back({ v.rule, v.row });
		}

		for(Expect const &e : want){
			b.push_back({ e.rule, e.row });
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
		} else{
			h2u::mdo << h2u::newl;

			for(Violation const &v : got){
				int const line_no = v.row + 1;

				h2u::mdo
				<< L"- `[" << Widen(v.rule) << L"]` line " << line_no << L": "
				<< Widen(v.message) << h2u::newl;
			}
		}

		h2u::mdo << h2u::empty_line;
	}
}
