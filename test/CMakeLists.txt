#	SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
#	SPDX-License-Identifier: MIT License
#=========#=========#=========#=========#=========#=========#=========#=========#=========#=========

# How2use 를 의존성으로 당겨온다. 오프라인이면 FETCHCONTENT_SOURCE_DIR_HOW2USE 로 로컬 클론을
# 가리켜 네트워크 없이 구성할 수 있다(예: -DFETCHCONTENT_SOURCE_DIR_HOW2USE=../How2use).
include(FetchContent)
FetchContent_Declare(
	How2use
	GIT_REPOSITORY https://github.com/SigmaPark/How2use.git
	GIT_TAG v2.3.1
)

set(BUILD_HOW2USE_TEST_PROJ OFF CACHE BOOL "Disable How2use's own guide build" FORCE)
FetchContent_MakeAvailable(How2use)

if(MSVC)
	add_definitions(-D_WINDOWS_SOLUTION_)
	add_compile_options(/Zc:__cplusplus)
	add_definitions(-DUNICODE)
	add_definitions(-D_UNICODE)
endif()

if(UNIX)
	add_definitions(-D_POSIX_SOLUTION_)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

# sak 검사기의 코어(main.cpp 를 제외한 전부)를 정적 라이브러리로 묶어 테스트가 직접 링크한다.
# check.cpp 는 이미 check.hpp/lexer.hpp 뒤의 순수 함수라 분리 리팩터가 필요 없다.
add_library(
	sak_core STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/../app/check.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../app/lexer.cpp
)

target_include_directories(sak_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../app)

if(MSVC)
	target_compile_options(sak_core PRIVATE /utf-8)
endif()

# test/ 아래 모든 .cpp 를 테스트 실행파일로 묶는다. 케이스 모듈을 추가하면 재구성된다.
file(
	GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
	${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

add_executable(sak_test ${TEST_SOURCES})

target_include_directories(sak_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(sak_test PRIVATE sak_core How2use_lib)

if(MSVC)
	target_compile_options(sak_test PRIVATE /utf-8)
endif()
