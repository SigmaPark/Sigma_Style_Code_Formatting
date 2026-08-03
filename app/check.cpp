/*  SPDX-FileCopyrightText: (c) 2020 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "check_internal.hpp"

#include <string>
#include <vector>

namespace sak{
	static auto Cut_tail_comment(Lines const &src, Seg_lines const &segs)->Lines;
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

// §8.3 잉여공백은 "§2 제외 대상인 주석을 걷어낸 뒤의 행"에서 잰다(v2.2.0). 그래야 행끝 주석이
// 붙은 행에서도 잉여공백과 §5.5 2칸 마커를 볼 수 있다. 행 끝까지 이어지는 주석 세그먼트를 잘라
// 낸 사본을 만든다 — 문자열 리터럴 꼬리는 코드이므로 자르지 않는다.
auto sak::Cut_tail_comment(Lines const &src, Seg_lines const &segs)->Lines{
	Lines out = src;

	int const  
		rows = static_cast<int>(out.size()),
		seg_rows = static_cast<int>(segs.size())
	;

	for(int r = 0; r < rows && r < seg_rows; ++r){
		if(segs[r].empty()){
			continue;
		}

		Segment const &last = segs[r].back();
		int const len = static_cast<int>(out[r].size());

		if(last.kind == Seg_kind::comment && last.col <= len){
			out[r] = out[r].substr( 0, static_cast<std::size_t>(last.col) );
		}
	}

	return out;
}

auto sak::check_lines(
	Lines const &lines, Seg_lines const &segs, bool const final_newline
)->std::vector<Violation>{
	std::vector<Violation> out;
	Lines const mask = render_mask(lines, segs);
	Lines const cut_lines = Cut_tail_comment(lines, segs);
	Lines const cut_mask = Cut_tail_comment(mask, segs);
	int const rows = static_cast<int>(lines.size());

	std::vector<Bk_pair> const pairs = Match_brackets(mask);

	for(Bk_pair const &p : pairs){
		Check_multiline_bracket(lines, mask, p, out);
		Check_attribute_close(mask, p, out);
		Check_bracket_blank_line(lines, mask, p, out);
	}

	Check_virtual_brace(lines, mask, pairs, out);
	Check_anchor_keyword_semicolon(lines, mask, out);
	Check_anchor_trailing_return(lines, mask, out);
	Check_anchor_inline_type(cut_lines, cut_mask, pairs, out);
	Check_declarator_expansion_brace(mask, pairs, out);
	Check_anchor_case(lines, mask, out);
	Check_anchor_colon_cbracket(lines, mask, out);
	Check_anchor_var_decl_marker(cut_lines, cut_mask, out);
	Check_cbracket_blank_line(lines, mask, cut_mask, pairs, out);

	// 표기 판정 — 꺾쇠 검사보다 먼저 돌린다. 꺾쇠의 정체는 이제 표기가 판정하기 때문이다.
	std::vector<Adj_tok> toks = Tokenize_file(mask, segs);
	Adjudicate_tokens(toks);

	// 꺾쇠 짝은 세 출처를 합친다 — 레거시 키워드 앵커·닫힘 신호 앵커에, 표기 판정이 확정한
	// 짝(나체 꺾쇠·괄호 속 중첩 꺾쇠까지)을 얹는다.
	std::vector<Angle_pair> angles = Match_template_cast_angles(mask);
	std::vector<Angle_pair> const closer_angles = Match_closer_anchored_angles(mask);
	std::vector<Angle_pair> const adj_angles = Adjudicated_angles(toks);
	angles.insert(angles.end(), closer_angles.begin(), closer_angles.end());
	angles.insert(angles.end(), adj_angles.begin(), adj_angles.end());
	Dedup_angles(angles);

	for(Angle_pair const &a : angles){
		Check_multiline_angle(lines, mask, a, out);
		Check_angle_close_last(mask, a, out);
		Check_declarator_expansion(mask, a, out);
		Check_angle_boundary(mask, a, out);
		Check_angle_inner_space(mask, angles, a, out);
		Check_angle_blank_line(lines, mask, a, out);
	}

	for(int row = 0; row < rows; ++row){
		Check_width(lines[row], row, out);
		Check_indent(lines[row], row, out);
		Check_tab_use(mask[row], row, out);
		Check_space_run(mask[row], row, out);
		Check_inner_space(mask[row], row, out);
		Check_ctrl_brace(mask, row, out);
		Check_word_paren_space(mask[row], row, out);
		Check_continuation_cohesion(mask, row, out);
		Check_blank_line(lines, mask, row, out);
		Check_token_space(mask[row], row, out);
		Check_keyword_position(mask[row], row, out);
		Check_unary_juxtaposition(mask[row], row, out);
		Check_banned(mask[row], row, out);
	}
	//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

	// 2패스 — 표기 판정에 기대는 검사. 1패스(문맥 불변)에서 이미 위반이 난 행은 배치를 믿을 수
	// 없으므로 그 행에서는 침묵한다(연쇄 오염 차단).
	std::vector<Violation> pass2;
	Check_adjudicated_space(toks, pass2);
	Check_break_form(toks, pass2);
	Check_break_competition(mask, toks, pairs, angles, pass2);
	Check_word_paren_vop(lines, mask, toks, pairs, angles, pass2);
	Check_operator_blank_line(lines, mask, toks, pairs, pass2);
	Check_unary_pair(toks, pass2);
	Check_qualifier_prefix(toks, pass2);
	Check_unmarked_wrap(lines, cut_lines, cut_mask, pass2);
	Check_glued_declarator(cut_lines, toks, pairs, angles, pass2);
	Check_string_splice_newline(mask, cut_lines, segs, toks, pairs, angles, pass2);
	Check_brace_paren_newline(lines, mask, pass2);
	Check_suspects(toks, pass2);

	std::vector<char> gated(rows, 0);

	for(Violation const &v : out){
		if(v.row >= 0 && v.row < rows){
			gated[v.row] = 1;
		}
	}

	for(Violation const &v : pass2){
		if(v.row >= 0 && v.row < rows && gated[v.row] == 0){
			out.push_back(v);
		}
	}

	// §9.4 — 파일은 개행으로 끝나야 한다(마지막 행은 공행). 개행이 아니라 비어 있지 않은 코드로
	// 끝나면 위반. 개행을 만들 수 없으므로 edit 은 손대지 못한다([manual]).
	if(!final_newline && rows > 0){
		int const last = rows - 1;

		out.push_back(
			{ last, static_cast<int>(lines[last].size()), "9.4", "missing newline at end of file" }
		);
	}

	return out;
}
