/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "Test_Inline_Type.hpp"

#include "../sak_case.hpp"

//	Generated from the sak sample battery (v3.3.0 rules). Snippets are byte-exact.

static auto Intro()->void{
	h2u::mdo
	<< h2u::Title(L"Rule 5.5 - Inline types and buried declarators")
	<< L"An inline type definition or a buried declarator that spans lines must expand its corner "
	<< L"bracket (type alone with the marker, declarators one level deeper). `sak` flags the glue."
	<< h2u::empty_line;
}

static auto Inline_star_bad()->void{
	Lines const snippet = { "struct S{", "\tint x;", "}", "*p;" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_two("5.5", 2, "5.5", 3);

	sakt::render_case(
		L"inline struct then `*p;` with a wrong marker and indent - flagged", snippet, got
	);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Inline_fnptr_bad()->void{
	Lines const snippet = { "struct S{", "\tint x;", "}", "(*fp)(int);" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_three("9.4", 2, "5.5", 2, "5.5", 3);

	sakt::render_case(L"inline struct then `(*fp)(int);` function pointer - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Inline_type_ok()->void{
	Lines const  
		snippet
		= {
			"struct S{",
			"\tint x;",
			"}  ",
			"\t*p",
			";",
			"",
			"struct T{",
			"\tint y;",
			"}  ",
			"\tq",
			";"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"inline struct with the proper marker - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Declarator_glued_bad()->void{
	Lines const snippet = { "void f(){", "\tstd::map<", "\t\tint, int", "\t>::iterator it;", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("5.5", 3);

	sakt::render_case(
		L"multi-line `std::map<...>::iterator it;` unexpanded - flagged", snippet, got
	);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Declarator_call_ok()->void{
	Lines const snippet = { "void run(){", "\tfoo<", "\t\tint", "\t>::go();", "}" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"multi-line call `foo<...>::go()` - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Inline_list_bad()->void{
	Lines const snippet = { "struct S{", "\tint x;", "\tdouble y;", "} v1, v2;" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("5.5", 3);

	sakt::render_case(L"`} v1, v2;` inline type unexpanded - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Inline_single_bad()->void{
	Lines const snippet = { "struct S{", "\tint x;", "\tdouble y;", "} v;" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("5.5", 3);

	sakt::render_case(L"`} v;` inline type single declarator - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Cbracket_shield_ok()->void{
	Lines const  
		snippet
		= {
			"struct S{",
			"\tint x;",
			"}  ",
			"\tv1, v2",
			";",
			"",
			"Foo::Foo(int a, int b)",
			": _x(a), _y(b){",
			"}",
			"",
			"class Bar",
			": public Base_a, public Base_b{",
			"\tvoid f();",
			"};"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"inline struct, ctor init-list and inheritance - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Angle_decl_glued_bad()->void{
	Lines const  
		snippet
		= {
			"void run(){",
			"\tstd::map<",
			"\t\tint, int",
			"\t>::iterator a, b;",
			"}"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("5.5", 3);

	sakt::render_case(
		L"`>::iterator a, b;` multi-line declaration unexpanded - flagged", snippet, got
	);

	H2U_ASSERT( sakt::matches(got, want) );
}

//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

H2U_HOW2USE_TESTS(sakt::Test_, Inline_Type, /**/){
	::Intro,
	::Inline_star_bad,
	::Inline_fnptr_bad,
	::Inline_type_ok,
	::Declarator_glued_bad,
	::Declarator_call_ok,
	::Inline_list_bad,
	::Inline_single_bad,
	::Cbracket_shield_ok,
	::Angle_decl_glued_bad
};
