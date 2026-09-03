# Claude Code Guide

See [AGENTS.md](AGENTS.md) for full architecture, compilation strategies, Issue+PR workflow, and code quality guidelines.

## Quick CLI Reference

- **Compilation (Hardware-Adaptive)**:
  - Modest Hardware (CI-First): `gh workflow run ci.yml && gh run watch`
  - Pre-release Matrix Dry Build: `gh workflow run release.yml && gh run watch`
  - Local Incremental (Powerful hardware only): `mise run dev` -> `mise run build-debug`
- **Issue + PR Workflow**:
  - `git checkout -b <type>/issue-<id>-<desc>`
  - `gh pr create --draft --body "Closes #<id>\n\n### Tasks\n- [ ] 1. ..."`
  - Loop: Code 1 item -> `hk run check --safe` -> atomic commit & push -> `gh pr edit --body` (- [x])
  - Finish: `gh pr ready && gh pr merge --squash --delete-branch`
- **Code Quality (`hk`)**:
  - Check changed files:
    ```bash
    {
      git diff --name-only -z
      git diff --cached --name-only -z
      git ls-files --others --exclude-standard -z
    } | hk run check --files0-from - --safe
    ```
  - Auto-fix code format: `hk fix`
  - Full repo verification: `hk check`

## Ecosystem Repositories
- **Core Engine (C++20)**: [xifan2333/fcitx5-vinput](https://github.com/xifan2333/fcitx5-vinput) (`~/Code/fcitx5-vinput`)
- **Cloud ASR / LLM Scenes Registry**: [xifan2333/vinput-registry](https://github.com/xifan2333/vinput-registry) (`~/Code/vinput-registry`)
- **Arch AUR Packaging Automation**: [xifan2333/aur-auto](https://github.com/xifan2333/aur-auto) (`~/Code/aur-auto`)
- **Flatpak OSTree Repository Automation**: [xifan2333/flatpak-auto](https://github.com/xifan2333/flatpak-auto) (`~/Code/flatpak-auto`)

## Unified Project Skill
- **`vinput-dev`** (`.agents/skills/vinput-dev/SKILL.md`): Architecture, Fork contribution, pre-PR code health-check, Issue+PR workflow, PipeWire debugging, ecosystem extension, release packaging.
