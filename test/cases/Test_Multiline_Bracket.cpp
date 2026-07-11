/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "Test_Multiline_Bracket.hpp"

#include "../sak_case.hpp"

//	§5.4 다중행 괄호 — 여는 괄호는 행의 마지막, 닫는 괄호는 행의 처음, 여닫는 행 들여쓰기는 같고
//	중간 행은 한 단계 깊게. 함수·인자 필러(compute/run 등)는 후배 모델 Yeon 산출.

static auto Intro()->void{
	h2u::mdo
	<< h2u::Title(L"Rule 5.4 - Multi-line brackets")
	<< L"When a bracket spans lines, the opening bracket must be the last token on its line, "
	<< L"the closing bracket the first on its line, the two must share indentation, and the "
	<< L"middle lines sit one level deeper. `sak` flags each deviation."
	<< h2u::empty_line;
}

static auto Multiline_clean()->void{
	Lines const snippet = { "compute(", "\ta, b", ")" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"well-formed multi-line call - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Close_not_first()->void{
	Lines const snippet = { "compute(", "\ta, b);" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_two("5.4", 1, "5.4", 1);

	sakt::render_case(L"closing `)` not first token - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Open_not_last()->void{
	Lines const snippet = { "run(z,", "\tdelta", ")" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("5.4", 0);

	sakt::render_case(L"opening `(` not last token - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Multiline_clean_2()->void{
	Lines const snippet = { "multiply(", "\tz, base", ")" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"another well-formed multi-line call - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Close_not_first_2()->void{
	Lines const snippet = { "add(", "\ta, b);" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_two("5.4", 1, "5.4", 1);

	sakt::render_case(L"closing `)` glued to arguments - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

H2U_HOW2USE_TESTS(sakt::Test_, Multiline_Bracket, /**/){
	::Intro,
	::Multiline_clean,
	::Close_not_first,
	::Open_not_last,
	::Multiline_clean_2,
	::Close_not_first_2
};
