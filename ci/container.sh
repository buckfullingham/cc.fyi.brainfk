#!/usr/bin/env bash

set -e

export BUILD_ROOT="${BUILD_ROOT:-"$(readlink -f "$(dirname "$0")/..")"}"
export BUILD_PROFILE="${BUILD_PROFILE:-linux}"
export BUILD_CONAN_VOLUME=${CONAN_VOLUME:-conan}
export BUILD_ROOT_IN_CONTAINER=/tmp/"$(basename "$BUILD_ROOT")"
export BUILD_IMAGE="${BUILD_IMAGE:-brainfk-"$BUILD_PROFILE"}"

test -d "$BUILD_ROOT"
test -d "$BUILD_ROOT/ci/$BUILD_PROFILE"

apt update -vy || true
apt install -vy docker || true

docker build \
  -t "$BUILD_IMAGE" \
  -f "$BUILD_ROOT/ci/$BUILD_PROFILE/Dockerfile" \
  "$BUILD_ROOT/ci"

# make repeated runs in same docker context faster by reusing conan output
docker volume create "${BUILD_CONAN_VOLUME}" || true

if [ -t 1 ]; then
  EXTRA_ARGS=(-t)
else
  EXTRA_ARGS=()
fi

exec docker run \
  -i \
  -v "${BUILD_ROOT}:${BUILD_ROOT_IN_CONTAINER}" \
  -v "${BUILD_CONAN_VOLUME}:/mnt/conan" \
  -e CONAN_HOME=/mnt/conan \
  -e "BUILD_ROOT=${BUILD_ROOT_IN_CONTAINER}" \
  -e BUILD_TYPE \
  -e BUILD_PROFILE \
  "${EXTRA_ARGS[@]}" \
  "$BUILD_IMAGE" \
  "$@"
