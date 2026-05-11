#!/usr/bin/env python3
"""Validate the repository-local superpowers skill without external dependencies."""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SKILL = ROOT / ".codex" / "skills" / "superpowers" / "SKILL.md"
AGENT = ROOT / ".codex" / "skills" / "superpowers" / "agents" / "openai.yaml"


def fail(message: str) -> None:
    print(f"skill validation failed: {message}", file=sys.stderr)
    raise SystemExit(1)


def frontmatter(markdown: str) -> dict[str, str]:
    if not markdown.startswith("---\n"):
        fail("SKILL.md must start with YAML frontmatter")

    try:
        _, raw_frontmatter, body = markdown.split("---", 2)
    except ValueError as exc:
        fail(f"SKILL.md frontmatter is not closed: {exc}")

    values: dict[str, str] = {}
    for line in raw_frontmatter.strip().splitlines():
        if ":" not in line:
            fail(f"invalid frontmatter line: {line}")
        key, value = line.split(":", 1)
        values[key.strip()] = value.strip().strip('"')

    if not body.strip():
        fail("SKILL.md body must not be empty")
    return values


def require_text(path: Path) -> str:
    if not path.is_file():
        fail(f"missing required file: {path.relative_to(ROOT)}")
    return path.read_text(encoding="utf-8")


def main() -> int:
    skill_text = require_text(SKILL)
    metadata = frontmatter(skill_text)

    if metadata.get("name") != "superpowers":
        fail("SKILL.md name must be 'superpowers'")
    if not metadata.get("description"):
        fail("SKILL.md description is required")
    if "TODO" in skill_text:
        fail("SKILL.md must not contain TODO placeholders")

    agent_text = require_text(AGENT)
    required_agent_keys = ["display_name", "short_description", "default_prompt"]
    for key in required_agent_keys:
        if f"{key}:" not in agent_text:
            fail(f"agents/openai.yaml missing interface key: {key}")

    print("superpowers skill validation passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
