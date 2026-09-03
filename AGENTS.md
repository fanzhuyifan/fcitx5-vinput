# fcitx5-vinput Agent Guide

Guidelines and baseline rules for AI coding agents working on `fcitx5-vinput`.

---

## 1. Project Overview & Architecture

`fcitx5-vinput` is a voice input system for Fcitx5 providing local (sherpa-onnx) and cloud ASR, LLM post-processing, and cross-distro packaging.

- `src/addon/`: Fcitx5 input method addon (Qt/C++, hotkey triggers like `F8`, push-to-talk, D-Bus client).
- `src/daemon/`: Core daemon (`vinput-daemon`), handles PipeWire audio recording, sherpa-onnx inference, cloud ASR engines, LLM scene transformations, D-Bus service (`org.fcitx.Vinput`).
- `src/cli/`: Standalone `vinput` CLI for manual recording, profile switching, and status inspection.
- `src/common/`: Shared types, configuration structs (`nlohmann-json`), D-Bus XML interfaces.
- `po/` & `i18n/`: Translations (Gettext and Qt `.ts` / `.qm`).

---

## 2. Related Repositories & Ecosystem

`fcitx5-vinput` is part of a multi-repository ecosystem:

| Repository | GitHub URL | Local Path | Role & Purpose |
| :--- | :--- | :--- | :--- |
| **`fcitx5-vinput`** (Core) | [xifan2333/fcitx5-vinput](https://github.com/xifan2333/fcitx5-vinput) | `.` (`~/Code/fcitx5-vinput`) | Main C++20 repository: Fcitx5 addon, background daemon, CLI, PipeWire capture, local sherpa-onnx runtime, D-Bus service, GitHub releases. |
| **`vinput-registry`** | [xifan2333/vinput-registry](https://github.com/xifan2333/vinput-registry) | `~/Code/vinput-registry` | Resource catalog: index for local ASR models (`models.json`), cloud ASR provider scripts (`providers.json` + `resources/providers/`), and LLM scene adapters (`adapters.json` + `resources/adapters/`). |
| **`aur-auto`** | [xifan2333/aur-auto](https://github.com/xifan2333/aur-auto) | `~/Code/aur-auto` | Arch User Repository (AUR) automation: tracks `fcitx5-vinput` releases via `pkgs/fcitx5-vinput-bin/`, tests in clean chroot, and publishes to AUR. |
| **`flatpak-auto`** | [xifan2333/flatpak-auto](https://github.com/xifan2333/flatpak-auto) | `~/Code/flatpak-auto` | Flatpak repository automation: tracks releases via `products/fcitx5-vinput/`, imports bundles into shared OSTree repo, and publishes `.flatpakref` / `.flatpakrepo` to GitHub Pages. |

- **Adding / Modifying Cloud ASR or LLM Scenes**: Work in `~/Code/vinput-registry`.
- **Packaging / AUR Release Tracking**: Work in `~/Code/aur-auto` (`pkgs/fcitx5-vinput-bin/`).
- **Flatpak Distribution & OSTree Sync**: Work in `~/Code/flatpak-auto` (`products/fcitx5-vinput/`).

For detailed operational guides, fork contribution, code health check SOPs, and cross-repo workflows, refer to the unified skill at `.agents/skills/vinput-dev/SKILL.md`.

---

## 3. Compilation Strategy: Hardware-Adaptive (CI-First on Modest Hardware)

Compilation strategy should adapt to the local machine's hardware capabilities:

- **Modest / Resource-Constrained Local Hardware -> CI-First Strategy**:
  - Do NOT run heavy local full builds, multi-arch cross-compilations, or container builds locally.
  - Rely on **GitHub Actions CI** for building, testing, and matrix validation:
    - Standard CI: `gh workflow run ci.yml && gh run watch`
    - Pre-release full matrix dry run: `gh workflow run release.yml && gh run watch`
    - Packaging channels: `mise run channels`
  - Local operations should stay lightweight: code editing, static formatting/linting via `hk`, and json/i18n validation.
- **High-Performance Hardware with Full Toolchains**:
  - Local incremental debug builds are permitted: `mise run dev` -> `mise run build-debug`.
  - Always run `gh workflow run release.yml` before cutting a release for clean-room multi-distro verification.

---

## 4. Issue + PR Driven Development Workflow (SOP)

All non-trivial changes must strictly follow the **Issue -> Draft PR with Todo-List -> Atomic Commit -> Check off Todo -> Merge** loop:

```
+-------------------------------------------------------------+
| 1. Plan & Draft PR (Issue Breakdown & Draft PR Setup)       |
|    gh issue view <id>                                       |
|    git checkout -b <type>/issue-<id>-<desc>                 |
|    gh pr create --draft --body "Closes #<id>\n- [ ] Task..."|
+------------------------------+------------------------------+
                               |
                +--------------v--------------+
                | 2. Single-Item Dev          |
                |    Only code the first - [ ]|
                +--------------+--------------+
                               |
                +--------------v--------------+
                | 3. Quality Gate             |
                |    hk run check --safe      |
                |    hk fix (if formatting)   |
                +--------------+--------------+
                               |
                +--------------v--------------+
                | 4. Atomic Commit & Push     |
                |    git commit -m "feat:..." |
                |    git push                 |
                |    gh pr edit --body (- [x])|
                +--------------+--------------+
                               | (Remaining tasks?)
                               +---------- Yes ---------+
                               | No                     |
+------------------------------v--------------+         |
| 5. Full Validation & Merge                  |         |
|    gh pr checks                             |         |
|    gh pr ready                              |         |
|    gh pr merge --squash --delete-branch     |         |
+---------------------------------------------+         |
                               ^                         |
                               +-------------------------+
```

### The 5 Steps:
1. **Analyze Issue & Create Draft PR**:
   ```bash
   gh issue view <issue_id>
   git checkout -b feat/issue-<id>-<short-description>
   gh pr create --draft \
     --title "<type>: <description> (#<issue_id>)" \
     --body "Closes #<issue_id>

   ### Implementation Tasks
   - [ ] 1. Define data structures in src/common/
   - [ ] 2. Implement logic in src/daemon/
   - [ ] 3. Update i18n and tests"
   ```
2. **Execute One Item at a Time**: Focus strictly on the topmost unchecked task (`- [ ]`).
3. **Verify via `hk`**: Run `hk run check --safe` (and `hk fix` if needed). If local hardware permits, run `mise run build-debug`.
4. **Atomic Commit, Push & Check Off**:
   - `git add <files> && git commit -m "<type>(<scope>): <desc> (#<id>)" && git push origin <branch>`
   - Update PR body to mark the task completed (`- [x]`) via `gh pr edit --body "..."`.
5. **Loop to Finish & Merge**:
   - Repeat steps 2-4 until all items are checked.
   - Verify PR CI: `gh pr checks`.
   - Mark ready and merge: `gh pr ready && gh pr merge --squash --delete-branch`.

---

## 5. Code Quality & Verification (`hk` CLI)

This repository uses **`hk`** (Git Hook Manager) with `mise` to enforce formatting and validation:

- **Run checks on changed files (Required before committing/pushing)**:
  ```bash
  {
    git diff --name-only -z
    git diff --cached --name-only -z
    git ls-files --others --exclude-standard -z
  } | hk run check --files0-from - --safe
  ```
- **Auto-fix formatting violations**: `hk fix`
- **Full repository check**: `hk check`
- **Static analysis**: `mise run tidy`

---

## 6. Coding & Contribution Rules

1. **C++ Standard**: C++20. Follow existing patterns in the codebase.
2. **User-facing Strings**: Must be wrapped in `_("...")` or `ki18n` for gettext localization. Run `mise run check-i18n` to validate po files.
3. **Agent Discipline**: Never batch all changes into one giant commit without updating the PR Todo list. Keep commits atomic and traceable.
