#!/usr/bin/env bash
# Build both PERF_PROF targets and run the all-ROM chain on each (baseline capture).
set -u
cd "$(dirname "$0")/.."
for t in glN64 Rice; do
	.dev/build.sh glN64_wii clean >/dev/null 2>&1
	.dev/build_profiling.sh ${t}_wii > .dev/b_prof_$t.log 2>&1 || { echo "$t build failed"; exit 1; }
	echo "$t build: $(grep -c 'warning:' .dev/b_prof_$t.log) warnings"
	cp wii64-$t.dol .dev/wii64-$t-prof.dol
	rm -rf .dev/runs/base_$t
	.dev/wii64_diag.sh chain .dev/wii64-$t-prof.dol scripts/chains/all.txt .dev/runs/base_$t 2>&1 | tail -9
done
