/*  SPDX-FileCopyrightText: (c) 2020 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include "check_internal.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <vector>

namespace sak{
	static auto Prio_of_bidir(std::string const &text)->Prio;
	static auto Prio_of(std::string const &text, Adj_cls const cls)->Prio;
	static auto Lex_to_adj(Tk_cls const lex, std::string const &text)->Adj_cls;
	static auto Is_preproc_row(Seg_lines const &segs, int const row)->bool;
	static auto Is_literal_seg(Seg_kind const kind)->bool;
	static auto Is_keyword(std::string const &w)->bool;
	static auto Is_unary_prefix(std::string const &t)->bool;

	static void Mark_suspects(
		std::vector<Adj_tok> &toks, std::vector<int> const &el, std::vector<int> const &er,
		std::vector<int> const &dep
	);

	static auto Adj_name(Adj_cls const cls)->char const *;
	static auto Prio_name(Prio const prio)->char const *;
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

// §5.7 템플릿 헤더의 닫는 꺾쇠 뒤에는 단일행·다중행을 불문하고 반드시 개행을 둔다.
// 여는 `<` 바로 앞이 `template` 단어인 짝만 헤더로 보고, 닫는 `>` 가 그 행의 마지막 의미
// 토큰인지 검사한다(행끝 주석은 §2 제외 대상이라 마스크에서 `@` — 마지막 토큰 판정에 무해).
// 표기 판정 — 파일 전역 토큰 스트림.
//
// Tokenize_8_3(§8.4) 은 한 행만 보고, 문맥 의존 글리프(* & + - < > : && << >> :: ! ~ ...)를
// skip 으로 남긴다. 그 위에 파일 전역 스트림을 얹어 두 가지를 더 본다.
//   (1) 문자열·문자 리터럴을 피연산 토큰(lit)으로 실체화한다 — @마스크만 보면 "s" + x 의 "+" 가
//       왼쪽 피연산자를 잃어 부호로 오판된다.
//   (2) 행을 넘는 이웃을 본다 — §9.1·§9.2 는 두 행 창에서 판정된다.
// 이 구역은 스트림만 만든다. 실제 판정(표기 판정 술어)은 Adjudicate_tokens 의 몫이다.

// 양방향으로 판정된 기호형 토큰의 §6.1 우선순위.
auto sak::Prio_of_bidir(std::string const &text)->Prio{
	struct Entry{
		char const *text;
		Prio prio;
	};

	static Entry const  
		Table[]
		= {
			{ "=", Prio::assign }, { "+=", Prio::assign }, { "-=", Prio::assign },
			{ "*=", Prio::assign }, { "/=", Prio::assign }, { "%=", Prio::assign },
			{ "<<=", Prio::assign }, { ">>=", Prio::assign }, { "&=", Prio::assign },
			{ "^=", Prio::assign }, { "|=", Prio::assign },
			{ "?", Prio::ternary }, { ":", Prio::ternary },
			{ "||", Prio::lor }, { "&&", Prio::land },
			{ "|", Prio::bor }, { "^", Prio::bxor }, { "&", Prio::band },
			{ "==", Prio::eq }, { "!=", Prio::eq },
			{ "<", Prio::rel }, { "<=", Prio::rel }, { ">", Prio::rel }, { ">=", Prio::rel },
			{ "<=>", Prio::spaceship },
			{ "<<", Prio::shift }, { ">>", Prio::shift },
			{ "+", Prio::add }, { "-", Prio::add },
			{ "*", Prio::mul }, { "/", Prio::mul }, { "%", Prio::mul },
			{ ".*", Prio::mem_ptr }, { "->*", Prio::mem_ptr },
			{ "->", Prio::mem }, { ".", Prio::mem },
			{ "::", Prio::scope }
		}
	;

	for(Entry const &e : Table){
		if(text == e.text){
			return e.prio;
		}
	}

	return Prio::none;
}

// 토큰의 §6.1 우선순위. 단방향·피연산·비기호형은 개행 대상이 아니라 우선순위가 없다.
auto sak::Prio_of(std::string const &text, Adj_cls const cls)->Prio{
	if(cls == Adj_cls::semi){
		return Prio::semi;
	}

	if(cls == Adj_cls::comma){
		return Prio::comma;
	}

	if(cls == Adj_cls::open_b || cls == Adj_cls::close_b){
		return Prio::bracket;
	}

	if(cls == Adj_cls::bidir){
		return Prio_of_bidir(text);
	}

	return Prio::none;
}

// Tok_8_3 의 모양 분류를 §4 분류로 옮긴다. 모양만으로 갈리지 않는 글리프(skip)는 unresolved 로
// 남겨 Adjudicate_tokens 에 넘긴다.
auto sak::Lex_to_adj(Tk_cls const lex, std::string const &text)->Adj_cls{
	switch(lex){
	case Tk_cls::word:
		return Adj_cls::word;

	case Tk_cls::open_b:
		return Adj_cls::open_b;

	case Tk_cls::close_b:
		return Adj_cls::close_b;

	case Tk_cls::sep:
		return text == ";" ? Adj_cls::semi : Adj_cls::comma;

	case Tk_cls::bin_ns:
	case Tk_cls::bin_s:
		return Adj_cls::bidir;

	case Tk_cls::inc_dec:
		return Adj_cls::unidir;

	default:
		return Adj_cls::unresolved;
	}
}

// 그 행이 전처리행(§2 제외 대상)인가.
auto sak::Is_preproc_row(Seg_lines const &segs, int const row)->bool{
	if( row >= static_cast<int>(segs.size()) ){
		return false;
	}

	for(Segment const &s : segs[row]){
		if(s.kind == Seg_kind::preproc){
			return true;
		}
	}

	return false;
}

// 그 세그먼트가 피연산자로 서는 리터럴인가.
auto sak::Is_literal_seg(Seg_kind const kind)->bool{
	return
		kind == Seg_kind::string_lit || kind == Seg_kind::char_lit
		|| kind == Seg_kind::raw_string
	;
}

// 파일 전체를 표기 판정용 토큰열로 만든다(행 순서·열 순서). 전처리행은 통째로 뺀다.
auto sak::Tokenize_file(Lines const &mask, Seg_lines const &segs)->std::vector<Adj_tok>{
	std::vector<Adj_tok> out;
	int const rows = static_cast<int>(mask.size());

	for(int r = 0; r < rows; ++r){
		if( Is_preproc_row(segs, r) ){
			continue;
		}

		std::vector<Adj_tok> row_toks;

		for( Tok_8_3 const &t : Tokenize_8_3(mask[r]) ){
			std::string const text = Slice(mask[r], t.col, t.len);

			row_toks.push_back(
				{
					r, t.col, t.len, text, t.cls,
					Lex_to_adj(t.cls, text), Prio::none, false, -1, -1, -1, -1
				}
			);
		}

		if( r < static_cast<int>(segs.size()) ){
			for(Segment const &s : segs[r]){
				if( !Is_literal_seg(s.kind) ){
					continue;
				}

				row_toks.push_back(
					{
						r, s.col, s.len, std::string(), Tk_cls::skip,
						Adj_cls::lit, Prio::none, false, -1, -1, -1, -1
					}
				);
			}
		}

		std::sort(
			row_toks.begin(), row_toks.end(),
			[](Adj_tok const &a, Adj_tok const &b)->bool{
				return a.col < b.col;
			}
		);

		for(Adj_tok &t : row_toks){
			t.prio = Prio_of(t.text, t.cls);
		}

		out.insert(out.end(), row_toks.begin(), row_toks.end());
	}

	return out;
}

// 키워드는 피연산자 꼬리·머리가 아니다. 피연산자로 서는 true·false·nullptr·this 는 뺀다.
auto sak::Is_keyword(std::string const &w)->bool{
	static char const * const  
		Words[]
		= {
			"alignas", "alignof", "and", "asm", "auto", "bool", "break", "case", "catch",
			"char", "char16_t", "char32_t", "char8_t", "class", "co_await", "co_return",
			"co_yield", "concept", "const", "const_cast", "consteval", "constexpr",
			"constinit", "continue", "decltype", "default", "delete", "do", "double",
			"dynamic_cast", "else", "enum", "explicit", "export", "extern", "float", "for",
			"friend", "goto", "if", "inline", "int", "long", "mutable", "namespace", "new",
			"noexcept", "not", "operator", "or", "private", "protected", "public",
			"register", "reinterpret_cast", "requires", "return", "short", "signed",
			"sizeof", "static", "static_assert", "static_cast", "struct", "switch",
			"template", "throw", "try", "typedef", "typeid", "typename", "union",
			"unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while", "xor"
		}
	;

	for(char const * const kw : Words){
		if(w == kw){
			return true;
		}
	}

	return false;
}

// 단항으로 표현식을 열 수 있는 기호형 토큰인가 — 피연산자 머리 판정에 쓴다.
auto sak::Is_unary_prefix(std::string const &t)->bool{
	return
		t == "*" || t == "&" || t == "&&" || t == "+" || t == "-"
		|| t == "!" || t == "~" || t == "++" || t == "--" || t == "..."
	;
}

// 충돌 사각 — 표기만으로 분류를 확정할 수 없는 자리를 용의로 표시한다. 규약이 공표한
// 네 꼴 중 어휘로 가려낼 수 있는 것을 잡는다.
//   (1)(2) 선언이 올 수 있는 자리의 `단어 * 단어` — 잘못 띄어 쓴 선언과 옳게 쓴 연산이 동형.
//   (3) `단어 < 단어 > 단어` — 잘못 벌려 쓴 인스턴스화와 옳게 쓴 비교 연쇄가 동형.
//   (4) 뒤에 떨어진 선언 대상이 오는 피연산 데코레이터 — 잘못 띄어 쓴 선언일 수 있다.
void sak::Mark_suspects(
	std::vector<Adj_tok> &toks, std::vector<int> const &el, std::vector<int> const &er,
	std::vector<int> const &dep
){
	int const n = static_cast<int>(toks.size());

	// 각 토큰을 감싸는 여는 괄호.
	std::vector<int> encl(n, -1);

	{
		std::vector<int> open;

		for(int i = 0; i < n; ++i){
			encl[i] = open.empty() ? -1 : open.back();

			if(toks[i].cls == Adj_cls::open_b){
				open.push_back(i);
			}
			else if(toks[i].cls == Adj_cls::close_b && !open.empty()){
				open.pop_back();
			}
		}
	}

	// 선언이 설 수 있는 괄호 — 이름(비키워드 단어) 뒤에 열린 `(`. 곧 함수의 매개변수 목록이며,
	// 호출의 실인자 목록과 어휘로 구분되지 않는다(사각 2). `if(`·`while(` 처럼 키워드 뒤에 열린
	// 괄호에는 선언이 설 수 없으므로 사각이 아니다.
	auto const  
		decl_paren
		= [&toks](int const e)->bool{
			return
				e > 0 && toks[e].text == "(" && toks[e - 1].cls == Adj_cls::word
				&& !Is_keyword(toks[e - 1].text)
			;
		}
	;

	for(int i = 0; i < n; ++i){
		Adj_tok &t = toks[i];

		bool const  
			word_l
			= el[i] >= 0 && toks[ el[i] ].cls == Adj_cls::word
			&& !Is_keyword(toks[ el[i] ].text)
		;

		bool const  
			word_r
			= er[i] >= 0 && toks[ er[i] ].cls == Adj_cls::word
			&& !Is_keyword(toks[ er[i] ].text)
		;

		// (1)(2) — 양방향으로 판정된 `* & &&` 인데, 그 자리가 선언이 시작될 수 있는 자리다.
		if(
			t.cls == Adj_cls::bidir && word_l && word_r
			&& (t.text == "*" || t.text == "&" || t.text == "&&")
		){
			int const k = el[i] - 1;

			// 문장 머리 — 블록이나 파일 스코프에서만 성립한다. `for(…;…;…)` 의 `;` 는 문장을
			// 끝내는 것이 아니라 구분자이므로, 괄호 안이면 문장 머리가 아니다.
			bool const  
				in_block
				= encl[i] < 0 || toks[ encl[i] ].text == "{"
			;

			bool const  
				stmt_head
				= in_block
				&& (
					k < 0 || toks[k].cls == Adj_cls::semi
					|| toks[k].text == "{" || toks[k].text == "}"
					|| (toks[k].cls == Adj_cls::unidir && toks[k].text == ":")
				)
			;

			bool const  
				param_head
				= (toks[k >= 0 ? k : 0].cls == Adj_cls::comma || toks[k >= 0 ? k : 0].text == "(")
				&& k >= 0 && decl_paren(encl[i])
			;

			if(stmt_head || param_head){
				t.suspect = true;
			}
		}

		// (4) — 피연산 데코레이터 뒤에 공백을 두고 선언 대상처럼 보이는 이름이 온다.
		if(
			t.cls == Adj_cls::operand_like && word_r
			&& (t.text == "*" || t.text == "&" || t.text == "&&")
		){
			t.suspect = true;
		}

		// (3) — `단어 < 단어 > 단어` 꼴. 두 비교 연산자를 함께 지목한다.
		if(t.cls == Adj_cls::bidir && t.text == "<" && word_l && word_r){
			int const j = er[i] + 1;

			if(
				j + 1 < n && dep[j] == dep[i]
				&& toks[j].cls == Adj_cls::bidir && toks[j].text == ">"
				&& toks[j + 1].cls == Adj_cls::word && !Is_keyword(toks[j + 1].text)
			){
				t.suspect = true;
				toks[j].suspect = true;
			}
		}
	}
}

// 표기 판정 — 표기(좌우 공백의 유무와 인접 토큰)로 문맥 의존 글리프의 §4 분류를 확정한다.
// 어느 분류의 합법 표기와도 맞지 않는 자리는 unresolved 로 남는다(§8.4 위반 — S3 이 발화).
void sak::Adjudicate_tokens(std::vector<Adj_tok> &toks){
	int const n = static_cast<int>(toks.size());

	if(n == 0){
		return;
	}

	// gl/gr — 인접 공백 폭(행을 넘으면 개행이 공백을 대신하므로 1). 이웃이 없으면 -1.
	// el/er — §5.3 괄호 경계 투명성을 적용한 유효 이웃(감싸는 괄호는 인접 토큰이 아니다).
	std::vector<int> gl(n, -1), gr(n, -1), el(n, -1), er(n, -1), dep(n, 0);

	for(int i = 0; i < n; ++i){
		if(i > 0){
			Adj_tok const &p = toks[i - 1];

			gl[i] = p.row != toks[i].row ? 1 : toks[i].col - (p.col + p.len);
			el[i] = p.cls == Adj_cls::open_b ? -1 : i - 1;
		}

		if(i + 1 < n){
			Adj_tok const &q = toks[i + 1];

			gr[i] = q.row != toks[i].row ? 1 : q.col - (toks[i].col + toks[i].len);
			er[i] = q.cls == Adj_cls::close_b ? -1 : i + 1;
		}
	}

	std::vector<char> tail(n, 0);

	auto const  
		is_tail
		= [&tail](int const j)->bool{
			return j >= 0 && tail[j] != 0;
		}
	;

	// 피연산자 머리 — 오른쪽에서 피연산자를 이루며 시작할 수 있는 것.
	auto const  
		is_head
		= [&toks](int const j)->bool{
			if(j < 0){
				return false;
			}

			Adj_tok const &t = toks[j];

			if(t.cls == Adj_cls::word){
				return !Is_keyword(t.text);
			}

			return
				t.cls == Adj_cls::lit || t.cls == Adj_cls::open_b
				|| Is_unary_prefix(t.text) || t.text == "::"
			;
		}
	;

	// 데코레이터가 붙을 수 있는 대상 — 선언 변수·구조적 바인딩·중첩 데코레이터·역참조 피연산자.
	// 선언 대상도 역참조 피연산자도 숫자로 시작할 수 없다. 그래서 `f *2` 는 데코레이터로 읽히지
	// 않고, 잘못 붙여 쓴 이항 연산으로서 판정을 물린다.
	auto const  
		is_attachable
		= [&toks, &is_head](int const j)->bool{
			if( !is_head(j) ){
				return false;
			}

			Adj_tok const &t = toks[j];

			if(t.cls == Adj_cls::lit){
				return false;
			}

			return
				t.cls != Adj_cls::word
				|| !std::isdigit( static_cast<unsigned char>(t.text[0]) )
			;
		}
	;

	struct Angle{
		int idx, depth;
	};

	std::vector<Angle> stack;
	int depth = 0;

	// 짝을 잃은 여는 꺾쇠는 판정을 물린다 — 비교로 읽어도, 꺾쇠로 읽어도 어딘가 걸린다.
	auto const  
		flush
		= [&stack, &toks](int const keep)->void{
			while(!stack.empty() && stack.back().depth >= keep){
				toks[stack.back().idx].cls = Adj_cls::unresolved;
				stack.pop_back();
			}
		}
	;

	for(int i = 0; i < n; ++i){
		Adj_tok &t = toks[i];

		// 꺾쇠도 괄호다 — §5.3 경계 투명성이 그대로 적용된다. 여는 꺾쇠는 이미 판정됐고,
		// 닫는 꺾쇠는 아직이므로 "열린 꺾쇠가 있는데 다음이 `>`" 로 미리 알아본다.
		if(el[i] >= 0 && toks[ el[i] ].cls == Adj_cls::angle_open){
			el[i] = -1;
		}

		if(
			er[i] >= 0 && !stack.empty()
			&& (toks[ er[i] ].text == ">" || toks[ er[i] ].text == ">>")
		){
			er[i] = -1;
		}

		bool const  
			att_l = el[i] >= 0 && gl[i] == 0,
			att_r = er[i] >= 0 && gr[i] == 0,
			spc_l = el[i] >= 0 && gl[i] > 0,
			spc_r = er[i] >= 0 && gr[i] > 0
		;

		// §4.1 — operator 이름 안의 기호는 이름의 일부다. 판정 대상이 아니다.
		if(
			el[i] >= 0 && toks[ el[i] ].cls == Adj_cls::word
			&& toks[ el[i] ].text == "operator"
		){
			t.cls = Adj_cls::word;
		}
		else if(t.cls == Adj_cls::unresolved && t.text == "'"){
			t.cls = Adj_cls::word; // 숫자 구분자 — 리터럴의 일부다(`0x1'000`)
		}
		else if(t.cls == Adj_cls::unresolved){
			std::string const &x = t.text;

			if(x == "!" || x == "~"){
				t.cls = Adj_cls::unidir;
			}
			else if(x == "<<"){
				t.cls = Adj_cls::bidir;
			}
			else if(x == "..."){
				t.cls = att_l || att_r ? Adj_cls::unidir : Adj_cls::operand_like;
			}
			else if(x == "::"){
				// 범위해결 `::` 는 양쪽에 붙는다(§8.4). 왼쪽이 떨어져 있으면 왼쪽 피연산자가
				// 없다는 뜻이니 전역 범위 `::` 다 — 앞 행을 닫은 `}` 를 꼬리로 오인하지 않는다.
				t.cls
				= att_l && is_tail(el[i]) ? Adj_cls::bidir : Adj_cls::unidir;
			}
			else if(x == "+" || x == "-"){
				t.cls = is_tail(el[i]) ? Adj_cls::bidir : Adj_cls::unidir;
			}
			else if(x == "*" || x == "&" || x == "&&"){
				bool const mem_ptr = x == "*" && att_l && toks[ el[i] ].text == "::";

				// 데코레이터끼리 엮이면(중첩) 서로 붙는다(§8.4).
				bool const chain_l = att_l && toks[ el[i] ].cls == Adj_cls::unidir;

				if(mem_ptr || chain_l){
					t.cls
					= att_r && is_attachable(er[i]) ? Adj_cls::unidir
					: Adj_cls::operand_like;
				}
				else if(att_l){
					// 왼쪽에 붙었다 — 양방향(양쪽 공백)도, 단방향(반대쪽 공백)도, 피연산
					// (비기호형과 공백)도 될 수 없다. 어느 분류의 합법 표기와도 맞지 않는다.
					t.cls = Adj_cls::unresolved;
				}
				else if( att_r && is_attachable(er[i]) ){
					t.cls = Adj_cls::unidir;
				}
				else if( spc_l && spc_r && is_tail(el[i]) && is_head(er[i]) ){
					t.cls = Adj_cls::bidir;
				}
				else{
					// 붙을 선언 대상도, 양옆의 피연산자도 없다 — 피연산 데코레이터다.
					t.cls = Adj_cls::operand_like;
				}
			}
			else if(x == "<"){
				if(att_l && toks[ el[i] ].cls == Adj_cls::word){
					t.cls = Adj_cls::angle_open;
					stack.push_back({ i, depth });
				}
				else if(spc_l && spc_r){
					t.cls = Adj_cls::bidir;
				}
			}
			else if(x == ">" || x == ">>"){
				if(!stack.empty() && stack.back().depth == depth){
					t.cls = Adj_cls::angle_close;
					stack.pop_back();

					if(x == ">>" && !stack.empty() && stack.back().depth == depth){
						stack.pop_back();
					}
				}
				else if(spc_l && spc_r){
					t.cls = Adj_cls::bidir;
				}
			}
			else if(x == ":"){
				if(att_l){
					t.cls = Adj_cls::unidir;
				}
				else if(spc_l && spc_r){
					t.cls = Adj_cls::bidir;
				}
			}
		}
		else if(t.cls == Adj_cls::comma && er[i] < 0){
			t.cls = Adj_cls::unidir; // 트레일링 콤마 — 뒤가 닫는 괄호다
		}

		// 괄호 안에 홀로 선 기호형 토큰은 이웃이 없다(§5.3) — 연산자일 수 없으니 피연산이다.
		// 람다 캡처 기본값 `[=]` `[&]` 가 이 자리다(§4 표).
		if(el[i] < 0 && er[i] < 0 && t.cls == Adj_cls::bidir){
			t.cls = Adj_cls::operand_like;
		}

		if(t.cls == Adj_cls::open_b){
			if(t.text == "{"){
				flush(0);
			}

			++depth;
		}
		else if(t.cls == Adj_cls::close_b){
			flush(depth);

			if(depth > 0){
				--depth;
			}

			if(t.text == "}"){
				flush(0);
			}
		}
		else if(t.cls == Adj_cls::semi){
			flush(0);
		}

		dep[i] = depth;

		// 후위 ++ -- 는 피연산자 꼬리로 선다(그 왼쪽에 붙어 있을 때).
		bool const  
			postfix
			= t.cls == Adj_cls::unidir && (t.text == "++" || t.text == "--")
			&& att_l && is_tail(el[i])
		;

		tail[i]
		= t.cls == Adj_cls::lit || t.cls == Adj_cls::close_b
		|| t.cls == Adj_cls::operand_like || t.cls == Adj_cls::angle_close
		|| ( t.cls == Adj_cls::word && !Is_keyword(t.text) )
		|| postfix;
	}

	flush(0);
	Mark_suspects(toks, el, er, dep);

	for(int i = 0; i < n; ++i){
		Adj_tok &t = toks[i];

		t.gl = gl[i];
		t.gr = gr[i];
		t.el = el[i];
		t.er = er[i];

		t.prio
		= t.cls == Adj_cls::angle_open || t.cls == Adj_cls::angle_close ? Prio::bracket
		: Prio_of(t.text, t.cls);
	}
}

// 표기 판정이 확정한 꺾쇠 짝을 뽑는다.
//
// 레거시 매처(Match_template_cast_angles·Match_closer_anchored_angles)는 꺾쇠임이 **문법으로**
// 확정되는 자리 — `template`·`*_cast` 키워드 앵커, 그리고 닫는 `>` 뒤의 닫힘 신호 — 만 잡을 수
// 있었다. 그래서 `std::vector<int> v;` 나 `std::is_same<A, B>::value` 처럼 가장 흔한 자리를
// 통째로 놓쳤다. 표기 판정은 그 자리를 **표기로** 확정한다 — `vector<` 가 붙어 있다는 사실 자체가
// 꺾쇠라는 선언이다. 짝을 잃은 여는 꺾쇠는 이미 unresolved 로 물려 있으므로, 여기 남는 것은
// 확정된 짝뿐이다.
//
// 괄호 안에 든 꺾쇠도 스택이 그대로 세므로(레거시 역스캔은 `(...)` 를 통째로 건너뛰었다),
// §8.5 의 중첩 단계도 이제 정확하다.
auto sak::Adjudicated_angles(std::vector<Adj_tok> const &toks)->std::vector<Angle_pair>{
	std::vector<Angle_pair> out;
	std::vector<int> stack;
	int const n = static_cast<int>(toks.size());

	for(int i = 0; i < n; ++i){
		if(toks[i].cls == Adj_cls::angle_open){
			stack.push_back(i);
		}
		else if(toks[i].cls == Adj_cls::angle_close){
			int pops = toks[i].len == 2 ? 2 : 1;

			while(pops-- > 0 && !stack.empty()){
				int const o = stack.back();
				stack.pop_back();
				out.push_back({ toks[o].row, toks[o].col, toks[i].row, toks[i].col });
			}
		}
	}

	return out;
}

// 덤프용 이름표.
auto sak::Adj_name(Adj_cls const cls)->char const *{
	switch(cls){
	case Adj_cls::word:
		return "word";

	case Adj_cls::lit:
		return "lit";

	case Adj_cls::open_b:
		return "open";

	case Adj_cls::close_b:
		return "close";

	case Adj_cls::semi:
		return "semi";

	case Adj_cls::comma:
		return "comma";

	case Adj_cls::bidir:
		return "bidir";

	case Adj_cls::unidir:
		return "unidir";

	case Adj_cls::operand_like:
		return "operand";

	case Adj_cls::angle_open:
		return "angle_open";

	case Adj_cls::angle_close:
		return "angle_close";

	default:
		return "unresolved";
	}
}

auto sak::Prio_name(Prio const prio)->char const *{
	static char const * const  
		Names[]
		= {
			"semi", "comma",
			"assign", "ternary", "lor", "land", "bor", "bxor", "band",
			"eq", "rel", "spaceship", "shift", "add", "mul",
			"vop",
			"bracket",
			"mem_ptr", "mem", "str_adj", "scope",
			"-"
		}
	;

	return Names[static_cast<int>(prio)];
}

auto sak::render_classes(Lines const &lines, Seg_lines const &segs)->Lines{
	Lines const mask = render_mask(lines, segs);
	std::vector<Adj_tok> toks = Tokenize_file(mask, segs);
	Adjudicate_tokens(toks);
	Lines out;

	for(Adj_tok const &t : toks){
		std::string s = std::to_string(t.row + 1) + ":" + std::to_string(t.col + 1) + " ";
		s += Adj_name(t.cls);
		s += " ";
		s += Prio_name(t.prio);
		s += " ";
		s += t.cls == Adj_cls::lit ? std::string("<lit>") : t.text;

		if(t.suspect){
			s += " (suspect)";
		}

		out.push_back(s);
	}

	return out;
}
