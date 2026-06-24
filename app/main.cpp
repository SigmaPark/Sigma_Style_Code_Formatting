/*  SPDX-FileCopyrightText: (c) 2020 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
*   SPDX-License-Identifier: MIT License
*/
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

#include <cctype>
#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>
#include <list>

namespace fs = std::filesystem;
//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$//--//--//--//--//-$

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

// 끝에 '/'가 정확히 하나 오도록 정규화한다("app" 또는 "app\" -> "app/").
static auto With_trailing_slash(std::string s)->std::string{
	while( !s.empty() && (s.back() == '/' || s.back() == '\\') ){
		s.pop_back();
	}

	s += "/";
	
	return s;
}

// dir 아래의 대상 소스를 모은다. recursive면 하위 디렉터리까지 훑고
// dir 기준 상대경로를, 아니면 파일명만 담는다. 출력은 사전식으로 정렬한다.
static auto Collect_sources(fs::path const &dir, bool const recur)->std::list<std::string>{
	std::list<std::string> out;
	
	if(recur){
		fs::recursive_directory_iterator const end;
		std::error_code ec;

		for( fs::recursive_directory_iterator it(dir, ec); it != end; ++it ){
			if( ::is_target_source(it->path()) ){
				fs::path const rel = fs::relative(it->path(), dir, ec);

				out.emplace_back(rel.generic_string());
			}
		}
	} else{
		fs::directory_iterator const end;
		std::error_code ec;
		
		for( fs::directory_iterator it(dir, ec); it != end; ++it ){
			if( ::is_target_source(it->path()) ){
				out.emplace_back(it->path().filename().generic_string());
			}
		}
	}

	out.sort();

	return out;
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

		std::cout << "Hello " << target << "\n";
		
		return 0;
	}

	if( std::error_code ec; fs::is_directory(path, ec) ){
		std::cout << "Hello " << ::With_trailing_slash(target) << "\n";

		auto const files = ::Collect_sources(path, recur);

		for(std::string const &f : files){
			std::cout << f << "\n";
		}

		return 0;
	}

	std::cerr << "error: no such file or directory: " << target << "\n";

	return 2;
}
