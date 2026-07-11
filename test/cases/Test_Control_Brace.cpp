/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "Test_Control_Brace.hpp"

#include "../sak_case.hpp"

//	Rule 3 — 제어문 중괄호 생략 금지. sak 은 if/else/for/while/switch/do-while 본체의 중괄호
//	생략을 잡아내고, 이미 중괄호가 있으면 통과시킨다. 스니펫은 Lines 로 바이트 그대로 심는다.

static auto Intro()->void{
	h2u::mdo
	<< h2u::Title(L"Rule 3 - Control-flow braces")
	<< L"The convention forbids omitting braces on any control-flow body "
	<< L"(`if` / `else` / `for` / `while` / `switch` / `do`-`while`). "
	<< L"`sak` flags the omission and passes code that already carries them."
	<< h2u::empty_line;
}

static auto If_without_brace()->void{
	Lines const snippet = { "if(ready) run();" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("3", 0);

	sakt::render_case(L"`if` without braces - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto If_with_brace()->void{
	Lines const snippet = { "if(ready){ run(); }" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"`if` with braces - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto While_without_brace()->void{
	Lines const snippet = { "while(go) tick();" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("3", 0);

	sakt::render_case(L"`while` without braces - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto For_multiline_clean()->void{
	Lines const snippet = { "for(int i = 0; i != n; ++i){", "\tstep(i);", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"multi-line `for` with braces - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

H2U_HOW2USE_TESTS(sakt::Test_, Control_Brace, /**/){
	::Intro,
	::If_without_brace,
	::If_with_brace,
	::While_without_brace,
	::For_multiline_clean
};
