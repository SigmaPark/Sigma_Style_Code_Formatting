/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#pragma once

#include <string>
#include <vector>

#include "check.hpp"
#include "lexer.hpp"

namespace sakt{

	// 스니펫은 행 목록(Lines)으로 직접 담는다 — 각 행이 명시적 문자열 리터럴이라 바이트가 정확하고
	// (스니펫 내부 들여쓰기는 행 문자열 안의 '\t' 로 표현), 원시문자열의 flush-left 로 인한 구조
	// 오인이 없다. sak 은 이 행들을 그대로(§2 문자열 리터럴 면제 위에서) 렉싱·검사한다.
	auto run_sak(Lines const &snippet)->std::vector<Violation>;

	// 기대 위반 한 건 — 규칙 태그(정본 절 번호, 예: "8.4")와 0-기준 행.
	struct Expect{
		std::string rule;
		int row;
	};

	// 기대 목록 생성기 — H2U_ASSERT 안에서 중첩 괄호(§8.6) 없이 한 줄로 쓰도록 함수로 감싼다.
	// 한 스니펫이 같은 자리에 두 위반을 내는 경우(예: §8.4 연산자 앞·뒤)엔 expect_two 를 쓴다.
	auto expect_none()->std::vector<Expect>;
	auto expect_one(std::string rule, int row)->std::vector<Expect>;
	auto expect_two(std::string rule1, int row1, std::string rule2, int row2)->std::vector<Expect>;

	// 실제 위반이 기대와 정확히 일치하는가((rule, row) 다중집합 기준, 순서 무관).
	auto matches(std::vector<Violation> const &got, std::vector<Expect> const &want)->bool;

	// 케이스를 살아있는 커버리지 문서에 렌더한다 — 소제목·스니펫 코드블록·sak 이 낸 위반 목록.
	auto render_case(
		std::wstring const &title, Lines const &snippet, std::vector<Violation> const &got
	)->void;
}
