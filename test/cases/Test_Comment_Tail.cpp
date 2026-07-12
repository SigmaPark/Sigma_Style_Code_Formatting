/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "Test_Comment_Tail.hpp"

#include "../sak_case.hpp"

//	§8.3 (v2.2.0) — 잉여공백은 행끝 주석을 걷어낸 뒤의 행에서 잰다. 그래서 §5.5 의 2칸 마커가
//	주석과 나란히 설 수 있다. 정렬용 공백은 주석 바깥에 두면 §8.2(4연속 금지)에 걸리므로 안쪽에.

static auto Intro()->void{
	h2u::mdo
	<< h2u::Title(L"Rule 8.3 - Redundant whitespace and a trailing comment")
	<< L"Redundant whitespace is measured on the line with its trailing comment stripped, "
	<< L"so the two-space var-decl marker of Rule 5.5 survives a comment on the same line. "
	<< L"Padding placed *before* the comment is ordinary whitespace and hits Rule 8.2; "
	<< L"put the alignment inside the comment instead."
	<< h2u::empty_line;
}

static auto Marker_with_comment()->void{
	Lines const
		snippet
		= {
			"int const  // the marker survives the comment",
			"\ta = 0,",
			"\tb = 1",
			";"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"marker + trailing comment - clean, and the anchor is seen", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Marker_with_comment_bad_layout()->void{
	Lines const
		snippet
		= {
			"int const  // the anchor is confirmed, so the layout is checked",
			"\ta = 0,",
			"\tb = 1;"
		}
	;

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("5.5", 2);

	sakt::render_case(L"marker + comment, `;` not alone - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Padding_before_comment()->void{
	Lines const snippet = { "foo();    // aligned the wrong way" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("8.2", 0);

	sakt::render_case(L"four spaces before `//` - flagged by Rule 8.2", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Padding_inside_comment()->void{
	Lines const snippet = { "foo(); //     aligned inside the comment" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"padding inside the comment - clean (Rule 2 exempts it)", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

H2U_HOW2USE_TESTS(sakt::Test_, Comment_Tail, /**/){
	::Intro,
	::Marker_with_comment,
	::Marker_with_comment_bad_layout,
	::Padding_before_comment,
	::Padding_inside_comment
};
