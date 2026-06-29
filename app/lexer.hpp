/*  SPDX-FileCopyrightText: (c) 2020 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#pragma once

#include <string>
#include <vector>

using Lines = std::vector<std::string>;

// 무효 영역(주석·문자열·문자 리터럴)을 행 단위로 청킹하는 세그먼트.
// 한 물리행은 Segment 들로 빈틈없이 타일링된다(code + 무효 원자).
enum class Seg_kind{ code, comment, string_lit, char_lit, raw_string, preproc };

struct Segment{
	Seg_kind kind;
	int row, col;   // 0-기준 행, 행 내 시작 열(바이트)
	int len;   // 바이트 길이
};

using Seg_lines = std::vector< std::vector<Segment> >;

// 행들을 좌->우 모드 추적으로 스캔하여 행별 세그먼트를 만든다.
auto scan_lines(Lines const &lines)->Seg_lines;

// 검증용 렌더러 — @마스크(기하 보존)와 $TAG$ 덤프(종류).
auto render_mask(Lines const &lines, Seg_lines const &segs)->Lines;
auto render_dump(Lines const &lines, Seg_lines const &segs)->Lines;
