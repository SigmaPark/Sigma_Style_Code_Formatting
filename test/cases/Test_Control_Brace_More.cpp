/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "Test_Control_Brace_More.hpp"

#include "../sak_case.hpp"

//	Rule 3, further cases. The condition/statement filler is mass-produced by the junior
//	model Yeon; byte-exact stamping and the expected verdicts are the author's. sak scores them.

static auto If_ident_no_brace()->void{
	Lines const snippet = { "if(ready) run();" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("3", 0);

	sakt::render_case(L"`if(ready)` no braces - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto While_ident_no_brace()->void{
	Lines const snippet = { "while(done) reset();" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("3", 0);

	sakt::render_case(L"`while(done)` no braces - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto If_incdec_no_brace()->void{
	Lines const snippet = { "if(active) ++count;" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("3", 0);

	sakt::render_case(L"`if(active) ++count;` no braces - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto While_assign_no_brace()->void{
	Lines const snippet = { "while(has_next) x = 1;" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("3", 0);

	sakt::render_case(L"`while(has_next) x = 1;` no braces - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto If_ident_braced()->void{
	Lines const snippet = { "if(valid){ notify(); }" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"`if(valid){ notify(); }` - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto While_ident_braced()->void{
	Lines const snippet = { "while(retry){ sleep(1000); }" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"`while(retry){ sleep(1000); }` - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

H2U_HOW2USE_TESTS(sakt::Test_, Control_Brace_More, /**/){
	::If_ident_no_brace,
	::While_ident_no_brace,
	::If_incdec_no_brace,
	::While_assign_no_brace,
	::If_ident_braced,
	::While_ident_braced
};
