#!/usr/bin/env bash
# Local-only code graph. No compiler setup or background file watcher.
set -euo pipefail
task_graph_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$task_graph_root"
export CBM_CACHE_DIR="$task_graph_root/.dev/codegraph-cache"
task_graph_bin="${WII64_CODE_GRAPH:-$task_graph_root/.dev/tools/codegraph-v0.11.0/codebase-memory-mcp}"
if [[ ! -x "$task_graph_bin" && -x "$task_graph_bin.exe" ]]; then task_graph_bin+='.exe'; fi
[[ -x "$task_graph_bin" ]] || {
    echo 'Install the pinned graph binary: see doc/cpu-development-plan.md.' >&2
    exit 2
}
if [[ $# -eq 0 ]]; then set -- --tool-profile=analysis; fi
exec "$task_graph_bin" --ui=false "$@"
