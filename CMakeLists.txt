cmake_minimum_required(VERSION 3.24)
project(ps2 VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SRC_FILES src/*.cpp)

set(ASMJIT_EMBED ON)
set(ASMJIT_STATIC ON)
add_subdirectory(thirdparty/asmjit)

set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(SDL_TEST_LIBRARY OFF)
set(SDL_TESTS OFF)
set(SDL_DISABLE_INSTALL ON)
set(SDL_DISABLE_INSTALL_DOCS ON)
set(SDL_INSTALL_TESTS OFF)

add_subdirectory(thirdparty/SDL)

add_executable(ps2 ${SRC_FILES})
target_link_libraries(ps2
	asmjit::asmjit
	SDL3::SDL3
)


if(MSVC)
	target_compile_options(ps2 PRIVATE /W4)
else()
	target_compile_options(ps2 PRIVATE -Wall -Wextra)
endif()