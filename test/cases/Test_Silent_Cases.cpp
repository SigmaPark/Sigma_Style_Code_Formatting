/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "Test_Silent_Cases.hpp"

#include "../sak_case.hpp"

//	Generated from the sak sample battery (v3.3.0 rules). Snippets are byte-exact.

static auto Intro()->void{
	h2u::mdo
	<< h2u::Title(L"Conformance - constructs that look suspicious but pass")
	<< L"These constructs resemble flagged patterns but conform to the convention, so `sak` stays "
	<< L"silent on every one of them."
	<< h2u::empty_line;
}

static auto New_struct_mul_ok()->void{
	Lines const  
		snippet
		= {
			"struct S{",
			"\tint v;",
			"};",
			"",
			"auto Make(S *q)->S *{",
			"\tauto  ",
			"\t\tr",
			"\t\t= new struct S{ 1 }",
			"\t\t* q",
			"\t;",
			"",
			"\treturn r;",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"`new struct S{ 1 } * q` inside an expression - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Members_ok()->void{
	Lines const snippet = { "class Foo{", "public:", "\tvoid f(){ g(); }", "\tint x = 0;", "};" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"a class with public members - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Lock_scope_ok()->void{
	Lines const  
		snippet
		= {
			"void f(){",
			"\t{",
			"\t\tint tmp = 0;",
			"\t}",
			"",
			"\t{",
			"\t\tint tmp = 1;",
			"\t}",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"two bare scope blocks - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Attribute_ok()->void{
	Lines const snippet = { "void f(){", "\tg();", "}", "", "[[nodiscard]] auto h()->int;" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"a `[[nodiscard]]` attribute on a declaration - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Preproc_else_ok()->void{
	Lines const snippet = { "void f(){", "#ifdef X", "\tg();", "#else", "\th();", "#endif", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"`#else` in a preprocessor block - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Return_brace_ok()->void{
	Lines const snippet = { "auto make()->Foo{", "\treturn{ 1, 2 };", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"`return{ 1, 2 };` - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Trailing_single_ok()->void{
	Lines const  
		snippet
		= {
			"class C{",
			"public:",
			"\tauto size() const->int{ return _n; }",
			"\tauto has() const->bool{ return _n != 0; }",
			"",
			"private:",
			"\tint _n = 0;",
			"};"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"single-line member functions - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Brace_comma_args_ok()->void{
	Lines const  
		snippet
		= {
			"void f(){",
			"\tbuild(",
			"\t\tStyle{ 1, 2 },",
			"\t\tStyle{ 3, 4 }",
			"\t);",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"braced args in a multi-line call - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Fingerprint_ok()->void{
	Lines const  
		snippet
		= {
			"void f(){",
			"\tresult",
			"\t= Won{ 1 } + x;",
			"",
			"\tauto t = Won{ 2 } + x;",
			"\tuse(t);",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"`= Won{ 1 } + x;` after `result` - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

H2U_HOW2USE_TESTS(sakt::Test_, Silent_Cases, /**/){
	::Intro,
	::New_struct_mul_ok,
	::Members_ok,
	::Lock_scope_ok,
	::Attribute_ok,
	::Preproc_else_ok,
	::Return_brace_ok,
	::Trailing_single_ok,
	::Brace_comma_args_ok,
	::Fingerprint_ok
};
