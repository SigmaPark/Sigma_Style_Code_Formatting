/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "Test_Clause_Cohesion.hpp"

#include "../sak_case.hpp"

//	Generated from the sak sample battery (v3.3.0 rules). Snippets are byte-exact.

static auto Intro()->void{
	h2u::mdo
	<< h2u::Title(L"Rule 4.3 - Clause cohesion (virtual operator)")
	<< L"The virtual operator after a multi-line body wraps `else` / `catch` / `while` to the next "
	<< L"line; a single-line body keeps the clause glued. `sak` flags a glued multi-line clause."
	<< h2u::empty_line;
}

static auto Else_glued_bad()->void{
	Lines const  
		snippet
		= {
			"void f(bool c){",
			"\tif(c){",
			"\t\tg();",
			"\t} else{",
			"\t\th();",
			"\t}",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("4.3", 3);

	sakt::render_case(L"`}` followed immediately by `else{` - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Else_break_ok()->void{
	Lines const  
		snippet
		= {
			"void f(bool c){",
			"\tif(c){",
			"\t\tg();",
			"\t}",
			"\telse{",
			"\t\th();",
			"\t}",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"`else` on new line after multi-line body - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Else_single_ok()->void{
	Lines const snippet = { "void f(bool c){", "\tif(c){ g(); } else{ h(); }", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"single-line `if/else` structure - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Catch_glued_bad()->void{
	Lines const  
		snippet
		= {
			"void f(){",
			"\ttry{",
			"\t\tg();",
			"\t} catch(...){",
			"\t\th();",
			"\t}",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("4.3", 3);

	sakt::render_case(L"`}` followed immediately by `catch(...){` - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Do_while_glued_bad()->void{
	Lines const snippet = { "void f(){", "\tdo{", "\t\tg();", "\t} while(c());", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("4.3", 3);

	sakt::render_case(L"`}` followed immediately by `while(c());` - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Cohesion_single_ok()->void{
	Lines const  
		snippet
		= {
			"void f(bool c){",
			"\tif(c){ g(); } else{ h(); }",
			"\tdo{ g(); } while(c);",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"single-line `if/else` and `do-while` - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Comment_else_ok()->void{
	Lines const  
		snippet
		= {
			"void f(bool c){",
			"\tif(c){",
			"\t\tg();",
			"\t}",
			"\t/*note*/ else{",
			"\t\th();",
			"\t}",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"a comment preceding an `else` - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Plain_while_ok()->void{
	Lines const  
		snippet
		= {
			"void f(){",
			"\tint i = 0;",
			"",
			"\t{",
			"\t\tint t = 0;",
			"\t}",
			"",
			"\twhile(i != 3){",
			"\t\t++i;",
			"\t}",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"a standalone `while` loop - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

H2U_HOW2USE_TESTS(sakt::Test_, Clause_Cohesion, /**/){
	::Intro,
	::Else_glued_bad,
	::Else_break_ok,
	::Else_single_ok,
	::Catch_glued_bad,
	::Do_while_glued_bad,
	::Cohesion_single_ok,
	::Comment_else_ok,
	::Plain_while_ok
};
