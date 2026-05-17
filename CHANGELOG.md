# Changelog

All notable changes to Focus Video are documented in this file.

## Unreleased

- Added an optional in-process FFmpeg demux/decode smoke path with `--inspect-video`, `--decode-smoke`, and `--backend-status` CLI commands.
- Added backend status reporting for D3D11, NVDEC, and TensorRT so unavailable GPU acceleration paths are explicit in M2/M3 work.
- Added MinGW cross-compilation, CPack ZIP packaging, and Windows package usage notes for producing a real Windows x64 command-line package.
- Added Ubuntu runtime dependency installation and local playback smoke-test scripts so ffplay playback can be validated in a fresh environment.
- Added runtime capability detection for ffplay, FFmpeg, NVIDIA runtime, and TensorRT tooling.
- Added `--check-runtime` and `--play` CLI paths so local playback can launch through a real ffplay backend when available, while TensorRT SR requests fail loudly until the frame bridge is implemented.
- Documented the current gap between local playback, PPM reference upscaling, and fully real-time TensorRT video super-resolution.
- Added a frame-level CPU reference super-resolution path with P6 PPM read/write support for real offline image enlargement.
- Added CLI support for `--upscale-ppm` exports and tests covering the image upscaler round trip.
- Documented the offline video export path that will connect FFmpeg decode, frame-level SR, and mux/encode output.
- Added a repository changelog requirement so every future code, docs, or workflow change records a short summary here.
- Added dependency-free validation for the repository-local `superpowers` skill to avoid PyYAML-related testing failures.
- Updated the `superpowers` workflow checklist to require changelog updates for every change.

## 0.1.0 - 2026-05-11

- Added the initial C++20/CMake project skeleton with `focus_video_core`, the `focus-video` CLI prototype, and unit tests.
- Added RTX 3060 Ti-oriented super-resolution policy selection for 2K/4K output targets.
- Added architecture, roadmap, README, and repository-local `superpowers` skill documentation.
