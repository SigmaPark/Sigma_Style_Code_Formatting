/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "Test_Template_Header.hpp"

#include "../sak_case.hpp"

//	§9.3 (v2.2.0) — 템플릿 헤더의 닫는 꺾쇠 뒤에는 단일행·다중행을 불문하고 반드시 개행.
//	행끝 주석은 §2 제외 대상이라 닫는 `>` 뒤에 남아도 무방하다.

static auto Intro()->void{
	h2u::mdo
	<< h2u::Title(L"Rule 9.3 - A newline after the template header")
	<< L"The closing angle of a template header is followed by a newline, whether the header "
	<< L"is written on one line or spread over several. A trailing comment may still follow "
	<< L"it, since Rule 2 exempts comments."
	<< h2u::empty_line;
}

static auto Header_joined()->void{
	Lines const snippet = { "template<class T> auto twice(T x)->T;" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_one("9.3", 0);

	sakt::render_case(L"header joined to the declaration - flagged", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Header_broken()->void{
	Lines const snippet = { "template<class T>", "auto twice(T x)->T;" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"header on its own line - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}

static auto Header_with_comment()->void{
	Lines const snippet = { "template<class T> // a comment may follow", "auto twice(T x)->T;" };

	auto const got = sakt::run_sak(snippet);
	auto const want = sakt::expect_none();

	sakt::render_case(L"header followed by a comment - clean", snippet, got);

	H2U_ASSERT( sakt::matches(got, want) );
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

H2U_HOW2USE_TESTS(sakt::Test_, Template_Header, /**/){
	::Intro,
	::Header_joined,
	::Header_broken,
	::Header_with_comment
};
