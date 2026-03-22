#!/bin/bash

set -euo pipefail

ulimit -s unlimited
APP_DATA_DIR=$(dirname "$(readlink -f "$0")")
BUILD_DIR="$APP_DATA_DIR/build"

build() {
    local CONFIG="RelWithDebInfo"

    conan install . --build=missing --output-folder=$BUILD_DIR --settings=build_type=$CONFIG
    local CONAN_CMAKE_TOOLCHAIN=$BUILD_DIR/build/$CONFIG/generators/conan_toolchain.cmake

    pushd $BUILD_DIR > /dev/null
    cmake .. -GNinja -DCMAKE_TOOLCHAIN_FILE=$CONAN_CMAKE_TOOLCHAIN -DCMAKE_BUILD_TYPE=$CONFIG
    ninja
    popd > /dev/null
}

# runs unit test
run() {
    local BINARY_DIR="$BUILD_DIR/bin"
    pushd $BINARY_DIR > /dev/null

    for test in *; do
        if [[ $test == *"test_"* ]]; then
            echo "Running $test ..."
            ./$test
        fi
    done

    popd > /dev/null
}

build $@
run
