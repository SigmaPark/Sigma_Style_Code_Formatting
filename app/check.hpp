/*  SPDX-FileCopyrightText: (c) 2020 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#pragma once

#include <string>
#include <vector>

#include "lexer.hpp"

namespace sak{
	// edit 모드가 위반을 공백·탭만으로 결정적으로 고칠 수 있는 자리엔 수정 힌트를 함께 싣는다.
	//   none      — 자동교정 불가(개행·비공백 필요). edit 은 로그만 낸다.
	//   gap_left  — fix_col 바로 왼쪽의 공백·탭 런을 fix_val 개의 공백으로 맞춘다.
	//   gap_right — fix_col 바로 오른쪽의 공백·탭 런을 fix_val 개의 공백으로 맞춘다.
	//   indent    — 그 행의 선두 공백·탭 구역을 fix_val 개의 탭으로 맞춘다.
	enum class Fix_kind{ none, gap_left, gap_right, indent };

	// 충돌 사각 — 표기만으로 분류를 확정할 수 없는 자리는 위반으로 단정하지 않고 용의로 지목한다.
	//   violation — 확정 위반. 종료코드에 반영하고 edit 이 자동교정을 시도한다.
	//   suspect   — 용의. 사람의 판정 몫이라 종료코드에 넣지 않고 edit 도 손대지 않는다.
	enum class V_cat{ violation, suspect };

	struct Violation;
	struct Edit_note;
	struct Edit_result;

	// 행들을 규약 검사해 위반을 낸다(§1.1·§1.3는 raw 행, §8.2·§3 등은 @마스크 위에서).
	// final_newline — 파일이 개행으로 끝나는가(§9.4 파일 끝 개행).
	// 거짓이면 마지막 행에 위반을 낸다.
	auto check_lines(
		Lines const &lines, Seg_lines const &segs, bool final_newline
	)->std::vector<Violation>;

	// 검증용 렌더러 — 표기 판정 스트림을 토큰마다 한 행씩 덤프한다("행:열 분류 우선순위 원문").
	auto render_classes(Lines const &lines, Seg_lines const &segs)->Lines;

	// 공백·탭만 삽입/삭제해 위반을 결정적으로 고친다(개행·비공백 불가침). 고정점까지 반복하고,
	// [lo, hi] (0-기준 포함범위) 밖 행은 손대지 않는다. 회귀 게이트(공백·탭 제거 후 바이트 동일 +
	// 행 수 불변)를 실패하면 ok=false 로 원본을 그대로 돌려준다.
	auto edit_lines(Lines const &input, int lo, int hi, bool final_newline)->Edit_result;
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

struct sak::Violation{
	int row, col;   // 0-기준 행, 0-기준 바이트 열
	std::string rule, message;
	Fix_kind fix = Fix_kind::none;
	int fix_col = 0;   // gap_left/gap_right 의 경계 열 (indent 는 미사용, row 를 쓴다)
	int fix_val = 0;   // gap 은 목표 공백 수, indent 는 목표 탭 깊이
	V_cat cat = V_cat::violation;
};

// edit 모드 한 항목의 기록: 자동교정했는지(fixed) 범위 밖이라 남겼는지(!fixed).
struct sak::Edit_note{
	int row, col;
	std::string rule, message;
	bool fixed;
	V_cat cat = V_cat::violation;
};

// edit 결과: 수정된 행들 + 기록 + 회귀 게이트 통과 여부.
struct sak::Edit_result{
	Lines lines;
	std::vector<Edit_note> notes;
	bool ok;
};
