#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/../.." && pwd)"
TEST_BUILD_DIR="$(mktemp -d "${TMPDIR:-/tmp}/openxray-layout-tests.XXXXXX")"
trap 'rm -rf "${TEST_BUILD_DIR}"' EXIT

"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror \
    "${ROOT_DIR}/src/xrEngine/macos/GameDataLayout.cpp" \
    "${ROOT_DIR}/src/xrEngine/macos/tests/GameDataLayoutTests.cpp" \
    -o "${TEST_BUILD_DIR}/game_data_layout_tests"

"${TEST_BUILD_DIR}/game_data_layout_tests"
