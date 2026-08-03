/*  SPDX-FileCopyrightText: (c) 2020 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "check_internal.hpp"

#include <string>
#include <vector>

namespace sak{
	static auto Is_empty_pair(std::string const &line, int const from, int const to)->bool;
	static auto Is_virtual_gap_row(Lines const &lines, Lines const &mask, int const r)->bool;
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

// 단일행에서 여닫는 괄호 사이 [from, to) 가 공백·탭·@뿐인지 — §5.1 내용 없는 괄호쌍 판정.
auto sak::Is_empty_pair(std::string const &line, int const from, int const to)->bool{
	for(int cc = from; cc < to; ++cc){
		char const ic = line[cc];

		if(ic != ' ' && ic != '\t' && ic != '@'){
			return false;
		}
	}

	return true;
}

// @마스크 전체를 스캔해 ()/{}/[]/[[ ]] 짝을 모은다. 짝이 깨지면 그 자리는 버린다.
// 단일행 빈 괄호(())/{}/[]/[[]]는 §5.1에 따라 괄호 표현이 아니므로 제외한다.
// (다중행 빈 괄호는 거의 없으니 보수적으로 통과시킨다.)
auto sak::Match_brackets(Lines const &mask)->std::vector<Bk_pair>{
	std::vector<Bk_pair> out;

	struct Frame{
		int r, c, len;
		char kind;
	};

	std::vector<Frame> st;
	int const rows = static_cast<int>(mask.size());

	for(int r = 0; r < rows; ++r){
		std::string const &line = mask[r];
		int const n = static_cast<int>(line.size());
		int c = 0;

		while(c < n){
			char const ch = line[c];

			if(ch == '[' && c + 1 < n && line[c + 1] == '['){
				st.push_back({ r, c, 2, 'A' });
				c += 2;

				continue;
			}

			if(
				ch == ']' && c + 1 < n && line[c + 1] == ']'
				&& !st.empty() && st.back().kind == 'A'
			){
				Frame const f = st.back();
				st.pop_back();

				if( f.r != r || !Is_empty_pair(line, f.c + f.len, c) ){
					out.push_back({ f.r, f.c, f.len, r, c, 2, 'A' });
				}

				c += 2;

				continue;
			}

			if(ch == '(' || ch == '{' || ch == '['){
				st.push_back({ r, c, 1, ch });
				++c;

				continue;
			}

			if(ch == ')' || ch == '}' || ch == ']'){
				char const want = ch == ')' ? '(' : ch == '}' ? '{' : '[';

				if(!st.empty() && st.back().kind == want){
					Frame const f = st.back();
					st.pop_back();

					if( f.r != r || !Is_empty_pair(line, f.c + f.len, c) ){
						out.push_back({ f.r, f.c, f.len, r, c, 1, want });
					}
				}

				++c;

				continue;
			}

			++c;
		}
	}

	return out;
}

// §9.4 공행의 형상 유효성. (§9.2 종속의 강제 공행은 범주 4 → 검사 외.)
// 공행은 개행의 발생원이 아니라 인가된 자리에 쌓인 형상이다(§9.4) — 자리의 적법성은 각 개행
// 검사(Check_word_paren_newline·Check_unmarked_wrap·Check_continuation_head)가 공행을
// 횡단하며 본다. 여기서는 형상만 본다: 공행은 파일 경계가 아니어야 하고, 위·아래 인접 행도
// 공행이 아니어야 한다.
// 들여쓰기 비교는 인접 행이 "코드 토큰을 포함하는 행"일 때만 의미가 있다.
// 인접 행이 전처리 본문(§2 제외 — `\` 연장 행 포함)·주석 only·문자열 only 면
// 그 행의 raw 들여쓰기는 spec 상 들여쓰기가 아니므로 비교 대상에서 뺀다.
// 행 R 이 닫는 가상 중괄호 `⦄▽` 의 자리(물리적 빈 행)인가 — 공행이고, 아래로 공행을 건너 만나는
// 첫 비공행이 라벨 행이면 그렇다. 이 자리의 공행은 §5.6 리듬이 관할하므로 §9.4 형상 검사에서 뺀다.
auto sak::Is_virtual_gap_row(Lines const &lines, Lines const &mask, int const r)->bool{
	if( !Is_blank_row(lines[r]) ){
		return false;
	}

	int const rows = static_cast<int>(lines.size());
	int d = r + 1;

	while( d < rows && Is_blank_row(lines[d]) ){
		++d;
	}

	int hc = 0;

	return d < rows && Label_row(mask, d, hc);
}

void sak::Check_blank_line(
	Lines const &lines, Lines const &mask, int const row, std::vector<Violation> &out
){
	if( !Is_blank_row(lines[row]) ){
		return;
	}

	if( Is_virtual_gap_row(lines, mask, row) ){
		return; // §5.6 리듬 관할 — ⦄▽ 자리의 공행
	}

	int const last = static_cast<int>(lines.size()) - 1;

	// 파일 경계 — 첫 행 공행, 그리고 EOF 에 인접한 꼬리 공행(파일 끝 개행 하나 너머의 여분)은 위반.
	// 그 밖의 자리에서 공행이 연속하는 것은 §9.4 신판에서 허용한다(4연속·연속 금지 조항 폐지).
	if(row == 0 || row == last){
		out.push_back({ row, 0, "9.4", "blank line at file boundary" });

		return;
	}

	if( !Has_code(mask[row - 1]) || !Has_code(mask[row + 1]) ){
		return;
	}

	if( Indent_depth(lines[row - 1]) != Indent_depth(lines[row + 1]) ){
		out.push_back({ row, 0, "9.4", "neighbors differ in indentation" });
	}
}

// §9.4 확장 — 다중행 괄호(세미콜론 제외)가 형성한 개행 경쟁 범위(여는 행·닫는 행)의
// 바로 위·아래 인접행은 공행이어야 한다. 단 그 자리에 공행을 두어도 §9.4 유효성
// (인접 두 코드행의 들여쓰기 일치)이 성립할 때만 요구한다.
// 판정 기준 — "이 자리가 실제 문장 경계인가":
//   위쪽 — 인접 위 행의 마지막 코드 문자가 `;` 또는 `}` 여야 하며(문장 종결),
//   여는 행이 이항 연산자로 시작하거나 `else`/`catch` 로 시작하면 이전 문장의 연장이라 유보.
//   아래쪽(중괄호) — 닫는 행에 `;` 아닌 코드가 남거나(표현식 종속),
//   인접 아래 행이 이항 연산자·`else`/`catch`/`while` 로 시작하면 유보.
//   아래쪽(그 외 괄호·꺾쇠) — 셋으로 가른다:
//     · 닫는 행이 `;` 로 끝나면(사이에 다른 닫는 괄호 없음) 문장 종결 — 공행 필수.
//     · 닫는 괄호가 행의 마지막이면 문장이 '▽' 로 다음 행에 이어진다(§4.3) — 공행 필수가
//       아니라 오히려 금지다(§9.4). 다음 코드 행이 단어 머리일 때만 본다(연산자 머리는 그
//       토큰의 경쟁 범위 검사가 맡는다).
//     · 그 밖의 꼬리는 문장이 다른 자리에서 끝난다 — 관할 밖.
void sak::Check_close_open_blank_line(
	Lines const &lines, Lines const &mask,
	int const o_row, int const c_row, int const c_col, int const c_len, char const kind,
	std::vector<Violation> &out
){
	if(o_row == c_row){
		return;
	}

	int const rows = static_cast<int>(lines.size());
	int const o_ind = Indent_depth(lines[o_row]);
	int const c_ind = Indent_depth(lines[c_row]);

	if(o_row > 0){
		int const nr = o_row - 1;
		std::string const &above = mask[nr];

		if( !Is_blank_row(lines[nr]) && Has_code(above) && Indent_depth(lines[nr]) == o_ind ){
			char const above_last = Last_code_char(above);

			bool const  
				at_stmt_boundary
				= (above_last == ';' || above_last == '}')
				&& !Continues_statement(mask[o_row], false)
			;

			if(at_stmt_boundary){
				out.push_back({ o_row, 0, "9.4", "missing blank line above multi-line bracket" });
			}
		}
	}

	if(c_row + 1 >= rows){
		return;
	}

	int const nr = c_row + 1;
	std::string const &below = mask[nr];

	if(kind == '{'){
		if( !Is_blank_row(lines[nr]) && Has_code(below) && Indent_depth(lines[nr]) == c_ind ){
			bool const  
				at_stmt_boundary
				= !Has_nonsemi_code_after(mask[c_row], c_col + c_len)
				&& !Continues_statement(below, true)
			;

			if(at_stmt_boundary){
				out.push_back({ c_row, 0, "9.4", "missing blank line below multi-line bracket" });
			}
		}

		return;
	}

	if( Stmt_ends_after(mask[c_row], c_col + c_len) ){
		if( !Is_blank_row(lines[nr]) && Has_code(below) && Indent_depth(lines[nr]) == c_ind ){
			if( !Continues_statement(below, true) ){
				out.push_back({ c_row, 0, "9.4", "missing blank line below multi-line bracket" });
			}
		}

		return;
	}

	if( Last_significant_col(mask[c_row]) == c_col + c_len - 1 && Is_blank_row(lines[nr]) ){
		bool crossed = false;
		int const cont = Next_code_row_over_blanks(lines, mask, c_row, crossed);

		if(cont >= 0 && crossed){
			int const cc = First_significant_col(mask[cont]);

			if( cc >= 0 && is_word_char(mask[cont][cc]) ){
				for(int b = nr; b < cont; ++b){
					if( Is_blank_row(lines[b]) ){
						out.push_back(
							{ b, 0, "9.4", "blank line inside a break-competition range" }
						);
					}
				}
			}
		}
	}
}

void sak::Check_bracket_blank_line(
	Lines const &lines, Lines const &mask, Bk_pair const &p, std::vector<Violation> &out
){
	Check_close_open_blank_line(lines, mask, p.o_row, p.c_row, p.c_col, p.c_len, p.kind, out);
}

// §5.4 다중행 괄호의 위치·들여쓰기 검사.
// (1) 여닫는 행 들여쓰기 동일, (2) 여는 괄호 다음 같은 행에 코드 토큰 없음(행 끝),
// (3) 닫는 괄호 직전 같은 행에 코드 토큰 없음(행 처음), (4) 중간 코드 행의 들여쓰기 ≥ 외곽+1.
void sak::Check_multiline_bracket(
	Lines const &lines, Lines const &mask, Bk_pair const &p,
	std::vector<Violation> &out
){
	if(p.o_row == p.c_row){
		return;
	}

	// (1) 여닫는 행 들여쓰기 동일
	int const o_ind = Indent_depth(lines[p.o_row]), c_ind = Indent_depth(lines[p.c_row]);

	if(o_ind != c_ind){
		Push_fix(
			out, { p.c_row, 0, "5.4", "open/close indent differ" },
			Fix_kind::indent, 0, o_ind
		);
	}

	// (2) 여는 괄호가 행 마지막 코드 토큰인지
	std::string const &o_line = mask[p.o_row];
	int const o_n = static_cast<int>(o_line.size()), o_end = p.o_col + p.o_len;

	for(int cc = o_end; cc < o_n; ++cc){
		if( Is_code_char(o_line[cc]) ){
			out.push_back({ p.o_row, cc, "5.4", "opening bracket not last token" });

			break;
		}
	}

	// (3) 닫는 괄호가 행 첫 코드 토큰인지
	std::string const &c_line = mask[p.c_row];

	for(int cc = 0; cc < p.c_col; ++cc){
		if( Is_code_char(c_line[cc]) ){
			out.push_back({ p.c_row, cc, "5.4", "closing bracket not first token" });

			break;
		}
	}

	// (4) 중간 코드 행 들여쓰기 ≥ 외곽+1.
	// 단 §5.6 가상 중괄호로 닫혔다 열리는 자리 — case/default/public/private/protected —
	// 는 외곽과 같은 들여쓰기가 정상이므로 검사 제외한다.
	for(int r = p.o_row + 1; r < p.c_row; ++r){
		if( lines[r].empty() || !Has_code(mask[r]) ){
			continue;
		}

		int const ind = Indent_depth(lines[r]);

		std::string const &m = mask[r];
		int first = 0;
		int const m_n = static_cast<int>(m.size());

		while( first < m_n && (m[first] == ' ' || m[first] == '\t' || m[first] == '@') ){
			++first;
		}

		std::string const head = first < m_n ? Word_at(m, first) : "";

		bool const  
			virtual_close
			= head == "case" || head == "default"
			|| head == "public" || head == "private" || head == "protected"
		;

		// §5.6 가상 중괄호 자리(case/default/접근지정자)는 +1 룰을 적용하지 않는다.
		// 들여쓰기는 가장 안쪽 brace 와 비교해야 정확하므로 Check_virtual_brace 가 다룬다.
		if(virtual_close){
			continue;
		}

		// 첫 코드 토큰이 닫는 괄호이면 내부 짝의 close 라인이라 외곽 middle 검사에서 빠진다
		// (자기 짝의 §5.4 검사에서 다룬다).
		char const first_c = first < m_n ? m[first] : '\0';

		if(first_c == ')' || first_c == ']' || first_c == '}'){
			continue;
		}

		if(ind < o_ind + 1){
			Push_fix(
				out, { r, 0, "5.4", "middle line indent insufficient" },
				Fix_kind::indent, 0, o_ind + 1
			);
		}
	}
}

// §5.6 가상 중괄호 — case/default/접근지정자(public/private/protected) 라인은
// 그 라인을 *직접 둘러싼 가장 안쪽 `{ }` brace* 와 같은 들여쓰기여야 한다.
// 외곽 함수 본체나 namespace 본체와의 들여쓰기는 비교 대상이 아니다.
void sak::Check_virtual_brace(
	Lines const &lines, Lines const &mask,
	std::vector<Bk_pair> const &pairs, std::vector<Violation> &out
){
	int const rows = static_cast<int>(lines.size());

	for(int r = 0; r < rows; ++r){
		if( int head_col = 0; !Label_row(mask, r, head_col) ){
			continue;
		}

		Bk_pair const *inner = nullptr;

		for(Bk_pair const &p : pairs){
			if(p.kind != '{' || p.o_row >= r || p.c_row <= r){
				continue;
			}

			if(!inner || p.o_row > inner->o_row){
				inner = &p;
			}
		}

		if(!inner){
			continue;
		}

		int const r_ind = Indent_depth(lines[r]), o_ind = Indent_depth(lines[inner->o_row]);

		if(r_ind != o_ind){
			Push_fix(
				out, { r, 0, "5.6", "virtual close: indent must equal enclosing brace" },
				Fix_kind::indent, 0, o_ind
			);
		}

		// §5.6 리듬 — 닫는 가상 중괄호 `⦄▽` 는 물리적으로 빈 행이다. 라벨 위의 공행 수를 세어,
		// 위 첫 비공행이 여는 `{`(빈 첫 구간) 또는 다른 라벨(빈 낙하 구간)이면 붕괴로 공행 0,
		// 내용 행이면 공행 정확히 1 을 요구한다. 첫 비공행이 주석·전처리면 보수적으로 침묵한다.
		// 개행 삽입·삭제는 edit 이 못 하므로 힌트 없이 [manual] 로 남긴다.
		int p = r - 1;
		int blanks = 0;

		while( p >= 0 && Is_blank_row(lines[p]) ){
			++blanks;
			--p;
		}

		if( p >= inner->o_row && Has_code(mask[p]) ){
			int hc = 0;
			bool const empty_section = p == inner->o_row || Label_row(mask, p, hc);
			int const want = empty_section ? 0 : 1;

			if(blanks != want){
				out.push_back(
					{
						r, r_ind, "5.6",
						empty_section
						? "virtual-brace rhythm: no blank line before this label (empty section)"
						: "virtual-brace rhythm: exactly one blank line required before this label"
					}
				);
			}
		}
	}
}

// §4.3 다중행 [[ ]] 의 닫는 ']]' 뒤에 피연산 토큰(단어)이 이어지면 `▽` 가 개행하므로 ']]' 가
// 행 마지막 토큰이어야 한다. 부착·종결 토큰은 `▽` 자리가 아니라 침묵(단어만 위반).
void sak::Check_attribute_close(
	Lines const &mask, Bk_pair const &p, std::vector<Violation> &out
){
	if(p.kind != 'A' || p.o_row == p.c_row){
		return;
	}

	std::string const &c_line = mask[p.c_row];
	int const c_n = static_cast<int>(c_line.size()), c_end = p.c_col + p.c_len;

	for(int cc = c_end; cc < c_n; ++cc){
		if( Is_code_char(c_line[cc]) ){
			if( is_word_char(c_line[cc]) ){
				out.push_back(
					{ p.c_row, cc, "4.3", "']]' before an operand: it must end the line" }
				);
			}

			break;
		}
	}
}
