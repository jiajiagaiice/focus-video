#!/usr/bin/env bash
set -euo pipefail

if [[ "$(id -u)" -ne 0 ]]; then
  exec sudo "$0" "$@"
fi

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y ffmpeg libavformat-dev libavcodec-dev libavutil-dev libswscale-dev

cat <<'MSG'

Installed FFmpeg/ffplay and FFmpeg development libraries for local playback and in-process demux/decode smoke tests.

TensorRT real-time SR still requires a machine with:
  - NVIDIA RTX GPU and working NVIDIA driver
  - nvidia-smi on PATH
  - TensorRT installed with trtexec on PATH
  - a compatible .engine model file

This script intentionally does not install NVIDIA drivers or TensorRT because those
are host/GPU/driver-version specific and should be installed from NVIDIA packages
for the target Windows/Linux runtime.
MSG
