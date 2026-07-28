/*  SPDX-FileCopyrightText: (c) 2020 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>
#include <list>

#include "lexer.hpp"
#include "check.hpp"

namespace fs = std::filesystem;

namespace sak{
	struct Line_range;
	struct Raw_file;

	static auto Read_lines(std::string const &path)->Lines;
	static auto is_target_source(fs::path const &p)->bool;
	static auto Collect_sources(fs::path const &dir, bool const recur)->std::list<std::string>;
	static auto Parse_line_range(std::string const &s, Line_range &out)->bool;
	static auto Read_raw(std::string const &path, Raw_file &out)->bool;

	static auto Check_file(
		std::string const &path, Line_range const range, std::size_t &suspects
	)->std::size_t;

	static void Dump_classes(std::string const &path);
	static auto Range_to_lo_hi(Line_range const range, int &lo, int &hi)->void;

	static auto Edit_file(
		std::string const &path, Line_range const range, bool const dry
	)->std::size_t;
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

// 1-기준 [start, end] 라인 범위. 기본값은 파일 전체.
struct sak::Line_range{
	std::size_t start;
	std::size_t end;
};

// 원문 바이트를 행 내용과 종결자로 갈라 담는다 — edit 이 개행 종류·마지막 개행을 그대로 보존한다.
struct sak::Raw_file{
	Lines contents;
	std::vector<std::string> terms;
};
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

// 파일을 행 단위로 읽는다(말미의 '\r' 는 제거).
auto sak::Read_lines(std::string const &path)->Lines{
	Lines lines;

	std::ifstream in(path);
	std::string line;

	while( std::getline(in, line) ){
		if(!line.empty() && line.back() == '\r'){
			line.pop_back();
		}

		lines.push_back(line);
	}

	return lines;
}

// 검사 대상 확장자(.cpp .hpp .cu .cuh .h)를 가진 일반 파일인지 판별한다.
auto sak::is_target_source(fs::path const &p)->bool{
	if( std::error_code ec; !fs::is_regular_file(p, ec) ){
		return false;
	}

	std::string ext = p.extension().string();

	for(char &c : ext){
		c = static_cast<char>(  std::tolower( static_cast<unsigned char>(c) )  );
	}

	return ext == ".cpp" || ext == ".hpp" || ext == ".cu" || ext == ".cuh" || ext == ".h";
}

// dir 아래의 대상 소스 경로(검사용 전체 경로)를 모은다. recursive면 하위까지.
// 출력은 사전식 정렬.
auto sak::Collect_sources(fs::path const &dir, bool const recur)->std::list<std::string>{
	std::list<std::string> out;

	if(recur){
		fs::recursive_directory_iterator const end;
		std::error_code ec;

		for( fs::recursive_directory_iterator it(dir, ec); it != end; ++it ){
			if( is_target_source(it->path()) ){
				out.emplace_back(it->path().generic_string());
			}
		}
	}
	else{
		fs::directory_iterator const end;
		std::error_code ec;

		for( fs::directory_iterator it(dir, ec); it != end; ++it ){
			if( is_target_source(it->path()) ){
				out.emplace_back(it->path().generic_string());
			}
		}
	}

	out.sort();

	return out;
}

// "a:b" / "a:" / ":b" 를 파싱한다. 성공 시 true. 양쪽 모두 1 이상이어야 하며 start <= end.
auto sak::Parse_line_range(std::string const &s, Line_range &out)->bool{
	std::size_t const colon = s.find(':');

	if(colon == std::string::npos){
		return false;
	}

	std::string const left = s.substr(0, colon);
	std::string const right = s.substr(colon + 1);

	std::size_t start = 1;
	std::size_t end = std::numeric_limits<std::size_t>::max();

	auto const  
		parse_one
		= [](std::string const &tok, std::size_t &out_v)->bool{
			if(tok.empty()){
				return true;
			}

			for(char const c : tok){
				if(  !std::isdigit( static_cast<unsigned char>(c) )  ){
					return false;
				}
			}

			try{
				std::size_t pos = 0;
				unsigned long long const v = std::stoull(tok, &pos);

				if(pos != tok.size() || v < 1){
					return false;
				}

				out_v = static_cast<std::size_t>(v);
			}
			catch(...){
				return false;
			}

			return true;
		}
	;

	if( !parse_one(left, start) ){
		return false;
	}

	if( !parse_one(right, end) ){
		return false;
	}

	if(start > end){
		return false;
	}

	out = Line_range{ start, end };

	return true;
}

// 파일을 바이너리로 읽어 Raw_file 로 분해한다(내용은 Read_lines 와 동일하게 '\r' 를 뗀다).
// 열기 실패면 false.
auto sak::Read_raw(std::string const &path, Raw_file &out)->bool{
	std::ifstream in(path, std::ios::binary);

	if(!in){
		return false;
	}

	std::ostringstream ss;
	ss << in.rdbuf();
	std::string const buf = ss.str();
	std::size_t const n = buf.size();
	std::size_t i = 0;

	while(i < n){
		std::size_t j = i;

		while(j < n && buf[j] != '\n'){
			++j;
		}

		std::size_t e = j < n ? j : n;
		std::string term = j < n ? "\n" : "";

		if(e > i && buf[e - 1] == '\r'){
			--e;
			term = j < n ? "\r\n" : "\r";
		}

		std::string const content = buf.substr(i, e - i);
		out.contents.push_back(content);
		out.terms.push_back(term);
		i = j < n ? j + 1 : n;
	}

	return true;
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

// 한 파일을 검사해 위반을 출력하고, 위반 개수를 돌려준다. 범위 밖의 위반은 건너뛴다.
// 용의는 [suspect] 로 함께 출력하되 반환값(=종료코드)에 넣지 않고 suspects 로만 센다.
auto sak::Check_file(
	std::string const &path, Line_range const range, std::size_t &suspects
)->std::size_t{
	Raw_file raw;

	if( !Read_raw(path, raw) ){
		std::cerr << "error: cannot read: " << path << "\n";

		return 0;
	}

	bool const final_nl = raw.terms.empty() || !raw.terms.back().empty();
	Lines const &lines = raw.contents;
	Seg_lines const segs = scan_lines(lines);
	std::vector<Violation> const violations = check_lines(lines, segs, final_nl);

	std::size_t hits = 0;

	for(Violation const &v : violations){
		std::size_t const line_no = static_cast<std::size_t>(v.row) + 1;

		if(line_no < range.start || line_no > range.end){
			continue;
		}

		bool const is_suspect = v.cat == V_cat::suspect;

		std::cout
		<< path << ":" << v.row + 1 << ":" << v.col + 1
		<< " [" << v.rule << "] " << v.message
		<< (is_suspect ? " [suspect]" : "") << "\n";

		if(is_suspect){
			++suspects;
		}
		else{
			++hits;
		}
	}

	return hits;
}

// 검증용 — 표기 판정 스트림을 그대로 덤프한다(개발자 진단용 숨은 플래그).
void sak::Dump_classes(std::string const &path){
	Lines const lines = Read_lines(path);
	Seg_lines const segs = scan_lines(lines);

	for( std::string const &s : render_classes(lines, segs) ){
		std::cout << path << ":" << s << "\n";
	}
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

// range(1-기준 [start,end]) 를 edit_lines 용 0-기준 포함범위 [lo,hi] 로 바꾼다.
auto sak::Range_to_lo_hi(Line_range const range, int &lo, int &hi)->void{
	int const int_max = std::numeric_limits<int>::max();

	lo = range.start > 1 ? static_cast<int>(range.start) - 1 : 0;

	hi
	= range.end >= static_cast<std::size_t>(int_max) ? int_max
	: static_cast<int>(range.end) - 1;
}

// 한 파일을 edit 한다. 자동교정·범위밖(manual) 기록을 출력하고, dry 가 아니면 원자적으로 쓴다.
// 남은 manual 위반 개수를 돌려준다(종료코드용).
auto sak::Edit_file(std::string const &path, Line_range const range, bool const dry)->std::size_t{
	Raw_file raw;

	if( !Read_raw(path, raw) ){
		std::cerr << "error: cannot read: " << path << "\n";

		return 0;
	}

	int lo = 0, hi = 0;
	Range_to_lo_hi(range, lo, hi);

	bool const final_nl = raw.terms.empty() || !raw.terms.back().empty();
	Edit_result const res = edit_lines(raw.contents, lo, hi, final_nl);

	if(!res.ok){
		std::cerr << path << ": edit aborted (regression gate failed); left unchanged\n";

		return 0;
	}

	std::size_t manual = 0;

	for(Edit_note const &note : res.notes){
		char const *tail = " [manual]";

		if(note.cat == V_cat::suspect){
			tail = " [suspect]";
		}
		else if(note.fixed){
			tail = " (fixed)";
		}

		std::cout
		<< path << ":" << note.row + 1 << ":" << note.col + 1
		<< " [" << note.rule << "] " << note.message << tail << "\n";

		if(note.cat == V_cat::violation && !note.fixed){
			++manual;
		}
	}

	if(!dry && res.lines != raw.contents){
		std::string buf;

		for(std::size_t i = 0; i < res.lines.size(); ++i){
			buf += res.lines[i];
			buf += raw.terms[i];
		}

		std::string const tmp = path + ".sak_tmp";

		{
			std::ofstream fout(tmp, std::ios::binary | std::ios::trunc);

			if(!fout){
				std::cerr << "error: cannot write: " << path << "\n";

				return manual;
			}

			fout << buf;
		}

		std::error_code ec;
		fs::rename(tmp, path, ec);

		if(ec){
			std::cerr << "error: cannot replace: " << path << "\n";
		}
	}

	return manual;
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

auto main(int const argc, char const * const *argv)->int{
	std::vector<std::string> const  
		args
		= [begin = argv + 1, n = argc - 1]()->std::vector<std::string>{
			return{ begin, begin + n };
		}()
	;

	auto const  
		print_usage
		= []()->void{
			std::cerr
			<< "usage:\n"
			<< "  sak <file>\n"
			<< "  sak <directory> [-r]\n"
			<< "  sak <file> -b <start>:<end>    (e.g. 3:50, 3:, :50)\n"
			<< "  sak edit <file|directory> [-r] [-b <start>:<end>] [-n|--dry-run]\n";
		}
	;

	// 첫 토큰이 edit 이면 edit 모드, 나머지 인수는 기존 파서에 그대로 흘린다.
	bool const edit_mode = !args.empty() && args[0] == "edit";
	std::size_t const arg_start = edit_mode ? 1 : 0;

	bool recur = false;
	bool has_range = false;
	bool dry = false;
	bool classes = false;

	sak::Line_range range{ 1, std::numeric_limits<std::size_t>::max() };

	std::string target;
	bool parse_ok = true;

	for(std::size_t i = arg_start; i < args.size(); ++i){
		std::string const &a = args[i];

		if(a == "-r"){
			recur = true;
		}
		else if(a == "-n" || a == "--dry-run"){
			dry = true;
		}
		else if(a == "--classes"){
			classes = true;
		}
		else if(a == "-b"){
			if(i + 1 >= args.size()){
				std::cerr << "error: -b requires a range argument (e.g. 3:50, 3:, :50)\n";
				parse_ok = false;
				break;
			}

			++i;

			if( !sak::Parse_line_range(args[i], range) ){
				std::cerr << "error: invalid -b range: " << args[i] << "\n";
				parse_ok = false;
				break;
			}

			has_range = true;
		}
		else if(target.empty()){
			target = a;
		}
		else{
			std::cerr << "error: unexpected argument: " << a << "\n";
			parse_ok = false;
			break;
		}
	}

	if(!parse_ok || target.empty()){
		print_usage();

		return 2;
	}

	if(has_range && recur){
		std::cerr << "error: -b cannot be combined with -r\n";

		return 2;
	}

	if(dry && !edit_mode){
		std::cerr << "error: -n/--dry-run is only valid with 'edit'\n";

		return 2;
	}

	if(classes && edit_mode){
		std::cerr << "error: --classes is a check-mode diagnostic\n";

		return 2;
	}
	//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

	fs::path const path = target;

	if( std::error_code ec; fs::is_regular_file(path, ec) ){
		if(recur){
			std::cerr << "error: -r requires a directory\n";

			return 2;
		}

		if(edit_mode){
			return sak::Edit_file(target, range, dry) == 0 ? 0 : 1;
		}

		if(classes){
			sak::Dump_classes(target);

			return 0;
		}

		std::size_t suspects = 0;

		return sak::Check_file(target, range, suspects) == 0 ? 0 : 1;
	}

	if( std::error_code ec; fs::is_directory(path, ec) ){
		if(has_range){
			std::cerr << "error: -b requires a file, not a directory\n";

			return 2;
		}

		if(edit_mode){
			std::size_t manual = 0, files = 0;

			for( std::string const &f : sak::Collect_sources(path, recur) ){
				manual += sak::Edit_file(f, range, dry);
				++files;
			}

			std::cout << "[sak edit] " << files << " file(s), " << manual << " manual item(s)\n";

			return manual == 0 ? 0 : 1;
		}

		std::size_t total = 0, files = 0, suspects = 0;

		for( std::string const &f : sak::Collect_sources(path, recur) ){
			if(classes){
				sak::Dump_classes(f);
			}
			else{
				total += sak::Check_file(f, range, suspects);
			}

			++files;
		}

		if(classes){
			return 0;
		}

		std::cout << "[sak] " << files << " file(s), " << total << " violation(s)";

		if(suspects != 0){
			std::cout << ", " << suspects << " suspect(s)";
		}

		std::cout << "\n";

		return total == 0 ? 0 : 1;
	}

	std::cerr << "error: no such file or directory: " << target << "\n";

	return 2;
}
