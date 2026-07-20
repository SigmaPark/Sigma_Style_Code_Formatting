/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "Test_Spacing.hpp"

#include "../sak_case.hpp"

//	Rule 8.4 attach law - binary-operator spacing and the word/bracket boundary (old 8.5 folded
//	in). sak checks only context-free operators (=, ==, ...); ambiguous +, <, * go to a subagent.

static auto Intro()->void{
	h2u::mdo
	<< h2u::Title(L"Rule 8.4 - Token and bracket-boundary spacing")
	<< L"`sak` checks spacing around context-free binary operators (`=`, `==`, `+=`, ...) "
	<< L"and the word/bracket boundary (`foo()` vs `foo ()`). Ambiguous tokens such as "
	<< L"`+`, `<`, `*` are left to the subagent (they may be unary, template, or pointer)."
	<< h2u::empty_line;
}

static auto Assign_cramped()->void{
	Lines const snippet = { "sum=a;" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_two("8.4", 0, "8.4", 0);

	sakt::render_case(L"`sum=a;` - flagged before and after `=`", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Assign_clean()->void{
	Lines const snippet = { "sum = a;" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"`sum = a;` - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Assign_missing_after()->void{
	Lines const snippet = { "value =base;" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("8.4", 0);

	sakt::render_case(L"`value =base;` - flagged (missing space after `=`)", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Assign_missing_before()->void{
	Lines const snippet = { "value= base;" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("8.4", 0);

	sakt::render_case(L"`value= base;` - flagged (missing space before `=`)", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Call_spaced_paren()->void{
	Lines const snippet = { "notify (result);" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("8.4", 0);

	sakt::render_case(L"`notify (result);` - flagged (space before `(`)", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Call_clean()->void{
	Lines const snippet = { "notify(result);" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"`notify(result);` - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Call_spaced_paren_2()->void{
	Lines const snippet = { "reset (count);" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("8.4", 0);

	sakt::render_case(L"`reset (count);` - flagged (space before `(`)", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

H2U_HOW2USE_TESTS(sakt::Test_, Spacing, /**/){
	::Intro,
	::Assign_cramped,
	::Assign_clean,
	::Assign_missing_after,
	::Assign_missing_before,
	::Call_spaced_paren,
	::Call_clean,
	::Call_spaced_paren_2
};
