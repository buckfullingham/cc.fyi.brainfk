#!/usr/bin/env bash

set -e

ROOT_DIR="$(readlink -f "$(dirname "$0")/..")"

cd "$ROOT_DIR"

BUILD_TYPE=Debug BUILD_COVERAGE=1 BUILD_DIR=cmake-build-coverage-"${BUILD_PROFILE}" ci/build.sh

gcovr \
  --cobertura coverage.xml \
  --exclude-throw-branches \
  --exclude-unreachable-branches \
  --exclude-directories test \
  --exclude-directories CMakeFiles \
  --fail-under-line 85 \
  --fail-under-branch 70 \
  .
