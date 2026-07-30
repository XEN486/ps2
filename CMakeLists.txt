cmake_minimum_required(VERSION 3.24)
project(ps2 VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SRC_FILES src/*.cpp)

set(ASMJIT_EMBED ON)
set(ASMJIT_STATIC ON)
add_subdirectory(thirdparty/asmjit)

add_executable(ps2 ${SRC_FILES})
target_link_libraries(ps2
	asmjit::asmjit
)