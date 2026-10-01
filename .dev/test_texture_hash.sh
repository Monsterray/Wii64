#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
task_hash_tmp="$(mktemp -d "${TMPDIR:-/tmp}/wii64-texture-hash.XXXXXX")"
trap 'rm -f "$task_hash_tmp/test"; rmdir "$task_hash_tmp"' EXIT
"${CXX:-c++}" -std=c++11 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined \
    -fno-sanitize-recover=all -Itests/audio_stubs tests/texture_hash_test.cpp \
    -o "$task_hash_tmp/test"
"$task_hash_tmp/test"
