#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
task_rom_tmp="$(mktemp -d "${TMPDIR:-/tmp}/wii64-rom-vm.XXXXXX")"
trap 'rm -f "$task_rom_tmp/loader" "$task_rom_tmp/layout" "$task_rom_tmp/pagefile"; rmdir "$task_rom_tmp"' EXIT
for flush in 0 1; do
    "${CC:-cc}" -std=c11 -O1 -Wall -Wextra -Werror -Wno-deprecated-declarations -fsanitize=address,undefined \
        -fno-sanitize-recover=all -DVM_ROM_PREFLUSH="$flush" -Itests/rom_stubs -Itests/audio_stubs \
        tests/rom_cache_test.c -o "$task_rom_tmp/loader"
    "$task_rom_tmp/loader"
done
for agent in 0 1; do
    flags=(-UWII64_HBC_AGENT)
    if [ "$agent" = 1 ]; then flags=(-DWII64_HBC_AGENT); fi
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror "${flags[@]}" tests/mem2_layout_test.c -o "$task_rom_tmp/layout"
    "$task_rom_tmp/layout"
done
python3 tests/hbc_watch_test.py
for ahead in 0 1; do
    "${CC:-cc}" -std=c11 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
        -fno-sanitize-recover=all -DVM_PAGE_READAHEAD="$ahead" -Itests/rom_stubs \
        tests/pagefile_test.c -o "$task_rom_tmp/pagefile"
    "$task_rom_tmp/pagefile"
done
