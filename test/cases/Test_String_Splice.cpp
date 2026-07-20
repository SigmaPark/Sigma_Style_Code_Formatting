/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "Test_String_Splice.hpp"

#include "../sak_case.hpp"

//	Generated from the sak sample battery (v3.3.0 rules). Snippets are byte-exact.

static auto Intro()->void{
	h2u::mdo
	<< h2u::Title(L"Rule 9.1 - Adjacent string literals")
	<< L"Adjacent string literals may break across a line only inside a multi-line bracket; a bare "
	<< L"splice must be wrapped in parentheses. `sak` flags the unwrapped splice."
	<< h2u::empty_line;
}

static auto Splice_bare_bad()->void{
	Lines const snippet = { "void f(){", "\tstd::cout", "\t<< \"aaa \"", "\t\"bbb\";", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("9.1", 3);

	sakt::render_case(L"adjacent string literals split, unwrapped - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Splice_blank_ok()->void{
	Lines const snippet = { "void f(){", "\tstd::cout", "\t<< \"aaa \"", "", "\t\"bbb\";", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"adjacent string literals split across a blank line - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Splice_paren_ok()->void{
	Lines const  
		snippet
		= {
			"void f(){",
			"\tlog_message(",
			"\t\t\"aaa \"",
			"\t\t\"bbb\"",
			"\t);",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"adjacent string literals inside a multi-line call - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Splice_return_ok()->void{
	Lines const  
		snippet
		= {
			"auto join()->char const *{",
			"\treturn",
			"\t\t\"aaa \"",
			"\t\t\"bbb\"",
			"\t\t;",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"adjacent string literals in a multi-line return - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Splice_vardecl_ok()->void{
	Lines const  
		snippet
		= {
			"void f(){",
			"\tchar const  ",
			"\t\t*msg",
			"\t\t= \"aaa \"",
			"\t\t\"bbb\"",
			"\t;",
			"",
			"\tuse(msg);",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"adjacent string literals in a variable declaration - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

H2U_HOW2USE_TESTS(sakt::Test_, String_Splice, /**/){
	::Intro,
	::Splice_bare_bad,
	::Splice_blank_ok,
	::Splice_paren_ok,
	::Splice_return_ok,
	::Splice_vardecl_ok
};
