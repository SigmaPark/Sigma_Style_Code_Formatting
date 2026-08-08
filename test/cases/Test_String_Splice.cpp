/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "Test_String_Splice.hpp"

#include "../sak_case.hpp"

//	Generated from the sak sample battery (v3.7.0 rules). Snippets are byte-exact.

static auto Intro()->void{
	h2u::mdo
	<< h2u::Title(L"Rule 4.4 - String literal splicer")
	<< L"Adjacent string literals are joined by the splicer - an unprefixed empty string literal. "
	<< L"It is written only in a multi-line state, where it leads the continuation line; a "
	<< L"single-line adjacency omits it. No wrapping parentheses are required."
	<< h2u::empty_line;
}

static auto Splice_bare_bad()->void{
	Lines const snippet = { "void f(){", "\tstd::cout", "\t<< \"aaa \"", "\t\"bbb\";", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("9.1", 3);

	sakt::render_case(L"continuation line without the splicer - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Splice_bare_ok()->void{
	Lines const  
		snippet
		= { "void f(){", "\tstd::cout", "\t<< \"aaa \"", "\t\"\" \"bbb\";", "}" }
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"splicer leads the continuation, no parentheses - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Splice_blank_bad()->void{
	Lines const  
		snippet
		= { "void f(){", "\tstd::cout", "\t<< \"aaa \"", "", "\t\"bbb\";", "}" }
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("9.1", 4);

	sakt::render_case(L"a blank line does not stand in for the splicer - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Splice_single_line_bad()->void{
	Lines const  
		snippet
		= { "void f(){", "\tstd::cout << \"aaa \" \"\" \"bbb\";", "}" }
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("4.4", 1);

	sakt::render_case(L"splicer written in a single-line adjacency - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Splice_prefixed_bad()->void{
	Lines const  
		snippet
		= { "void f(){", "\tstd::wcout", "\t<< L\"aaa \"", "\tL\"\" L\"bbb\";", "}" }
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("4.4", 3);

	sakt::render_case(L"splicer carrying an encoding prefix - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Splice_paren_ok()->void{
	Lines const  
		snippet
		= {
			"void f(){",
			"\tlog_message(",
			"\t\t\"aaa \"",
			"\t\t\"\" \"bbb\"",
			"\t);",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"splicer inside a multi-line call - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Splice_triple_ok()->void{
	Lines const  
		snippet
		= {
			"void f(){",
			"\tlog_message(",
			"\t\t\"aaa \"",
			"\t\t\"\" \"bbb \"",
			"\t\t\"\" \"ccc\"",
			"\t);",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"three pieces - one splicer per break - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Splice_partial_ok()->void{
	Lines const  
		snippet
		= {
			"void f(){",
			"\tlog_message(",
			"\t\t\"aaa \" \"bbb \"",
			"\t\t\"\" \"ccc\"",
			"\t);",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"only the broken joint takes a splicer - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Splice_lone_empty_ok()->void{
	Lines const snippet = { "void f(){", "\tchar const *empty = \"\";", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"a lone empty literal is an operand, not a splicer - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Splice_return_ok()->void{
	Lines const  
		snippet
		= {
			"auto join()->char const *{",
			"\treturn",
			"\t\t\"aaa \"",
			"\t\t\"\" \"bbb\"",
			"\t\t;",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"splicer in a multi-line return - clean", snippet, got);

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
			"\t\t\"\" \"bbb\"",
			"\t;",
			"",
			"\tuse(msg);",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"splicer in a variable declaration - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

H2U_HOW2USE_TESTS(sakt::Test_, String_Splice, /**/){
	::Intro,
	::Splice_bare_bad,
	::Splice_bare_ok,
	::Splice_blank_bad,
	::Splice_single_line_bad,
	::Splice_prefixed_bad,
	::Splice_paren_ok,
	::Splice_triple_ok,
	::Splice_partial_ok,
	::Splice_lone_empty_ok,
	::Splice_return_ok,
	::Splice_vardecl_ok
};
