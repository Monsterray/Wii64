#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source_file="$repo_root/scripts/wii_video.swift"
binary="$repo_root/.dev/tools/wii-video"
module_cache="$repo_root/.dev/build_tmp/swift-module-cache"

mkdir -p "$(dirname "$binary")" "$module_cache"
if [[ ! -x "$binary" || "$source_file" -nt "$binary" ]]; then
	swiftc -module-cache-path "$module_cache" -framework AVFoundation -framework AppKit \
		-framework CoreImage -o "$binary" "$source_file"
fi
exec "$binary" "$@"
