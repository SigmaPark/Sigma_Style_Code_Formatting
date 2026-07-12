/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "Test_Bracket_Adjacency.hpp"

#include "../sak_case.hpp"

//	§8.5 의 v2.2.0 두 항 — 닫는 괄호와 여는 괄호 사이에는 공백을 두지 않고(앞의 닫는 괄호까지가
//	하나의 피연산 토큰이고 그 뒤에서 새 괄호가 열린다), 인접한 두 비기호형 토큰 사이는 정확히 1칸.

static auto Intro()->void{
	h2u::mdo
	<< h2u::Title(L"Rule 8.5 - Bracket adjacency and word spacing")
	<< L"A closing bracket followed by an opening one takes no space between them "
	<< L"(`){`, `)(`, `][`), and two neighbouring non-symbolic tokens are separated by "
	<< L"exactly one space."
	<< h2u::empty_line;
}

static auto Close_open_spaced()->void{
	Lines const snippet = { "if(cond) {" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("8.5", 0);

	sakt::render_case(L"`if(cond) {` - flagged (space between `)` and `{`)", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Call_chain_spaced()->void{
	Lines const snippet = { "func(a) (b);" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("8.5", 0);

	sakt::render_case(L"`func(a) (b);` - flagged (space between `)` and `(`)", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Call_chain_clean()->void{
	Lines const snippet = { "func(a)(b);" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"`func(a)(b);` - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Subscript_chain_spaced()->void{
	Lines const snippet = { "table[i] [j];" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("8.5", 0);

	sakt::render_case(L"`table[i] [j];` - flagged (space between `]` and `[`)", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Words_padded()->void{
	Lines const snippet = { "int  count = 0;" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("8.5", 0);

	sakt::render_case(L"`int  count = 0;` - flagged (two spaces between words)", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Words_clean()->void{
	Lines const snippet = { "int count = 0;" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"`int count = 0;` - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

H2U_HOW2USE_TESTS(sakt::Test_, Bracket_Adjacency, /**/){
	::Intro,
	::Close_open_spaced,
	::Call_chain_spaced,
	::Call_chain_clean,
	::Subscript_chain_spaced,
	::Words_padded,
	::Words_clean
};
