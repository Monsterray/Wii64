#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"  # toolchain paths; python3 on Windows
cd "$(dirname "${BASH_SOURCE[0]}")/.."
task_rom_tmp="$(mktemp -d "${TMPDIR:-/tmp}/wii64-rom-vm.XXXXXX")"
trap 'rm -f "$task_rom_tmp/loader" "$task_rom_tmp/layout" "$task_rom_tmp/pagefile"; rmdir "$task_rom_tmp"' EXIT
for flush in default 0 1; do
    flags=(-UVM_ROM_PREFLUSH)
    if [ "$flush" != default ]; then flags=(-DVM_ROM_PREFLUSH="$flush"); fi
    "${CC:-cc}" -std=c11 -O1 -Wall -Wextra -Werror -Wno-deprecated-declarations -fsanitize=address,undefined \
        -fno-sanitize-recover=all "${flags[@]}" -Itests/rom_stubs -Itests/audio_stubs \
        tests/rom_cache_test.c -o "$task_rom_tmp/loader"
    "$task_rom_tmp/loader"
done
# Rice and glN64 (which adds the depth-copy region), each with and without the agent.
for plugin in "" -DGLN64_GX; do
for agent in 0 1; do
    flags=(-UWII64_HBC_AGENT $plugin)
    if [ "$agent" = 1 ]; then flags=(-DWII64_HBC_AGENT $plugin); fi
    "${CC:-cc}" -std=c11 -Wall -Wextra -Werror "${flags[@]}" tests/mem2_layout_test.c -o "$task_rom_tmp/layout"
    "$task_rom_tmp/layout"
done
done
python3 tests/hbc_watch_test.py
python3 tests/vm_flush_test.py
for ahead in default 0 1 memory; do
    flags=(-UVM_PAGE_READAHEAD)
    if [ "$ahead" = memory ]; then flags=(-DPERF_PROF -DPERF_MEMORY);
    elif [ "$ahead" != default ]; then flags=(-DVM_PAGE_READAHEAD="$ahead"); fi
    "${CC:-cc}" -std=c11 -O1 -Wall -Wextra -Werror -fsanitize=address,undefined \
        -fno-sanitize-recover=all "${flags[@]}" -Itests/rom_stubs \
        tests/pagefile_test.c -o "$task_rom_tmp/pagefile"
    "$task_rom_tmp/pagefile"
done
