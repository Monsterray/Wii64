#!/usr/bin/env bash
# Reuse the existing same-binary freezer and leased queue/collector.
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
exec bash .dev/profile_pc.sh "${1:-wii64-glN64.dol}" "${2:-memory_census}" memory
