#!/usr/bin/env bash
# Same probes, same source: hash memo candidate/reference/candidate.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
export WII64_SURVEY_SAME_PROBES=1
export WII64_SURVEY_FULL_FLAGS='-DPERF_PROF -DPERF_SUBSYSTEM_PROBES -DGLN64_TMEM_HASH_CACHE=1'
export WII64_SURVEY_CONTROL_FLAGS='-DPERF_PROF -DPERF_SUBSYSTEM_PROBES -DGLN64_TMEM_HASH_CACHE=0'
exec bash .dev/profile_subsystems.sh "${@:-graphics_survey}"
