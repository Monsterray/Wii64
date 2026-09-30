#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
task_sub_tmp="$(mktemp -d "${TMPDIR:-/tmp}/wii64-subsystems.XXXXXX")"
trap 'rm -f "$task_sub_tmp/control" "$task_sub_tmp/probes" "$task_sub_tmp/interval"; rmdir "$task_sub_tmp"' EXIT
for mode in control probes interval; do
    flags=(-UPERF_SUBSYSTEM_PROBES)
    if [ "$mode" != control ]; then flags=(-DPERF_PROF -DPERF_SUBSYSTEM_PROBES); fi
    if [ "$mode" = interval ]; then flags+=(-DPERF_SUBSYSTEM_INTERVAL=17); fi
    "${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined \
        -fno-sanitize-recover=all -Itests/perf_stubs "${flags[@]}" \
        tests/perf_subsystem_test.c main/perf_subsystem.c -o "$task_sub_tmp/$mode"
    "$task_sub_tmp/$mode"
done
python3 tests/hardware_queue_test.py
python3 tests/subsystem_report_test.py
python3 tests/subsystem_workflow_test.py
