/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "Test_Indent.hpp"

#include "../sak_case.hpp"

//	Rules 1.3 / 8.2 - a run of four spaces reads as one tab. Snippets are byte-exact.

static auto Intro()->void{
	h2u::mdo
	<< h2u::Title(L"Rules 1.3 / 8.2 - Indentation units (tab or four spaces)")
	<< L"A run of four spaces reads as one tab (Rule 8.2), so indentation may be spelled with "
	<< L"tabs, four-space groups, or a mix. Leftover 1-3 spaces in the line head are flagged."
	<< h2u::empty_line;
}

static auto Four_space_ok()->void{
	Lines const snippet = { "void f(){", "    if(x){", "        a();", "    }", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"four-space indentation, depth 1 and 2 - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Mixed_unit_ok()->void{
	Lines const snippet = { "void f(){", "\tif(x){", "\t    a();", "\t}", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"tab and four-space in one line head - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Leftover_six_bad()->void{
	Lines const snippet = { "void f(){", "      g();", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("1.3", 1);

	sakt::render_case(L"six leading spaces - one unit plus leftover", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Leftover_tab_two_bad()->void{
	Lines const snippet = { "void f(){", "\t  g();", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("1.3", 1);

	sakt::render_case(L"tab then two spaces in the head - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Comment_line_ok()->void{
	Lines const snippet = { "void f(){", "    // note", "    g();", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"a four-space indented comment line - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto All_tab_ok()->void{
	Lines const snippet = { "void f(){", "\tg();", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"all-tab indentation - clean (regression)", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

H2U_HOW2USE_TESTS(sakt::Test_, Indent, /**/){
	::Intro,
	::Four_space_ok,
	::Mixed_unit_ok,
	::Leftover_six_bad,
	::Leftover_tab_two_bad,
	::Comment_line_ok,
	::All_tab_ok
};
