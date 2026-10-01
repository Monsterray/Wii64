#!/usr/bin/env bash
# Frozen paging/reference/paging runs; same probes and agent in both.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
bash .dev/test_rom_vm.sh
bash .dev/build_agent.sh
export WII64_SURVEY_FULL_FLAGS='-DPERF_PROF -DPERF_SUBSYSTEM_PROBES -DVM_PAGE_READAHEAD=1 -DVM_ROM_PREFLUSH=1'
export WII64_SURVEY_CONTROL_FLAGS='-DPERF_PROF -DPERF_SUBSYSTEM_PROBES -DVM_PAGE_READAHEAD=0 -DVM_ROM_PREFLUSH=0'
export WII64_SURVEY_SAME_PROBES=1
exec bash .dev/profile_subsystems.sh "${1:-rom_paging}" HBC_AGENT=1 \
    "HBC_AGENT_ROOT=${WII64_HBC_ROOT:-$PWD/../hbc-reborn}"
