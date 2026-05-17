#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${FOCUS_VIDEO_WINDOWS_BUILD_DIR:-$repo_root/build-windows}"
dist_dir="${FOCUS_VIDEO_DIST_DIR:-$repo_root/dist}"
toolchain="$repo_root/cmake/toolchains/mingw64.cmake"

if ! command -v x86_64-w64-mingw32-g++-posix >/dev/null 2>&1; then
  echo "Missing x86_64-w64-mingw32-g++-posix; install g++-mingw-w64-x86-64 on Ubuntu." >&2
  exit 1
fi

cmake -S "$repo_root" -B "$build_dir" \
  -DCMAKE_TOOLCHAIN_FILE="$toolchain" \
  -DCMAKE_BUILD_TYPE=Release \
  -DFOCUS_VIDEO_ENABLE_FFMPEG=OFF \
  -DBUILD_TESTING=OFF
cmake --build "$build_dir" --config Release
cmake --build "$build_dir" --target package --config Release

mkdir -p "$dist_dir"
cp "$build_dir"/FocusVideo-*-win64.zip "$dist_dir"/

echo "Windows package written to:" "$dist_dir"/FocusVideo-*-win64.zip
