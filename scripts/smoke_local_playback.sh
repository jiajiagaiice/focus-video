#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${FOCUS_VIDEO_BUILD_DIR:-$repo_root/build}"
smoke_dir="${FOCUS_VIDEO_SMOKE_DIR:-$(mktemp -d)}"
trap 'rm -rf "$smoke_dir"' EXIT

if ! command -v ffmpeg >/dev/null 2>&1 || ! command -v ffplay >/dev/null 2>&1; then
  echo "ffmpeg/ffplay are required for this smoke test." >&2
  echo "On Ubuntu, run: sudo scripts/install_runtime_deps_ubuntu.sh" >&2
  exit 1
fi

cmake -S "$repo_root" -B "$build_dir"
cmake --build "$build_dir"

sample="$smoke_dir/focus-video-smoke.mp4"
ffmpeg -y -hide_banner \
  -f lavfi -i testsrc=size=160x90:rate=10:duration=1 \
  -pix_fmt yuv420p "$sample" >/dev/null

"$build_dir/focus-video" --check-runtime --play "$sample"

# In CI/headless containers we can still prove the ffplay backend launches and
# decodes the local file by using SDL's dummy video driver. On a desktop, set
# SDL_VIDEODRIVER to your normal display backend or leave it unset.
SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-dummy}" \
  timeout "${FOCUS_VIDEO_SMOKE_TIMEOUT:-10}" \
  "$build_dir/focus-video" --play "$sample"

echo "Local playback smoke test passed: $sample"
