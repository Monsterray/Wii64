#!/usr/bin/env bash
# Check the real RSP audio code with sanitizers in both DMEM layouts.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
compiler="${CC:-cc}"
link_flag=-Wl,--gc-sections
if [ "$(uname -s)" = Darwin ]; then link_flag=-Wl,-dead_strip; fi
task_audio_tmp="$(mktemp -d "${TMPDIR:-/tmp}/wii64-rsp-audio.XXXXXX")"
trap 'rm -f "$task_audio_tmp/alist_envmix" "$task_audio_tmp/adpcm" "$task_audio_tmp/adpcm_big"; rmdir "$task_audio_tmp"' EXIT
for name in alist_envmix adpcm adpcm_big; do
    source_name="$name"
    layout_flag=-U_BIG_ENDIAN
    if [ "$name" = adpcm_big ]; then source_name=adpcm; layout_flag=-D_BIG_ENDIAN; fi
    "$compiler" -std=c11 -O2 -Wall -Wextra -Werror "$layout_flag" \
        -ffunction-sections -fdata-sections "$link_flag" \
        -fsanitize=address,undefined -fno-sanitize-recover=all \
        "tests/${source_name}_test.c" rsp_hle/alist.c rsp_hle/audio.c rsp_hle/memory.c \
        -o "$task_audio_tmp/$name"
    "$task_audio_tmp/$name"
done
