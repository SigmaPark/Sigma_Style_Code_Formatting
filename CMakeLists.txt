cmake_minimum_required(VERSION 3.16)

project(sak LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 단일 설정 생성기(Makefiles/Ninja)에서 기본 빌드 타입을 정해 둔다.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE Release)
endif()

# 생성기 종류(단일/다중 설정)와 무관하게 실행 파일을 항상 build/ 바로 아래에 둔다.
# 그래야 setup 스크립트가 build/sigma_checker(.exe) 한 경로만 알면 된다.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
foreach(cfg ${CMAKE_CONFIGURATION_TYPES})
	string(TOUPPER ${cfg} CFG)
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG} ${CMAKE_BINARY_DIR})
endforeach()

# app/ 안의 모든 .cpp 를 예제 겸 검사 대상으로 묶는다. 파일을 추가하면 재구성된다.
file(GLOB APP_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/app/*.cpp)

add_executable(sak ${APP_SOURCES})

# 소스를 UTF-8 로 읽도록 한다(한글 주석에 대한 MSVC C4819 경고 제거).
if(MSVC)
	target_compile_options(sak PRIVATE /utf-8)
endif()
