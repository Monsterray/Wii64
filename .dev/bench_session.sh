# Source from a queue entry point, once per survey/session.
export WII_BENCH_AGENT="${WII_BENCH_AGENT:-wii64-$(python3 -c 'import uuid; print(uuid.uuid4().hex)')}"
export WII64_RECEIVER_TIMEOUT="${WII64_RECEIVER_TIMEOUT:-$(python3 - "${WII64_CHAIN_FILE:-scripts/chains/${1:?chain required}.txt}" <<'PY'
import math, pathlib, re, sys
lines = pathlib.Path(sys.argv[1]).read_text().splitlines()
vis = [int(re.match(r'chain=(\d+)', line)[1]) for line in lines if line.startswith('chain=')]
if not vis:
    sys.exit('Chain has no VI targets')
# PAL is the slowest region. Include each guest watchdog and ROM preparation,
# then leave time for results. This is a safety bound, not a predicted runtime.
print(math.ceil(sum(n / 50 * 3 + 60 + 120 for n in vis) + 300))
PY
)}"
[[ "$WII64_RECEIVER_TIMEOUT" =~ ^[1-9][0-9]*$ && "${WII64_SKIP_BUILD:-0}" =~ ^[01]$ ]] || {
    echo 'Receiver timeout must be positive integer seconds; skip-build must be 0 or 1' >&2
    return 2
}
export WII64_JOB_TIMEOUT="${WII64_JOB_TIMEOUT:-$((WII64_RECEIVER_TIMEOUT + 300 + 900 * (1 - ${WII64_SKIP_BUILD:-0})))}"
python3 - "$WII64_RECEIVER_TIMEOUT" "$WII64_JOB_TIMEOUT" <<'PY'
import sys
try:
    receiver, job = map(int, sys.argv[1:])
except ValueError:
    sys.exit('Wii timeouts must be positive integer seconds')
if receiver <= 0 or job < receiver + 180:
    sys.exit('Job timeout must allow the receiver plus at least 180 seconds for cleanup')
PY
