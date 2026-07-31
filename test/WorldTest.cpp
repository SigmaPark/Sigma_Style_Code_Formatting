/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "WorldTest.hpp"

#include <iostream>

static auto Test()->void;

template<class A, class ...ARGS>
static auto Log_message(A &&a, ARGS &&...args) noexcept->void{
	std::wcout << static_cast<A &&>(a);

	if constexpr( sizeof...(ARGS) != 0 ){
		::Log_message( static_cast<ARGS &&>(args)... );
	}
}

auto wt::Tests(wchar_t const * const module_title) noexcept->bool{
	::Log_message(
		L"//========//========//========//========//=======#\n",
		module_title, L" test Start\n"
	);

	try{
		::Test();

		::Log_message(
			module_title, L" test Complete\n",
			L"//========//========//========//========//=======#\n"
		);

		return true;
	}
	catch(...){
		std::wcout << L"Unexpected Error!\n";
	}

	::Log_message(
		module_title, L" test Failed\n",
		L"//========//========//========//========//=======#\n"
	);

	return false;
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "cases/Test_Control_Brace.hpp"
#include "cases/Test_Control_Brace_More.hpp"
#include "cases/Test_Spacing.hpp"
#include "cases/Test_Multiline_Bracket.hpp"
#include "cases/Test_Empty_Bracket.hpp"
#include "cases/Test_Bracket_Adjacency.hpp"
#include "cases/Test_Comment_Tail.hpp"
#include "cases/Test_Operator_Name.hpp"

#include "cases/Test_Clause_Cohesion.hpp"
#include "cases/Test_Bracket_Vop.hpp"
#include "cases/Test_Paren_Head.hpp"
#include "cases/Test_Var_Decl_Marker.hpp"
#include "cases/Test_Inline_Type.hpp"
#include "cases/Test_String_Splice.hpp"
#include "cases/Test_Blank_Line.hpp"
#include "cases/Test_Label_Rhythm.hpp"
#include "cases/Test_Silent_Cases.hpp"
#include "cases/Test_Indent.hpp"

static auto Test()->void{
	sakt::Test_Control_Brace::test();
	sakt::Test_Control_Brace_More::test();
	sakt::Test_Spacing::test();
	sakt::Test_Multiline_Bracket::test();
	sakt::Test_Empty_Bracket::test();
	sakt::Test_Bracket_Adjacency::test();
	sakt::Test_Comment_Tail::test();
	sakt::Test_Operator_Name::test();

	sakt::Test_Clause_Cohesion::test();
	sakt::Test_Bracket_Vop::test();
	sakt::Test_Paren_Head::test();
	sakt::Test_Var_Decl_Marker::test();
	sakt::Test_Inline_Type::test();
	sakt::Test_String_Splice::test();
	sakt::Test_Blank_Line::test();
	sakt::Test_Label_Rhythm::test();
	sakt::Test_Silent_Cases::test();
	sakt::Test_Indent::test();
}

auto main(int const, char const * const *)->int{
	wchar_t const * const  
		os
#if defined(_WINDOWS_SOLUTION_)
		= L"Windows"
#elif defined(_POSIX_SOLUTION_)
		= L"POSIX"
#else
		= L"Others"
#endif
	;

	return wt::Tests(os), 0;
}
