#!/usr/bin/env bash

set -e

BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_PROFILE="${BUILD_PROFILE:-linux}"
BUILD_ROOT="${BUILD_ROOT:-"$(readlink -f "$(dirname "$0")/..")"}"
BUILD_TYPE_LC="$(echo "$BUILD_TYPE" | tr '[:upper:]' '[:lower:]')"
BUILD_DIR="${BUILD_DIR:-"cmake-build-${BUILD_TYPE_LC}-${BUILD_PROFILE}"}"

python3 --version
pip3 --version
cmake --version

mkdir -p $CONAN_HOME/profiles
mako-render "$BUILD_ROOT/ci/conan.profile.mako" > $CONAN_HOME/profiles/default

CONAN_SETTINGS=(
  -s build_type="$BUILD_TYPE"
  -c tools.cmake.cmake_layout:build_folder_vars="['settings.build_type', 'settings.os']"
)

if [ -n "$BUILD_COVERAGE" ]; then
  CMAKE_SETTINGS=(
    -DCMAKE_C_FLAGS=--coverage
    -DCMAKE_CXX_FLAGS=--coverage
  )
else
  CMAKE_SETTINGS=()
fi

cd "$BUILD_ROOT"
conan install -of "$BUILD_DIR" --build=missing "${CONAN_SETTINGS[@]}" .

source ${BUILD_DIR}/conanbuildenv-${BUILD_TYPE_LC}*.sh

"$CC" --version
"$CXX" --version

# configure build
cd "$BUILD_DIR"
cmake .. \
  -G "Unix Makefiles" \
  -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake \
  -DCMAKE_POLICY_DEFAULT_CMP0091=NEW \
  -DBUILD_PROFILE=$BUILD_PROFILE \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  "${CMAKE_SETTINGS[@]}"

# put dependencies' dll's on LD_LIBRARY_PATH etc
source conanrun.sh

# build
cmake --build . --parallel

# run tests
ctest .

# package
cpack -G TGZ .
