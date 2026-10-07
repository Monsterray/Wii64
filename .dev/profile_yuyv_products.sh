#!/usr/bin/env bash
# Exact coefficient lookup candidate versus integer multiplication.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
export WII64_SURVEY_SAME_PROBES=1
export WII64_SURVEY_FULL_FLAGS='-DPERF_PROF -DPERF_SUBSYSTEM_PROBES -DGLN64_YUYV_PRODUCTS=1'
export WII64_SURVEY_CONTROL_FLAGS='-DPERF_PROF -DPERF_SUBSYSTEM_PROBES -DGLN64_YUYV_PRODUCTS=0'
exec bash .dev/profile_subsystems.sh "${@:-next_hotpaths}"
