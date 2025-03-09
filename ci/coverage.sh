#!/usr/bin/env bash

set -e

ROOT_DIR="$(readlink -f "$(dirname "$0")/..")"

cd "$ROOT_DIR" && gcovr \
  --cobertura coverage.xml \
  --exclude-throw-branches \
  --exclude-unreachable-branches \
  --exclude-directories test \
  --exclude-directories CMakeFiles \
  --fail-under-line 85 \
  --fail-under-branch 70 \
  .
