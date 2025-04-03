#!/usr/bin/env bash

set -e
set -x

BUILD_TYPE="${BUILD_TYPE:-Release}"
BUILD_PROFILE="${BUILD_PROFILE:-linux}"
BUILD_ROOT="${BUILD_ROOT:-"$(readlink -f "$(dirname "$0")/..")"}"
BUILD_TYPE_LC="$(echo "$BUILD_TYPE" | tr '[:upper:]' '[:lower:]')"
BUILD_DIR="${BUILD_DIR:-"cmake-build-${BUILD_TYPE_LC}-${BUILD_PROFILE}"}"

cd "$BUILD_ROOT"

echo -n "$BUILD_VERSION" | grep "^[0-9]\{1,\}\.[0-9]\{1,\}\.[0-9]\{1,\}$"

user_args=(
 -c user.name='Release Script'
 -c user.email=''
 -c safe.directory="${BUILD_ROOT}"
)

orig_branch="$(git branch --show-current)"

test -n "$orig_branch"

sed -i -e "/^project/s/)/ VERSION $BUILD_VERSION)/" CMakeLists.txt
git "${user_args[@]}" commit -a -m "release $BUILD_VERSION"
git "${user_args[@]}" tag -f "$BUILD_VERSION"
git "${user_args[@]}" revert --no-edit -n HEAD
git "${user_args[@]}" commit -a -m "reverting release commit for development"
git "${user_args[@]}" checkout "$BUILD_VERSION"

./ci/build.sh

git "${user_args[@]}" checkout "$orig_branch"
