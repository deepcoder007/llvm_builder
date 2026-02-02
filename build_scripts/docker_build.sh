#!/bin/bash

set -euo pipefail

SCRIPT_DIR=$(dirname ${BASH_SOURCE[0]})
docker_dir="$SCRIPT_DIR/../docker/"

pushd $docker_dir
docker build . -t codegen_engine
