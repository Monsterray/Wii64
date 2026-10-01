#!/usr/bin/env bash
# Bit-identical constant-length XXH32 dispatch versus the generic wrapper.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
export WII64_SURVEY_SAME_PROBES=1
export WII64_SURVEY_FULL_FLAGS='-DPERF_PROF -DPERF_SUBSYSTEM_PROBES -DGLN64_FIXED_HASH_LENGTHS=1'
export WII64_SURVEY_CONTROL_FLAGS='-DPERF_PROF -DPERF_SUBSYSTEM_PROBES -DGLN64_FIXED_HASH_LENGTHS=0'
exec bash .dev/profile_subsystems.sh "${@:-graphics_survey}"
