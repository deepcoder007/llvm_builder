cmake_minimum_required(VERSION 3.16)

project(llvm_builder DESCRIPTION "LLvm representation builder" VERSION 1.0 LANGUAGES CXX)

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

set(CMAKE_CXX_FLAGS "-ftemplate-backtrace-limit=20")

# Strict warning flags for the main library
set(LLVM_BUILDER_CXX_FLAGS
    -fno-exceptions
    -fno-rtti
    -Wall
    -Wextra
    -Werror
    -Wfloat-conversion
    -Wconversion
    -Wpedantic
    -Wstrict-aliasing
    -Wno-unused-parameter  # Allow unused parameters in llvm_builder
)
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
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " -O3 -DLLVM_BUILDER_RELEASE -DNDEBUG -s -fuse-linker-plugin -fdevirtualize-at-ltrans")
    # string(APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO " -O3 -ggdb3 -fno-omit-frame-pointer -flto=8 -fuse-linker-plugin -fdevirtualize-at-ltrans -gz=zlib -Wno-maybe-uninitialized")
    # string(APPEND CMAKE_CXX_FLAGS_RELEASE " -O3 -DLLVM_BUILDER_RELEASE -DNDEBUG -s -flto=8 -fuse-linker-plugin -fdevirtualize-at-ltrans")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_compile_options(-fcolor-diagnostics)
    string(APPEND CMAKE_CXX_FLAGS_DEBUG " -O0 -ggdb -finline-functions -D_GLIBCXX_DEBUG_PEDANTIC=1 -D_GLIBCXX_SANITIZE_VECTOR=1 -gz=zlib -fsanitize=undefined")
    string(APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO " -O3 -ggdb -fno-omit-frame-pointer -gz=zlib -fsanitize=memory -fsanitize-memory-track-origins -fsanitize-recover=memory")
    string(APPEND CMAKE_CXX_FLAGS_RELEASE " -O3 -g0 -DLLVM_BUILDER_RELEASE -DNDEBUG ")
    # string(APPEND CMAKE_CXX_FLAGS_RELEASE " -O3 -g0 -DLLVM_BUILDER_RELEASE -DNDEBUG -flto=auto")
endif()

set(MODULE_NAME "${PROJECT_NAME}")

find_package(LLVM REQUIRED CONFIG)
message("Found LLVM ${LLVM_PACKAGE_VERSION}")
message("Using LLVMConfig.cmake in ${LLVM_DIR}")
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})
include_directories(include)

llvm_map_components_to_libnames(llvm_libs
  Core
  CodeGen
  ExecutionEngine
  IRReader
  native
  OrcJIT
  OrcDebugging
  )

file(GLOB_RECURSE ${MODULE_NAME}_sources CONFIGURE_DEPENDS "src/*/*.cpp")
list(FILTER ${MODULE_NAME}_sources EXCLUDE REGEX "\\.[xt]\\.cpp$")
message(VERBOSE "List of identified source files: ${${MODULE_NAME}_sources}")

add_library(${MODULE_NAME}_headers INTERFACE)
target_include_directories(
    ${MODULE_NAME}_headers INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

add_library(${MODULE_NAME} STATIC ${${MODULE_NAME}_sources})
target_link_libraries(${MODULE_NAME} PUBLIC ${MODULE_NAME}_headers)
target_compile_options(${MODULE_NAME} PRIVATE ${LLVM_BUILDER_CXX_FLAGS})
set_target_properties(${MODULE_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Precompiled headers for LLVM (significantly speeds up compilation)
option(USE_PCH "Use precompiled headers for LLVM" ON)
if(USE_PCH)
    target_precompile_headers(${MODULE_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/llvm/pch.h
    )
    message(STATUS "PCH enabled for ${MODULE_NAME}")
endif()

install(TARGETS ${MODULE_NAME} ${MODULE_NAME}_headers
    EXPORT llvm_builderTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING
        PATTERN "*/*.hpp"
        PATTERN "*/*.h")

# Generate and install CMake config files for find_package() support
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/llvm_builderConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/llvm_builderConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/llvm_builder
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/llvm_builderConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(EXPORT llvm_builderTargets
    FILE llvm_builderTargets.cmake
    NAMESPACE llvm_builder::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/llvm_builder
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/llvm_builderConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/llvm_builderConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/llvm_builder
)

function(setup_test target src_files)
    add_executable(${target} ${src_files} ${ARGN})
    target_link_libraries(${target} PRIVATE GTest::gtest GTest::gtest_main ${MODULE_NAME})
    target_include_directories(${target} PRIVATE include)
    add_test(${target}_test ${target})
endfunction()

enable_testing()
find_package(GTest REQUIRED)
file(GLOB_RECURSE ${MODULE_NAME}_test_sources CONFIGURE_DEPENDS "src/unit_tests/*.x.cpp")
message(VERBOSE "List of tests: ${${MODULE_NAME}_test_sources}")
setup_test(unit_test_${MODULE_NAME} "${${MODULE_NAME}_test_sources}")
target_link_libraries(unit_test_${MODULE_NAME} PUBLIC ${llvm_libs})

function(setup_llvm_exec target src_file)
    add_executable(${target} ${src_file})
    target_compile_options(${target} PRIVATE ${LLVM_BUILDER_CXX_FLAGS})
    target_link_libraries(${target} PRIVATE ${MODULE_NAME})
    target_link_libraries(${target} PUBLIC ${llvm_libs})
    target_include_directories(${target} PRIVATE include)
endfunction()

setup_llvm_exec(dummy "src/apps/dummy.x.cpp")
setup_llvm_exec(dummy2 "src/apps/dummy2.x.cpp")
setup_llvm_exec(raw_llvm "src/apps/raw_llvm.x.cpp")
setup_llvm_exec(raw_llvm2 "src/apps/raw_llvm2.x.cpp")

# Option for bundled/standalone builds (no external LLVM dependency at runtime)
option(LLVM_BUILDER_BUNDLE_LLVM "Bundle LLVM libraries for standalone distribution" OFF)

# Python bindings using nanobind
option(BUILD_PYTHON_BINDINGS "Build Python bindings" ON)
if(BUILD_PYTHON_BINDINGS)
    find_package(Python 3.8 COMPONENTS Interpreter Development.Module REQUIRED)

    # Try to find nanobind
    execute_process(
        COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
        OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE nanobind_ROOT
        RESULT_VARIABLE nanobind_RESULT
    )

    if(nanobind_RESULT EQUAL 0)
        list(APPEND CMAKE_PREFIX_PATH "${nanobind_ROOT}")
        find_package(nanobind CONFIG REQUIRED)

        # Create the Python module - don't use NB_STATIC to avoid inheriting global flags
        nanobind_add_module(
            llvm_builder_py
            STABLE_ABI
            python/llvm_builder_py.cpp
        )

        # Link against the main library and LLVM (use shared LLVM for Python module)
        # Find shared LLVM library
        find_library(LLVM_SHARED_LIB NAMES LLVM LLVM-21 LLVM-20 PATHS /usr/lib /usr/local/lib)
        if(LLVM_SHARED_LIB)
            message(STATUS "Using shared LLVM library: ${LLVM_SHARED_LIB}")
            target_link_libraries(llvm_builder_py PRIVATE ${MODULE_NAME} ${LLVM_SHARED_LIB})
        else()
            message(WARNING "Shared LLVM library not found, using static (may fail)")
            target_link_libraries(llvm_builder_py PRIVATE ${MODULE_NAME} ${llvm_libs})
        endif()
        target_include_directories(llvm_builder_py PRIVATE include)

        # Set RPATH for bundled builds - look for libs relative to the module
        if(LLVM_BUILDER_BUNDLE_LLVM)
            set_target_properties(llvm_builder_py PROPERTIES
                INSTALL_RPATH "$ORIGIN/.libs"
                BUILD_WITH_INSTALL_RPATH ON
            )
        endif()

        # Install the Python module
        install(TARGETS llvm_builder_py LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages)

        message(STATUS "Python bindings enabled (nanobind found at ${nanobind_ROOT})")
    else()
        message(WARNING "nanobind not found. Install with: pip install nanobind")
        message(WARNING "Python bindings will not be built")
    endif()
endif()

# Bundled distribution target - copies LLVM libs for standalone package
if(LLVM_BUILDER_BUNDLE_LLVM AND LLVM_SHARED_LIB)
    # Get all LLVM shared library dependencies
    get_filename_component(LLVM_LIB_DIR "${LLVM_SHARED_LIB}" DIRECTORY)

    # Create a custom target to bundle LLVM libraries
    add_custom_target(bundle_llvm_libs
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/.libs"
        COMMAND ${CMAKE_COMMAND} -E copy "${LLVM_SHARED_LIB}" "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/.libs/"
        COMMENT "Bundling LLVM shared libraries for standalone distribution"
    )
endif()
