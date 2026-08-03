/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "Test_Label_Rhythm.hpp"

#include "../sak_case.hpp"

//	Generated from the sak sample battery (v3.3.0 rules). Snippets are byte-exact.

static auto Intro()->void{
	h2u::mdo
	<< h2u::Title(L"Rule 5.6 - Virtual-brace label rhythm")
	<< L"Inside a `switch` or a class body, exactly one blank line precedes each label (and none "
	<< L"before the first). `sak` flags a missing or a spurious blank around a label."
	<< h2u::empty_line;
}

static auto Rhythm_ok()->void{
	Lines const  
		snippet
		= {
			"int f(int k){",
			"\tswitch(k){",
			"\tcase 0:",
			"\t\treturn 1;",
			"",
			"\tcase 1:",
			"\t\treturn 2;",
			"",
			"\tdefault:",
			"\t\treturn 0;",
			"\t}",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"switch labels separated by one blank line - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Rhythm_missing_bad()->void{
	Lines const  
		snippet
		= {
			"int f(int k){",
			"\tswitch(k){",
			"\tcase 0:",
			"\t\treturn 1;",
			"\tcase 1:",
			"\t\treturn 2;",
			"\t}",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("5.6", 4);

	sakt::render_case(L"switch labels with no blank line between - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Rhythm_firstlabel_bad()->void{
	Lines const  
		snippet
		= {
			"int f(int k){",
			"\tswitch(k){",
			"",
			"\tcase 0:",
			"\t\treturn 1;",
			"\t}",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("5.6", 3);

	sakt::render_case(L"a blank line before the first `case` label - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Fallthrough_ok()->void{
	Lines const  
		snippet
		= {
			"int f(int k){",
			"\tswitch(k){",
			"\tcase 0:",
			"\tcase 1:",
			"\t\treturn 1;",
			"",
			"\tdefault:",
			"\t\treturn 0;",
			"\t}",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"`case 0: case 1:` fallthrough labels - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

H2U_HOW2USE_TESTS(sakt::Test_, Label_Rhythm, /**/){
	::Intro,
	::Rhythm_ok,
	::Rhythm_missing_bad,
	::Rhythm_firstlabel_bad,
	::Fallthrough_ok
};
