/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "Test_Paren_Head.hpp"

#include "../sak_case.hpp"

//	Generated from the sak sample battery (v3.3.0 rules). Snippets are byte-exact.

static auto Intro()->void{
	h2u::mdo
	<< h2u::Title(L"Rules 4.3 / 9.3 - Splits notation cannot settle")
	<< L"A newline between `)` and a word, or between `}` and `(`, may be a macro / IIFE or a new "
	<< L"statement. Notation cannot decide, so `sak` marks these as suspects, not hard violations."
	<< h2u::empty_line;
}

static auto Paren_word_suspect()->void{
	Lines const snippet = { "TEST_MACRO(foo)", "static void Helper(){", "\trun();", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_suspect("4.3", 1);

	sakt::render_case(L"closing `)` before a word on the next line - suspect", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Paren_blank_suspect()->void{
	Lines const snippet = { "TEST_MACRO(foo)", "", "static void Helper(){", "\trun();", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_suspect("4.3", 2);

	sakt::render_case(L"closing `)` before a word after a blank line - suspect", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Iife_split_suspect()->void{
	Lines const snippet = { "void f(){", "\t[](int x){ use(x); }", "\t(5);", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_suspect("9.3", 2);

	sakt::render_case(L"`}` before `(` on the next line (IIFE?) - suspect", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Stmt_paren_suspect()->void{
	Lines const snippet = { "void f(){", "\tif(g()){", "\t\th();", "\t}", "", "\t(run)();", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_suspect("9.3", 5);

	sakt::render_case(L"`}` blank line before `(run)();` - suspect", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Chain_ok()->void{
	Lines const  
		snippet
		= {
			"void f(){",
			"\tobj.query(",
			"\t\ta, b",
			"\t).filter(x).first()",
			"\t+ fallback;",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"a chained call after a multi-line call - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Attach_ok()->void{
	Lines const  
		snippet
		= {
			"void f(){",
			"\tauto v = Foo{ 1 };",
			"\tuse(v);",
			"\t[](int x){ use(x); }(5);",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"braced init and IIFE attached on one line - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

H2U_HOW2USE_TESTS(sakt::Test_, Paren_Head, /**/){
	::Intro,
	::Paren_word_suspect,
	::Paren_blank_suspect,
	::Iife_split_suspect,
	::Stmt_paren_suspect,
	::Chain_ok,
	::Attach_ok
};
