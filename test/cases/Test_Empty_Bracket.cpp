/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "Test_Empty_Bracket.hpp"

#include "../sak_case.hpp"

//	Rule 8.5 empty bracket pairs (old 8.6) - a single-line pair holding only spaces loses them
//	and never counts toward a nesting stage; a brace pair is no exception.

static auto Intro()->void{
	h2u::mdo
	<< h2u::Title(L"Rule 8.5 - Empty bracket pairs")
	<< L"A single-line pair holding nothing but spaces loses those spaces (`f( )` becomes "
	<< L"`f()`, `Foo{ }` becomes `Foo{}`), and an empty pair never counts toward the nesting "
	<< L"stage of the bracket that encloses it."
	<< h2u::empty_line;
}

static auto Paren_spaced()->void{
	Lines const snippet = { "f( );" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_two("8.5", 0, "8.5", 0);

	sakt::render_case(L"`f( );` - flagged on both sides of the empty pair", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Paren_clean()->void{
	Lines const snippet = { "f();" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"`f();` - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Brace_spaced()->void{
	Lines const snippet = { "Foo{ };" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_two("8.5", 0, "8.5", 0);

	sakt::render_case(L"`Foo{ };` - flagged (a brace pair is no exception)", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Brace_clean()->void{
	Lines const snippet = { "Foo{};" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"`Foo{};` - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Nested_empty_not_counted()->void{
	Lines const snippet = { "foo(bar());" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"`foo(bar());` - the empty pair adds no nesting stage", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

H2U_HOW2USE_TESTS(sakt::Test_, Empty_Bracket, /**/){
	::Intro,
	::Paren_spaced,
	::Paren_clean,
	::Brace_spaced,
	::Brace_clean,
	::Nested_empty_not_counted
};
