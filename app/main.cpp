/*  SPDX-FileCopyrightText: (c) 2020 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include <cctype>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>
#include <list>

#include "lexer.hpp"
#include "check.hpp"

namespace fs = std::filesystem;
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

// 파일을 행 단위로 읽는다(말미의 '\r' 는 제거).
static auto Read_lines(std::string const &path)->Lines{
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
static auto is_target_source(fs::path const &p)->bool{
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
static auto Collect_sources(fs::path const &dir, bool const recur)->std::list<std::string>{
	std::list<std::string> out;

	if(recur){
		fs::recursive_directory_iterator const end;
		std::error_code ec;

		for( fs::recursive_directory_iterator it(dir, ec); it != end; ++it ){
			if( ::is_target_source(it->path()) ){
				out.emplace_back(it->path().generic_string());
			}
		}
	} else{
		fs::directory_iterator const end;
		std::error_code ec;

		for( fs::directory_iterator it(dir, ec); it != end; ++it ){
			if( ::is_target_source(it->path()) ){
				out.emplace_back(it->path().generic_string());
			}
		}
	}

	out.sort();

	return out;
}

// 한 파일을 검사해 위반을 출력하고, 위반 개수를 돌려준다.
static auto Check_file(std::string const &path)->std::size_t{
	Lines const lines = ::Read_lines(path);
	Seg_lines const segs = ::scan_lines(lines);
	std::vector<Violation> const violations = ::check_lines(lines, segs);

	for(Violation const &v : violations){
		std::cout
		<< path << ":" << v.row + 1 << ":" << v.col + 1
		<< " [" << v.rule << "] " << v.message << "\n";
	}

	return violations.size();
}
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

auto main(int const argc, char const * const *argv)->int{
	std::vector<std::string> const
		args
		= [begin = argv + 1, n = argc - 1]()->std::vector<std::string>{
			return{ begin, begin + n };
		}()
	;

	bool const recur = !args.empty() && args[0] == "-r";

	std::string const
		target
		= [recur](std::vector<std::string> const &args)->std::string{
			if(recur){
				if(args.size() < 2){
					std::cerr << "usage: sak -r <directory>\n";

					return{};
				} else{
					return args[1];
				}
			} else if(!args.empty()){
				return args[0];
			} else{
				std::cerr << "usage: sak <file|directory> | sak -r <directory>\n";

				return{};
			}
		}(args)
	;

	if(target.empty()){
		return 2;
	}
	//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

	fs::path const path = target;

	if( std::error_code ec; fs::is_regular_file(path, ec) ){
		if(recur){
			std::cerr << "error: -r requires a directory\n";

			return 2;
		}

		return ::Check_file(target) == 0 ? 0 : 1;
	}

	if( std::error_code ec; fs::is_directory(path, ec) ){
		std::size_t total = 0;
		std::size_t files = 0;
		for( std::string const &f : ::Collect_sources(path, recur) ){
			total += ::Check_file(f);
			++files;
		}

		std::cout << "[sak] " << files << " file(s), " << total << " violation(s)\n";

		return total == 0 ? 0 : 1;
	}

	std::cerr << "error: no such file or directory: " << target << "\n";

	return 2;
}
