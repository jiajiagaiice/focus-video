---
name: superpowers
description: Focus Video development workflow for product-quality changes. Use when working in this repository on player architecture, real-time video super-resolution, Windows/NVIDIA GPU performance, C++/CMake code, documentation, tests, pull requests, or when the user mentions superpowers.
---

# Superpowers

## Core workflow

1. Inspect repository instructions and current state before editing: `find .. -name AGENTS.md -print`, `git status --short --branch`, and targeted `rg --files` queries.
2. Convert the user request into a small product decision: define the target user experience, fallback behavior, and validation signal before writing code.
3. Keep playback stability above image quality. Any real-time super-resolution feature must be optional, measurable, and able to degrade without breaking audio/video sync.
4. Prefer narrow, testable C++20/CMake changes. Pair policy changes with unit tests and documentation updates.
5. Update `CHANGELOG.md` for every code, documentation, workflow, or testing change.
6. Validate with build and tests. If a web UI or visible app changes, capture a screenshot.
7. Commit on the current branch and create a PR with a concise title and body.

## Focus Video technical defaults

- Target first-version platform: Windows 10/11 x64 desktop.
- Baseline GPU: NVIDIA GeForce RTX 3060 Ti with 8 GB VRAM.
- Preferred media path: FFmpeg demux, NVDEC decode, GPU frame pool, TensorRT FP16 inference, D3D11 rendering.
- First-version super-resolution goal: low-latency 2x enhancement for 720p/1080p sources, with 2K/4K output targets selected by runtime budget.
- Do not assume 1080p60 -> 4K60 full-quality SR is guaranteed on 3060 Ti. Use Performance mode or frame skipping when needed.

## Change checklist

- Update docs whenever behavior, supported targets, or product promises change.
- Update `CHANGELOG.md` in the same patch for every change.
- Add or update tests for every policy branch that affects GPU capability, resolution, frame rate, output target, or fallback mode.
- Keep CLI prototypes aligned with core types so product assumptions can be inspected from terminal output.
- Avoid adding heavyweight dependencies until the relevant integration milestone needs them.
