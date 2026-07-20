/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "Test_Operator_Name.hpp"

#include "../sak_case.hpp"

//	Rule 4.1 - `operator` and the token(s) after it form one non-symbolic token (a function
//	name), so the symbols inside are not operators and the Rule 8.4 spacing does not reach them.

static auto Intro()->void{
	h2u::mdo
	<< h2u::Title(L"Rule 4.1 - An `operator` name is one token")
	<< L"The symbols after the `operator` keyword are part of a function name, not operators, "
	<< L"so the spacing rules of Rule 8.4 do not reach inside them - `operator<=>` stays "
	<< L"as written."
	<< h2u::empty_line;
}

static auto Spaceship()->void{
	Lines const snippet = { "auto operator<=>(Foo const &) const = default;" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"`operator<=>` - clean (not read as a comparison)", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Call_operator()->void{
	Lines const snippet = { "auto operator()(int i)->int;" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"`operator()` - clean (the empty pair is the name)", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Subscript_operator()->void{
	Lines const snippet = { "auto operator[](int i)->int;" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"`operator[]` - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

H2U_HOW2USE_TESTS(sakt::Test_, Operator_Name, /**/){
	::Intro,
	::Spaceship,
	::Call_operator,
	::Subscript_operator
};
