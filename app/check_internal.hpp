/*  SPDX-FileCopyrightText: (c) 2020 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#pragma once

#include <string>
#include <vector>

#include "check.hpp"
#include "lexer.hpp"

// check 구현부 내부 공유 계약 — check_*.cpp 번역단위들 사이에서만 쓰는 타입·헬퍼 선언.
// 공개 API 는 check.hpp 가 정의하고, 이 헤더는 그 구현을 의미 단위 파일들로 나누기 위한
// 안쪽 이음새다(app 밖에 노출하지 않는다).
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

namespace sak{
	struct Bk_pair;
	struct Angle_pair;
	struct Tok_8_3;
	struct Adj_tok;

	// 문맥 불변 토큰 분류 (§8.4 단계 2 — 분류가 모양만으로 결정되는 것만).
	// 문맥 의존 토큰(* & + - < > : && ! ~ 등)과 분류 모호(<< >> ::)는 모두 skip.
	enum class Tk_cls{
		skip,
		word,
		open_b, close_b,
		sep, // ; ,
		bin_ns, // . -> .* ->* (양쪽 무공백)
		bin_s, // = == != <= >= <=> || ? / % | ^ 와 모든 복합대입 (양쪽 공백)
		inc_dec, // ++ --
	};

	// 표기 판정 스트림의 §4 분류 (check_adjudicate.cpp 참조).
	enum class Adj_cls{
		word, // 식별자·키워드
		lit, // 문자열·문자 리터럴 — 피연산자
		open_b, close_b,
		semi, comma,
		bidir, unidir, operand_like, // §4 의 세 분류
		angle_open, angle_close, // 꺾쇠로 확정된 < >
		unresolved, // 아직 판정되지 않은 문맥 의존 글리프
	};

	// §6.1 개행 우선순위 — 위가 높다. 양방향 토큰과 다중행 괄호만 우선순위를 갖는다(§6.1 첫 항).
	enum class Prio{
		semi, comma,
		assign, ternary, lor, land, bor, bxor, band,
		eq, rel, spaceship, shift, add, mul,
		vop, // 가상연산자 ▽ 의 자리(§4.3·§6.1 ◆) — 실토큰이 갖는 값이 아니라 비교 기준선이다
		bracket,
		mem_ptr, mem, str_adj, scope, // str_adj = 인접 문자열 리터럴 사이의 자리(§9.1·§6.1 ◆)
		none,
	};
	//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

	// check_util.cpp — 마스크 탐색·단어 추출·행 성질 판정 등 공용 프리미티브.
	auto Display_width(std::string const &line)->std::size_t;
	auto is_word_char(char const c)->bool;
	auto Is_blank_row(std::string const &line)->bool;
	auto Is_code_char(char const ch)->bool;
	auto Has_code(std::string const &mask_row)->bool;
	auto Indent_unit_at(std::string const &line, int const p)->int;
	auto Indent_depth(std::string const &line)->int;
	auto Last_significant_col(std::string const &mask_row)->int;
	auto First_significant_col(std::string const &mask_row)->int;
	auto First_code_char(std::string const &mask_line)->char;
	auto Last_code_char(std::string const &mask_line)->char;
	auto Tail_spaces(std::string const &line)->int;
	auto Slice(std::string const &line, int const col, int const len)->std::string;
	auto Word_at(std::string const &line, int const col)->std::string;
	auto Word_starts_at(std::string const &line, int const i)->bool;
	auto Word_ending_at(std::string const &line, int const col)->std::string;
	auto Next_code(Lines const &mask, int const max_row, int &row, int &col)->bool;
	auto Match_paren(Lines const &mask, int const max_row, int &row, int &col)->bool;
	auto Prev_significant(Lines const &mask, int &row, int &col)->bool;
	auto Match_brace_back(Lines const &mask, int &row, int &col)->bool;

	auto Match_bracket_back(
		Lines const &mask, char const open, char const close, int &row, int &col
	)->bool;

	auto Word_before(Lines const &mask, int const row, int const col)->std::string;
	auto Is_do_tail(Lines const &mask, int const row, int const col)->bool;
	auto Starts_with_keyword(std::string const &mask_line, char const *kw)->bool;
	auto Starts_with_continuation_op(std::string const &mask_line)->bool;
	auto Continues_statement(std::string const &mask_line, bool const with_while)->bool;
	auto Has_nonsemi_code_after(std::string const &mask_line, int const pos)->bool;
	auto Stmt_ends_after(std::string const &mask_line, int const pos)->bool;
	auto Label_row(Lines const &mask, int const r, int &head_col)->bool;

	auto Next_code_row_over_blanks(
		Lines const &lines, Lines const &mask, int const from, bool &crossed_blank
	)->int;

	void Push_fix(
		std::vector<Violation> &out, Violation v,
		Fix_kind const kind, int const fix_col, int const fix_val
	);
	//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

	// check_line.cpp — 행 단위 문맥 불변 검사 (§1·§3·§4.3·§8) 와 §8.4 행 토크나이저.
	auto Tokenize_8_3(std::string const &mask)->std::vector<Tok_8_3>;
	void Check_width(std::string const &line, int const row, std::vector<Violation> &out);
	void Check_indent(std::string const &line, int const row, std::vector<Violation> &out);
	void Check_tab_use(std::string const &mask, int const row, std::vector<Violation> &out);
	void Check_space_run(std::string const &mask, int const row, std::vector<Violation> &out);
	void Check_inner_space(std::string const &mask, int const row, std::vector<Violation> &out);
	void Check_ctrl_brace(Lines const &mask, int const row, std::vector<Violation> &out);

	void Check_word_paren_space(
		std::string const &mask, int const row, std::vector<Violation> &out
	);

	void Check_continuation_cohesion(Lines const &mask, int const row, std::vector<Violation> &out);
	void Check_token_space(std::string const &mask, int const row, std::vector<Violation> &out);
	void Check_banned(std::string const &mask, int const row, std::vector<Violation> &out);

	void Check_keyword_position(
		std::string const &mask, int const row, std::vector<Violation> &out
	);

	void Check_unary_juxtaposition(
		std::string const &mask, int const row, std::vector<Violation> &out
	);
	//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

	// check_bracket.cpp — 실괄호 매처와 §5.4·§5.6, 그리고 §9.4 공행 검사.
	auto Match_brackets(Lines const &mask)->std::vector<Bk_pair>;

	void Check_multiline_bracket(
		Lines const &lines, Lines const &mask, Bk_pair const &p, std::vector<Violation> &out
	);

	void Check_virtual_brace(
		Lines const &lines, Lines const &mask,
		std::vector<Bk_pair> const &pairs, std::vector<Violation> &out
	);

	void Check_attribute_close(Lines const &mask, Bk_pair const &p, std::vector<Violation> &out);

	void Check_blank_line(
		Lines const &lines, Lines const &mask, int const row, std::vector<Violation> &out
	);

	void Check_close_open_blank_line(
		Lines const &lines, Lines const &mask,
		int const o_row, int const c_row, int const c_col, int const c_len, char const kind,
		std::vector<Violation> &out
	);

	void Check_bracket_blank_line(
		Lines const &lines, Lines const &mask, Bk_pair const &p, std::vector<Violation> &out
	);
	//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

	// check_cbracket.cpp — §5.5 낫괄호 앵커 검사들.
	auto Is_inline_type_close(Lines const &mask, Bk_pair const &p)->bool;

	void Check_anchor_keyword_semicolon(
		Lines const &lines, Lines const &mask, std::vector<Violation> &out
	);

	void Check_anchor_trailing_return(
		Lines const &lines, Lines const &mask, std::vector<Violation> &out
	);

	void Check_anchor_inline_type(
		Lines const &lines, Lines const &mask,
		std::vector<Bk_pair> const &pairs, std::vector<Violation> &out
	);

	void Check_anchor_case(Lines const &lines, Lines const &mask, std::vector<Violation> &out);

	void Check_anchor_colon_cbracket(
		Lines const &lines, Lines const &mask, std::vector<Violation> &out
	);

	void Check_anchor_var_decl_marker(
		Lines const &lines, Lines const &mask, std::vector<Violation> &out
	);

	void Check_cbracket_blank_line(
		Lines const &lines, Lines const &mask, Lines const &cut_mask,
		std::vector<Bk_pair> const &pairs, std::vector<Violation> &out
	);
	//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

	// check_angle.cpp — 꺾쇠 매처들과 §5.4·§5.7·§8.4·§8.5 꺾쇠 검사.
	void Dedup_angles(std::vector<Angle_pair> &v);
	auto Match_template_cast_angles(Lines const &mask)->std::vector<Angle_pair>;
	auto Match_closer_anchored_angles(Lines const &mask)->std::vector<Angle_pair>;
	auto Angle_chain_end(std::string const &m, int const from)->int;

	void Check_multiline_angle(
		Lines const &lines, Lines const &mask, Angle_pair const &p, std::vector<Violation> &out
	);

	void Check_angle_close_last(
		Lines const &mask, Angle_pair const &p, std::vector<Violation> &out
	);

	void Check_declarator_expansion(
		Lines const &mask, Angle_pair const &p, std::vector<Violation> &out
	);

	void Check_declarator_expansion_brace(
		Lines const &mask, std::vector<Bk_pair> const &pairs, std::vector<Violation> &out
	);

	void Check_angle_boundary(Lines const &mask, Angle_pair const &p, std::vector<Violation> &out);

	void Check_angle_inner_space(
		Lines const &mask, std::vector<Angle_pair> const &pairs,
		Angle_pair const &p, std::vector<Violation> &out
	);

	void Check_angle_blank_line(
		Lines const &lines, Lines const &mask, Angle_pair const &a, std::vector<Violation> &out
	);
	//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

	// check_adjudicate.cpp — 표기 판정 스트림 (파일 전역 토큰화·§4 분류 확정).
	auto Tokenize_file(Lines const &mask, Seg_lines const &segs)->std::vector<Adj_tok>;
	void Adjudicate_tokens(std::vector<Adj_tok> &toks);
	auto Adjudicated_angles(std::vector<Adj_tok> const &toks)->std::vector<Angle_pair>;
	//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

	// check_stream.cpp — 표기 판정에 기대는 2패스 검사 (§3·§5.5·§9.1·§9.2·§9.4·용의).
	void Check_adjudicated_space(std::vector<Adj_tok> const &toks, std::vector<Violation> &out);
	void Check_break_form(std::vector<Adj_tok> const &toks, std::vector<Violation> &out);

	void Check_break_competition(
		Lines const &mask, std::vector<Adj_tok> const &toks,
		std::vector<Bk_pair> const &pairs, std::vector<Angle_pair> const &angles,
		std::vector<Violation> &out
	);

	void Check_word_paren_vop(
		Lines const &lines, Lines const &mask, std::vector<Adj_tok> const &toks,
		std::vector<Bk_pair> const &pairs, std::vector<Angle_pair> const &angles,
		std::vector<Violation> &out
	);

	void Check_operator_blank_line(
		Lines const &lines, Lines const &mask, std::vector<Adj_tok> const &toks,
		std::vector<Bk_pair> const &pairs, std::vector<Violation> &out
	);

	void Check_unary_pair(std::vector<Adj_tok> const &toks, std::vector<Violation> &out);
	void Check_qualifier_prefix(std::vector<Adj_tok> const &toks, std::vector<Violation> &out);

	void Check_unmarked_wrap(
		Lines const &lines, Lines const &cut_lines, Lines const &cut_mask,
		std::vector<Violation> &out
	);

	void Check_glued_declarator(
		Lines const &cut_lines, std::vector<Adj_tok> const &toks,
		std::vector<Bk_pair> const &pairs, std::vector<Angle_pair> const &angles,
		std::vector<Violation> &out
	);

	void Check_string_splice_newline(
		Lines const &mask, Lines const &cut_lines, Seg_lines const &segs,
		std::vector<Adj_tok> const &toks, std::vector<Bk_pair> const &pairs,
		std::vector<Angle_pair> const &angles, std::vector<Violation> &out
	);

	void Check_brace_paren_newline(
		Lines const &lines, Lines const &mask, std::vector<Violation> &out
	);

	void Check_suspects(std::vector<Adj_tok> const &toks, std::vector<Violation> &out);
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

// 다중행 괄호 매처 (§5.4·§5.7).
// 짝지은 괄호 한 쌍의 여닫는 위치와 종류. kind = '(', '{', '[', 'A'([[ ]]).
struct sak::Bk_pair{
	int o_row, o_col, o_len;
	int c_row, c_col, c_len;
	char kind;
};

// §5·§8 꺾쇠괄호 매처·검사 — template<...> · *_cast<...> 자리 (문법 확정).
// well-formed C++ 은 template argument list 안의 나체 `<`/`>` 를 template 구분자로만
// 허용한다(연산자로 쓰려면 괄호 필수) → 렉서 수준에서 위양성 0 으로 짝을 잡을 수 있다.
struct sak::Angle_pair{
	int o_row, o_col;
	int c_row, c_col;
};

struct sak::Tok_8_3{
	int col, len;
	Tk_cls cls;
};

struct sak::Adj_tok{
	int row, col, len;
	std::string text; // @마스크에서 뜬 원문 (리터럴 자리는 '@' 로 덮여 있어 비워 둔다)
	Tk_cls lex;
	Adj_cls cls;
	Prio prio;
	bool suspect; // 충돌 사각 위에 선 판정
	int gl, gr; // 인접 공백 폭(행을 넘으면 개행이 공백을 대신하므로 1). 이웃이 없으면 -1
	int el, er; // §5.3 투명성을 적용한 유효 이웃의 인덱스(감싸는 괄호는 이웃이 아니다). 없으면 -1
};
