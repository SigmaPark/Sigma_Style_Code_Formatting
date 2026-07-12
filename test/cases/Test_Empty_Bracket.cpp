/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "Test_Empty_Bracket.hpp"

#include "../sak_case.hpp"

//	§8.6 내용 없는 괄호쌍 (v2.2.0) — 단일행 괄호 안이 모두 공백뿐이면 그 공백을 제거하고,
//	중첩 단계에도 세지 않는다. 중괄호도 예외가 아니다(n = 단계 + 1 이 빈 쌍에는 적용되지 않는다).

static auto Intro()->void{
	h2u::mdo
	<< h2u::Title(L"Rule 8.6 - Empty bracket pairs")
	<< L"A single-line pair holding nothing but spaces loses those spaces (`f( )` becomes "
	<< L"`f()`, `Foo{ }` becomes `Foo{}`), and an empty pair never counts toward the nesting "
	<< L"stage of the bracket that encloses it."
	<< h2u::empty_line;
}

static auto Paren_spaced()->void{
	Lines const snippet = { "f( );" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_two("8.6", 0, "8.6", 0);

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
	auto const want = sakt::expect_two("8.6", 0, "8.6", 0);

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
