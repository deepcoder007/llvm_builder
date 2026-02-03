# Standalone CMakeLists.txt for Python bindings
# Can be used directly if building only the Python module

cmake_minimum_required(VERSION 3.16)
project(llvm_builder_py LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Python
find_package(Python 3.8 COMPONENTS Interpreter Development.Module REQUIRED)

# Try to find nanobind via pip, or use FetchContent as fallback
execute_process(
    COMMAND "${Python_EXECUTABLE}" -m nanobind --cmake_dir
    OUTPUT_STRIP_TRAILING_WHITESPACE OUTPUT_VARIABLE nanobind_ROOT
    RESULT_VARIABLE nanobind_RESULT
)

if(nanobind_RESULT EQUAL 0)
    list(APPEND CMAKE_PREFIX_PATH "${nanobind_ROOT}")
    find_package(nanobind CONFIG REQUIRED)
    message(STATUS "Found nanobind via pip: ${nanobind_ROOT}")
else()
    # Fallback: use FetchContent to download nanobind
    message(STATUS "nanobind not found via pip, fetching from GitHub...")
    include(FetchContent)
    FetchContent_Declare(
        nanobind
        GIT_REPOSITORY https://github.com/wjakob/nanobind.git
        GIT_TAG v2.2.0
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(nanobind)
    find_package(nanobind CONFIG REQUIRED)
endif()

# Find LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
include_directories(${LLVM_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

llvm_map_components_to_libnames(llvm_libs
    Core
    CodeGen
    ExecutionEngine
    IRReader
    native
    OrcJIT
    OrcDebugging
)

# Find the parent llvm_builder library
if(NOT TARGET llvm_builder)
    # Build as standalone - need to compile the library
    set(LLVM_BUILDER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
    include_directories(${LLVM_BUILDER_ROOT}/include)

    file(GLOB_RECURSE LLVM_BUILDER_SOURCES CONFIGURE_DEPENDS "${LLVM_BUILDER_ROOT}/src/*/*.cpp")
    list(FILTER LLVM_BUILDER_SOURCES EXCLUDE REGEX "\\.[xt]\\.cpp$")

    add_library(llvm_builder_static STATIC ${LLVM_BUILDER_SOURCES})
    target_include_directories(llvm_builder_static PUBLIC
        ${LLVM_BUILDER_ROOT}/include
        ${LLVM_BUILDER_ROOT}/src
    )
    target_compile_options(llvm_builder_static PRIVATE -fno-exceptions -fno-rtti)
    set(LLVM_BUILDER_LIB llvm_builder_static)
else()
    set(LLVM_BUILDER_LIB llvm_builder)
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../include)
endif()

# Create the Python module
nanobind_add_module(
    llvm_builder_py
    STABLE_ABI
    NB_STATIC
    llvm_builder_py.cpp
)

# Link against the llvm_builder library and LLVM
target_link_libraries(llvm_builder_py PRIVATE ${LLVM_BUILDER_LIB} ${llvm_libs})
target_include_directories(llvm_builder_py PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

# nanobind requires RTTI and exceptions - override the library's flags
target_compile_options(llvm_builder_py PRIVATE -frtti -fexceptions)

# Install
install(TARGETS llvm_builder_py
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages
)
install(FILES __init__.py llvm_builder_py.pyi
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/python${Python_VERSION_MAJOR}.${Python_VERSION_MINOR}/site-packages/llvm_builder
)
