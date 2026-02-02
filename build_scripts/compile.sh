#!/usr/bin/env bash

set -euxo pipefail

SCRIPT_DIR=$(dirname $(dirname ${BASH_SOURCE[0]}))

export BUILD_TYPE=Debug
export BUILD_TYPE=RelWithDebInfo

pushd $SCRIPT_DIR
mkdir -p build
pushd build

# export BASE_LLVM_DIR="/home/vibhanshu/datadisk/git_src/llvm-project/build/bin"
# export CC="$BASE_LLVM_DIR/clang"
# export CXX="$BASE_LLVM_DIR/clang++"

cmake -GNinja -DCMAKE_BUILD_TYPE=${BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=../artifacts/install_${BUILD_TYPE} ..

ninja

ninja install

popd
popd
