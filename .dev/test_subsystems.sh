#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/env.sh"  # toolchain paths; python3 on Windows
cd "$(dirname "${BASH_SOURCE[0]}")/.."
task_sub_tmp="$(mktemp -d "${TMPDIR:-/tmp}/wii64-subsystems.XXXXXX")"
trap 'rm -f "$task_sub_tmp/control" "$task_sub_tmp/probes" "$task_sub_tmp/interval" "$task_sub_tmp/no-self" "$task_sub_tmp/scope" "$task_sub_tmp/timer.o" "$task_sub_tmp/hprof" "$task_sub_tmp/memory"; rmdir "$task_sub_tmp"' EXIT
"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined \
    -fno-sanitize-recover=all -DPERF_PROF -DPERF_MEMORY -Itests/memory_stubs -Itests/perf_stubs \
    tests/perf_memory_test.c main/perf_memory.c -o "$task_sub_tmp/memory"
"$task_sub_tmp/memory"
python3 tests/memory_report_test.py
python3 tests/exception_probe_test.py
"${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined \
    -fno-sanitize-recover=all tests/hprof_test.c -o "$task_sub_tmp/hprof"
"$task_sub_tmp/hprof"
python3 tests/hprof_view_test.py
python3 tests/hprof_workflow_test.py
for mode in control probes interval no-self; do
    flags=(-UPERF_SUBSYSTEM_PROBES)
    if [ "$mode" != control ]; then flags=(-DPERF_PROF -DPERF_SUBSYSTEM_PROBES); fi
    if [ "$mode" = interval ]; then flags+=(-DPERF_SUBSYSTEM_INTERVAL=17); fi
    if [ "$mode" = no-self ]; then flags+=(-DPERF_SUBSYSTEM_SELF=0); fi
    "${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined \
        -fno-sanitize-recover=all -Itests/perf_stubs "${flags[@]}" \
        tests/perf_subsystem_test.c main/perf_subsystem.c -o "$task_sub_tmp/$mode"
    "$task_sub_tmp/$mode"
    "${CC:-cc}" -std=c11 -O2 -Wall -Wextra -Werror -Itests/perf_stubs "${flags[@]}" \
        -c main/perf_subsystem.c -o "$task_sub_tmp/timer.o"
    "${CXX:-c++}" -std=c++11 -O2 -Wall -Wextra -Werror -fsanitize=address,undefined \
        -fno-sanitize-recover=all "${flags[@]}" tests/perf_scope_test.cpp "$task_sub_tmp/timer.o" \
        -o "$task_sub_tmp/scope"
    "$task_sub_tmp/scope"
done
python3 tests/hardware_queue_test.py
python3 tests/build_agent_test.py
python3 tests/subsystem_report_test.py
python3 tests/subsystem_workflow_test.py
python3 tests/wii_input_staging_test.py
python3 tests/invalidation_range_test.py
python3 tests/chain_validation_test.py
python3 tests/baseline_provenance_test.py
python3 tests/mario_kart_check_test.py
python3 tests/zelda_check_test.py
python3 tests/library_report_test.py
python3 tests/library_workflow_test.py
python3 tests/graphics_reset_test.py
python3 tests/yuyv_convert_test.py
bash .dev/test_texture_hash.sh
