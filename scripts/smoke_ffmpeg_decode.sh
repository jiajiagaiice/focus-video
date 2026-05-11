#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${FOCUS_VIDEO_BUILD_DIR:-$repo_root/build}"
smoke_dir="${FOCUS_VIDEO_SMOKE_DIR:-$(mktemp -d)}"
trap 'rm -rf "$smoke_dir"' EXIT

if ! pkg-config --exists libavformat libavcodec libavutil libswscale; then
  echo "FFmpeg development libraries are required for in-process demux/decode." >&2
  echo "On Ubuntu, run: sudo apt-get install -y libavformat-dev libavcodec-dev libavutil-dev libswscale-dev" >&2
  exit 1
fi
if ! command -v ffmpeg >/dev/null 2>&1; then
  echo "ffmpeg command is required to generate the smoke-test video." >&2
  exit 1
fi

cmake -S "$repo_root" -B "$build_dir" -DFOCUS_VIDEO_ENABLE_FFMPEG=ON
cmake --build "$build_dir"

sample="$smoke_dir/focus-video-decode-smoke.mp4"
ffmpeg -y -hide_banner \
  -f lavfi -i testsrc=size=160x90:rate=10:duration=1 \
  -pix_fmt yuv420p "$sample" >/dev/null

"$build_dir/focus-video" --inspect-video "$sample"
"$build_dir/focus-video" --decode-smoke "$sample" --max-frames 5
"$build_dir/focus-video" --backend-status

echo "In-process FFmpeg demux/decode smoke test passed: $sample"
