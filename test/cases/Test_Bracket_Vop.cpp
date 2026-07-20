/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "Test_Bracket_Vop.hpp"

#include "../sak_case.hpp"

//	Generated from the sak sample battery (v3.3.0 rules). Snippets are byte-exact.

static auto Intro()->void{
	h2u::mdo
	<< h2u::Title(L"Rule 4.3 - Closing bracket before a word")
	<< L"A multi-line `>` / `]]` / template header must end its line: the virtual operator wraps a "
	<< L"following word; a single-line header stays on one line. `sak` flags the multi-line glue."
	<< h2u::empty_line;
}

static auto Template_single_ok()->void{
	Lines const snippet = { "template<class T> auto twice(T x)->T;" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"single-line template declaration - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Angle_word_bad()->void{
	Lines const snippet = { "template<", "\tclass A, class B", "> void func();" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("4.3", 2);

	sakt::render_case(L"multi-line template header before a word - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Angle_attach_ok()->void{
	Lines const snippet = { "void run(){", "\tfoo<", "\t\tint, int", "\t>(a, b);", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"multi-line call `foo<...>(a, b)` - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Attr_word_bad()->void{
	Lines const snippet = { "[[", "\tnodiscard", "]] int get();" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("4.3", 2);

	sakt::render_case(L"multi-line attribute before a word - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

H2U_HOW2USE_TESTS(sakt::Test_, Bracket_Vop, /**/){
	::Intro,
	::Template_single_ok,
	::Angle_word_bad,
	::Angle_attach_ok,
	::Attr_word_bad
};
