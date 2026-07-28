/*  SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#pragma once

#include <string>
#include <vector>

#include "check.hpp"
#include "lexer.hpp"

// The checker now lives in namespace sak. A case file reads better with the three names it
// handles constantly kept bare, so they are pulled in here once for the whole suite.
using sak::Lines;
using sak::V_cat;
using sak::Violation;

namespace sakt{

	// A snippet is held directly as a list of lines (Lines) - each line is an explicit string
	// literal, so the bytes are exact (indentation inside a snippet is the '\t' within a line
	// string) with no structural misread from a raw string's flush-left layout. sak lexes and
	// checks these lines as-is (over the Rule 2 string-literal exemption).
	//
	// final_newline - whether the file ends with a newline (Rule 9.4). The default is true (a
	// normal file); only a snippet that tests a missing EOF newline passes false.
	auto run_sak(Lines const &snippet, bool final_newline = true)->std::vector<Violation>;

	// One expected finding - a rule tag (the spec section, e.g. "8.4"), a 0-based row, and
	// a category (violation / suspect).
	struct Expect{
		std::string rule;
		int row;
		V_cat cat = V_cat::violation;
	};

	// Expectation builders - wrapped in functions so a case reads on one line inside H2U_ASSERT
	// without nested brackets (Rule 8.5). Use expect_two / expect_three when one snippet yields
	// several findings at once, and expect_suspect where a notation clash (Rule 8.4) is a suspect.
	auto expect_none()->std::vector<Expect>;
	auto expect_one(std::string rule, int row)->std::vector<Expect>;
	auto expect_two(std::string rule1, int row1, std::string rule2, int row2)->std::vector<Expect>;

	auto expect_three(
		std::string rule1, int row1, std::string rule2, int row2, std::string rule3, int row3
	)->std::vector<Expect>;

	auto expect_suspect(std::string rule, int row)->std::vector<Expect>;

	// Whether the actual findings match the expectation exactly (a (rule, row, cat) multiset,
	// order-independent).
	auto matches(std::vector<Violation> const &got, std::vector<Expect> const &want)->bool;

	// Render a case into the living coverage document - a subheading, the snippet code block,
	// and the list of findings sak produced.
	auto render_case(
		std::wstring const &title, Lines const &snippet, std::vector<Violation> const &got
	)->void;
}
