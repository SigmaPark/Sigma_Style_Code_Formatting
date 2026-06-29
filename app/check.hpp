/*  SPDX-FileCopyrightText: (c) 2020 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#pragma once

#include <string>
#include <vector>

#include "lexer.hpp"

struct Violation{
	int row, col;   // 0-기준 행, 0-기준 바이트 열
	std::string rule, message;
};

// 행들을 규약 검사해 위반을 낸다(§1.1·§1.2는 raw 행, §8.1·§8.2·§3은 @마스크 위에서).
auto check_lines(Lines const &lines, Seg_lines const &segs)->std::vector<Violation>;
