/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "Test_Blank_Line.hpp"

#include "../sak_case.hpp"

//	Generated from the sak sample battery (v3.3.0 rules). Snippets are byte-exact.

static auto Intro()->void{
	h2u::mdo
	<< h2u::Title(L"Rule 9.4 - Blank lines and end of file")
	<< L"Blank lines must share their neighbors indentation, and the file must end with a newline. "
	<< L"`sak` flags a missing final newline; well-formed blank lines pass."
	<< h2u::empty_line;
}

static auto Boundary_ok()->void{
	Lines const  
		snippet
		= {
			"void f(bool ready){",
			"\tif(ready){",
			"\t\tstep();",
			"\t}",
			"",
			"\tfinish();",
			"\tdone();",
			"",
			"\tmore();",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"blank lines surrounding a braced block - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Eof_missing_bad()->void{
	Lines const snippet = { "void f(){", "\tg();", "}", "", "int tail;" };

	auto const got = sakt::run_sak(snippet, false);
	auto const want = sakt::expect_one("9.4", 4);

	sakt::render_case(L"a file with no trailing newline - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Eof_ok()->void{
	Lines const snippet = { "void f(){", "\tg();", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"a file that ends with a newline - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Consecutive_blank_ok()->void{
	Lines const snippet = { "void f(){", "\ta();", "", "", "\tb();", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"two consecutive blank lines - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

H2U_HOW2USE_TESTS(sakt::Test_, Blank_Line, /**/){
	::Intro,
	::Boundary_ok,
	::Eof_missing_bad,
	::Eof_ok,
	::Consecutive_blank_ok
};
