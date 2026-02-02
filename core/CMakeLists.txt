cmake_minimum_required(VERSION 3.16)

set(MODULE_NAME "${PROJECT_NAME}_core")

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

file(GLOB_RECURSE ${MODULE_NAME}_shared_sources CONFIGURE_DEPENDS "src/*/*.shared.x.cpp")
message(VERBOSE "List of shared sources files: ${${MODULE_NAME}_shared_sources}")

add_library(${MODULE_NAME}_headers INTERFACE)
target_include_directories(
    ${MODULE_NAME}_headers INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

add_library(${MODULE_NAME} STATIC ${${MODULE_NAME}_sources} "${${MODULE_NAME}_shared_sources}")
target_link_libraries(${MODULE_NAME} PUBLIC ${MODULE_NAME}_headers)

# Precompiled headers for LLVM (significantly speeds up compilation)
option(USE_PCH "Use precompiled headers for LLVM" ON)
if(USE_PCH)
    target_precompile_headers(${MODULE_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/llvm/pch.h
    )
    message(STATUS "PCH enabled for ${MODULE_NAME}")
endif()

if (${TIMING_STATS})
    add_definitions(-DCORE_PERF)
    target_link_libraries(${MODULE_NAME} PRIVATE -lpfm)
endif()

install(TARGETS ${MODULE_NAME} DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING
        PATTERN "*/*.hpp"
        PATTERN "*/*.h")


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
    target_link_libraries(${target} PRIVATE ${MODULE_NAME})
    target_link_libraries(${target} PUBLIC ${llvm_libs})
    target_include_directories(${target} PRIVATE include)
endfunction()

setup_llvm_exec(dummy "src/apps/dummy.x.cpp")
setup_llvm_exec(dummy2 "src/apps/dummy2.x.cpp")
setup_llvm_exec(raw_llvm "src/apps/raw_llvm.x.cpp")
