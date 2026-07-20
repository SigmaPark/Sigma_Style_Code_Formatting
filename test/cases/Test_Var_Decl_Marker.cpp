/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "Test_Var_Decl_Marker.hpp"

#include "../sak_case.hpp"

//	Generated from the sak sample battery (v3.3.0 rules). Snippets are byte-exact.

static auto Intro()->void{
	h2u::mdo
	<< h2u::Title(L"Rule 5.5 - Variable-declaration marker")
	<< L"A variable declaration that wraps its type to the declarator needs the two-space marker; "
	<< L"without it a word wrapping to a word is flagged (blank lines do not license the split)."
	<< h2u::empty_line;
}

static auto Type_blank_bad()->void{
	Lines const snippet = { "void f(){", "\tint const", "", "\tx = 0;", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("5.5", 1);

	sakt::render_case(
		L"`int const` then a blank line then `x` without marker - flagged", snippet, got
	);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Word_word_bad()->void{
	Lines const snippet = { "void f(){", "\tint const", "\tx = 0;", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("5.5", 1);

	sakt::render_case(L"`int const` wrapping to `x` without the marker - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

H2U_HOW2USE_TESTS(sakt::Test_, Var_Decl_Marker, /**/){
	::Intro,
	::Type_blank_bad,
	::Word_word_bad
};
