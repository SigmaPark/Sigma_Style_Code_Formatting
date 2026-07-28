/*  SPDX-FileCopyrightText: (c) 2020 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "check_internal.hpp"

#include <algorithm>
#include <string>
#include <vector>

// 한 행에서 공백·탭을 모두 뺀 문자열 — 회귀 게이트용(비공백·줄 구조 불변 증명).
static auto Strip_ws(std::string const &line)->std::string{
	std::string out;

	for(char const ch : line){
		if(ch != ' ' && ch != '\t'){
			out += ch;
		}
	}

	return out;
}

// edit 한 항목: 어느 행·열의 위반을 어떤 수정으로 처리했는지.
struct Edit_op{
	int row, col, val;
	Fix_kind kind;
};

// 그 경계의 공백이 표기 판정에 참여하는 신호인가 — 문맥 의존 글리프의 좌우 경계에 붙은 공백 런.
static auto Is_signal_gap(
	std::vector<Adj_tok> const &toks, int const row, int const col, Fix_kind const kind
)->bool{
	for(Adj_tok const &t : toks){
		if(t.row != row || t.lex != Tk_cls::skip || t.cls == Adj_cls::lit){
			continue;
		}

		if(kind == Fix_kind::gap_left && col == t.col){
			return true;
		}

		if(kind == Fix_kind::gap_right && col == t.col + t.len){
			return true;
		}
	}

	return false;
}

// 그 경계에 지금 놓인 공백·탭의 폭.
static auto Gap_width(std::string const &line, int const col, Fix_kind const kind)->int{
	int const n = static_cast<int>(line.size());
	int g = 0;

	if(kind == Fix_kind::gap_left){
		int s = col - 1;

		while( s >= 0 && (line[s] == ' ' || line[s] == '\t') ){
			--s;
			++g;
		}

		return g;
	}

	int e = col;

	while( e < n && (line[e] == ' ' || line[e] == '\t') ){
		++e;
		++g;
	}

	return g;
}

// 한 행에 수정을 적용한다. gap 은 fix_col 경계의 공백·탭 런을 val 개의 공백으로,
// indent 는 선두 공백·탭 구역을 val 개의 탭으로 맞춘다(모두 공백·탭만 건드린다).
static void Apply_edit_op(std::string &line, Edit_op const &op){
	int const n = static_cast<int>(line.size());

	if(op.kind == Fix_kind::indent){
		int q = 0;

		while( q < n && (line[q] == ' ' || line[q] == '\t') ){
			++q;
		}

		line = std::string(op.val, '\t') + line.substr(q);

		return;
	}

	if(op.kind == Fix_kind::gap_left){
		int s = op.col;

		while( s > 0 && (line[s - 1] == ' ' || line[s - 1] == '\t') ){
			--s;
		}

		line = line.substr(0, s) + std::string(op.val, ' ') + line.substr(op.col);

		return;
	}

	if(op.kind == Fix_kind::gap_right){
		int e = op.col;

		while( e < n && (line[e] == ' ' || line[e] == '\t') ){
			++e;
		}

		line = line.substr(0, op.col) + std::string(op.val, ' ') + line.substr(e);
	}
}

auto edit_lines(
	Lines const &input, int const lo, int const hi, bool const final_newline
)->Edit_result{
	Lines lines = input;
	std::vector<Edit_note> fixed_notes;

	// 고정점까지 반복 — 매 패스 재검사해 자동교정 힌트(fix != none)를 모아 적용한다.
	for(int pass = 0; pass < 8; ++pass){
		Seg_lines const segs = ::scan_lines(lines);
		std::vector<Violation> const viol = ::check_lines(lines, segs, final_newline);

		// 신호 공백 불가침 — 판정에 참여하는 공백의 유무를 자동교정이 뒤집어선 안 된다.
		Lines const mask = ::render_mask(lines, segs);
		std::vector<Adj_tok> toks = ::Tokenize_file(mask, segs);
		::Adjudicate_tokens(toks);

		std::vector<Edit_op> ops;

		for(Violation const &v : viol){
			// 용의는 확정 판정이 아니므로 자동교정 대상이 아니다 — 손대지 않는다.
			if(
				v.cat != V_cat::violation || v.fix == Fix_kind::none
				|| v.row < lo || v.row > hi
			){
				continue;
			}

			bool const  
				gap
				= v.fix == Fix_kind::gap_left || v.fix == Fix_kind::gap_right
			;

			if( gap && ::Is_signal_gap(toks, v.row, v.fix_col, v.fix) ){
				int const cur = ::Gap_width(lines[v.row], v.fix_col, v.fix);

				// 폭만 다듬는 교정은 허용하되, 신호(있음/없음)를 뒤집는 교정은 손대지 않는다.
				if( (cur == 0) != (v.fix_val == 0) ){
					continue;
				}
			}

			ops.push_back({ v.row, v.fix_col, v.fix_val, v.fix });
			fixed_notes.push_back({ v.row, v.col, v.rule, v.message, true, v.cat });
		}

		if(ops.empty()){
			break;
		}

		// 한 행 안에서는 오른쪽(높은 열)부터 적용해 앞선 수정이 뒤 열을 밀지 않게 한다.
		// indent 는 fix_col 이 0 이라 자연히 마지막에 적용된다.
		std::sort(
			ops.begin(), ops.end(),
			[](Edit_op const &a, Edit_op const &b)->bool{
				return a.row != b.row ? a.row < b.row : a.col > b.col;
			}
		);

		for(Edit_op const &op : ops){
			::Apply_edit_op(lines[op.row], op);
		}
	}

	// 회귀 게이트 — 공백·탭 제거 후 바이트 동일 + 행 수 불변이어야 자동교정이 정당하다.
	bool ok = lines.size() == input.size();

	for(std::size_t i = 0; ok && i < lines.size(); ++i){
		if( ::Strip_ws(lines[i]) != ::Strip_ws(input[i]) ){
			ok = false;
		}
	}

	Edit_result res;
	res.ok = ok;
	res.lines = ok ? lines : input;

	// 자동교정 기록(중복 제거) — 게이트를 통과했을 때만 유효하다.
	if(ok){
		for(Edit_note const &fn : fixed_notes){
			bool dup = false;

			for(Edit_note const &seen : res.notes){
				if(
					seen.row == fn.row && seen.col == fn.col
					&& seen.rule == fn.rule && seen.message == fn.message
				){
					dup = true;

					break;
				}
			}

			if(!dup){
				res.notes.push_back(fn);
			}
		}
	}

	// 자동교정 밖에 남은 위반(범위 안)을 manual 로 기록 — 사람·AI 인계 목록.
	Seg_lines const segs = ::scan_lines(res.lines);
	std::vector<Violation> const remain = ::check_lines(res.lines, segs, final_newline);

	for(Violation const &v : remain){
		if(v.row < lo || v.row > hi){
			continue;
		}

		res.notes.push_back({ v.row, v.col, v.rule, v.message, false, v.cat });
	}

	std::sort(
		res.notes.begin(), res.notes.end(),
		[](Edit_note const &a, Edit_note const &b)->bool{
			return a.row != b.row ? a.row < b.row : a.col < b.col;
		}
	);

	return res;
}
