cmake_minimum_required(VERSION 3.16)

project(codegen_engine DESCRIPTION "Compiler project" VERSION 1.0 LANGUAGES CXX)

find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    message(STATUS "Found ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
else()
    message(STATUS "ccache not found, building without cache")
endif()

include(GNUInstallDirs)

enable_language(C)
enable_language(CXX)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
else()
    message(FATAL_ERROR "invalid CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE}")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE INTERNAL "")
set(CMAKE_INSTALL_MESSAGE LAZY)

set(CMAKE_CXX_FLAGS "-ftemplate-backtrace-limit=20 -Wall -fno-exceptions -fno-rtti -Wextra -Werror -Wfloat-conversion -Wconversion -Wpedantic -Wstrict-aliasing")
include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-fconcepts-diagnostics-depth=10 FLAG_CXX_CONCEPT_DIAGNOSTIC)
if (FLAG_CXX_CONCEPT_DIAGNOSTIC)
    add_compile_options(-fconcepts-diagnostics-depth=10)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options(-fdiagnostics-color=always)
    # TODO{vibhanshu}: enable _GLIBCXX_DEBUG flags in Debug mode
    string(APPEND CMAKE_CXX_FLAGS_DEBUG " -O0 -ggdb3 -finline-small-functions -D_GLIBCXX_DEBUG_PEDANTIC=1 -D_GLIBCXX_SANITIZE_VECTOR=1 -gz=zlib")
    string(APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO " -O3 -ggdb3 -fno-omit-frame-pointer -fuse-linker-plugin -fdevirtualize-at-ltrans -gz=zlib -Wno-maybe-uninitialized")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " -O3 -DCORE_RELEASE -DNDEBUG -s -fuse-linker-plugin -fdevirtualize-at-ltrans")
    # string(APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO " -O3 -ggdb3 -fno-omit-frame-pointer -flto=8 -fuse-linker-plugin -fdevirtualize-at-ltrans -gz=zlib -Wno-maybe-uninitialized")
    # string(APPEND CMAKE_CXX_FLAGS_RELEASE " -O3 -DCORE_RELEASE -DNDEBUG -s -flto=8 -fuse-linker-plugin -fdevirtualize-at-ltrans")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
    string(APPEND CMAKE_CXX_FLAGS_DEBUG " -O0 -ggdb -finline-functions -D_GLIBCXX_DEBUG_PEDANTIC=1 -D_GLIBCXX_SANITIZE_VECTOR=1 -gz=zlib -fsanitize=undefined")
    string(APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO " -O3 -ggdb -fno-omit-frame-pointer -gz=zlib -fsanitize=memory -fsanitize-memory-track-origins -fsanitize-recover=memory")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " -O3 -g0 -DCORE_RELEASE -DNDEBUG ")
    # string(APPEND CMAKE_CXX_FLAGS_RELEASE " -O3 -g0 -DCORE_RELEASE -DNDEBUG -flto=auto")
endif()

message(">> Define module:${PROJECT_NAME}")

add_subdirectory(core)
# add_subdirectory(grammar)
