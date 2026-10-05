"""Cold HOME snapshots must not add a per-frame heap walk or release a live lease."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parents[1]
source = (root / 'main/dev_agent.c').read_text()
frame = source[source.index('static void memoryHomeFrame'):source.index('\n#endif', source.index('static void memoryHomeFrame'))]
assert 'if (homeFirstFrame)' in frame
assert frame.index('homeFirstFrame = 0') < frame.index('perfMem_snapshot("home_active")')
overlay = source[source.index('static void openOverlay'):source.index('\nvoid devAgent_pollHome')]
assert overlay.index('GX_DrawDone()') < overlay.index('perfMem_snapshot("home_enter")')
assert overlay.index('hbc_agent_home(vmode)') < overlay.index('perfMem_snapshot("home_closed")')
script = (root / '.dev/test_memory_home.sh').read_text()
assert script.index('WII_BENCH_JOB:-') < script.index('key h')
assert 'trap \'wait "$run_pid" || true\' EXIT' in script
assert 'SYS_POWEROFF' not in script and ' key b' in script
assert 'WII64_CHAIN_FILE="$chain_file" WII64_ROM_DIR=' in script
assert 'cp "$chain_file" "$frozen/chain.txt"' in script
assert 'cp .dev/test_memory_home.sh "$frozen/launcher.sh"' in script
subprocess.run(['bash', '-n', str(root / '.dev/test_memory_home.sh')], check=True)
print('Memory HOME: stopped GX, one active snapshot, close/resume and leased lifetime PASS')
