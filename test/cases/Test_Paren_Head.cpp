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
	<< L"A newline between `)` and a word rides the virtual operator, so `sak` adjudicates it by "
	<< L"break competition (Rule 9.2): a single-line token that outranks the virtual operator in "
	<< L"the two rows makes the split a hard violation, no competitor makes it a licensed break, "
	<< L"and blank lines in between stay suspects. A newline between `}` and `(` (IIFE or a new "
	<< L"statement) still cannot be settled by notation, so it stays a suspect."
	<< h2u::empty_line;
}

static auto Paren_word_licensed()->void{
	Lines const snippet = { "TEST_MACRO(foo)", "static void Helper(){", "\trun();", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"closing `)` before a word, no competitor - licensed break", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Paren_word_outranked()->void{
	Lines const snippet = { "void f(){", "\tTEST_MACRO(foo)", "\tint x = 0;", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("4.3", 2);

	sakt::render_case(
		L"closing `)` before a word, single-line `=` outranks the break - violation", snippet, got
	);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Qualifier_split_licensed()->void{
	Lines const snippet = { "auto g(int x)", "noexcept;" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"qualifier after a single-line signature - licensed break", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Glued_pure_virtual_outranked()->void{
	Lines const snippet = { "struct A{", "\tvirtual auto f(int x)", "\tconst = 0;", "};" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("4.3", 2);

	sakt::render_case(
		L"`const = 0;` after the break - `=` must drop to its own row (Rule 4.3)", snippet, got
	);

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
	::Paren_word_licensed,
	::Paren_word_outranked,
	::Qualifier_split_licensed,
	::Glued_pure_virtual_outranked,
	::Paren_blank_suspect,
	::Iife_split_suspect,
	::Stmt_paren_suspect,
	::Chain_ok,
	::Attach_ok
};
