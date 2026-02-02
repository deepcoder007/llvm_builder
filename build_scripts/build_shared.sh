#!/usr/bin/env bash

set -euo pipefail
# set -euxo pipefail

SCRIPT_DIR=$(dirname ${BASH_SOURCE[0]})

export BUILD_TYPE=Debug

if [[ $# -ne 1 ]]; then
    echo "No library name specified:"
    exit 1
fi

lib_name="$1*.ll"
lib_util_name="$1_util.cpp"
header_name="$1.h"
shared_object="$1.so"
if [[ -f ./$header_name && -f ./$lib_util_name ]]; then
    echo "Process library $1"
    g++ -std=c++20 -fPIC -D_GLIBCXX_DEBUG_PEDANTIC -I../include -c ./$lib_util_name -o ${lib_util_name}.o
    for llvm_file in $lib_name; do
        clang++ -std=c++20 -O3 -fPIC -c ./${llvm_file} -o ${llvm_file}.o
    done
    echo "Building sharedObject ${shared_object}"
    g++ -shared -fPIC -o ${shared_object} -Wl,--whole-archive libcodegen_engine_linkage.a -Wl,--no-whole-archive ${lib_name}.o ${lib_util_name}.o
    rm ${lib_util_name}.o
    rm ${lib_name}.o
else
    echo "Lib impl not found: $lib_name"
    exit 1
fi
