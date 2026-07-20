#	SPDX-FileCopyrightText: (c) 2026 Jin-Eon Park <greengb@naver.com> <sigma@gm.gist.ac.kr>
#	SPDX-License-Identifier: MIT License
#=========#=========#=========#=========#=========#=========#=========#=========#=========#=========

# Pull How2use in as a dependency. Offline, point FETCHCONTENT_SOURCE_DIR_HOW2USE at a local
# clone to configure without the network (e.g. -DFETCHCONTENT_SOURCE_DIR_HOW2USE=../How2use).
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

# Bundle the sak checker core (everything but main.cpp) as a static library the tests link.
# check.cpp is already a pure function behind check.hpp/lexer.hpp, so it needs no split refactor.
add_library(
	sak_core STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/../app/check.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../app/lexer.cpp
)

target_include_directories(sak_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../app)

if(MSVC)
	target_compile_options(sak_core PRIVATE /utf-8)
endif()

# Gather every .cpp under test/ into the test executable. Adding a case module reconfigures it.
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
