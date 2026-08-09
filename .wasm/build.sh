#!/usr/bin/env bash
set -euo pipefail
source /opt/emsdk/emsdk_env.sh

SRC_DIR="${1:-/work}"
BUILD_DIR="${2:-/build}"

# Use Qt's WASM toolchain file — it chainloads the Emscripten toolchain,
# auto-detects $EMSDK, and sets up CMAKE_PREFIX_PATH/CMAKE_FIND_ROOT_PATH.
# Using the raw Emscripten toolchain breaks find_package(Qt6) and misses
# _qt_test_emscripten_version (defined in QtPublicWasmToolchainHelpers.cmake).
cmake -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE="$QT_WASM_PATH/lib/cmake/Qt6/qt.toolchain.cmake" \
    -DCMAKE_BUILD_TYPE=Release \
    -S "$SRC_DIR" -B "$BUILD_DIR"

cmake --build "$BUILD_DIR"
