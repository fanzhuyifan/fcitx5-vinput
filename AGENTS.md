# fcitx5-vinput Agent Guide

Guidelines, dual-planning model, and hard constraints for AI coding agents working on `fcitx5-vinput`.

---

## 1. Project Overview & Architecture

`fcitx5-vinput` is a voice input system for Fcitx5 providing local (sherpa-onnx) and cloud ASR, LLM post-processing, and cross-distro packaging.

- `src/addon/`: Fcitx5 input method addon (Qt/C++, hotkey triggers like `F8`, push-to-talk, D-Bus client).
- `src/daemon/`: Core daemon (`vinput-daemon`), handles PipeWire audio recording, sherpa-onnx inference, cloud ASR engines, LLM scene transformations, D-Bus service (`org.fcitx.Vinput`).
- `src/cli/`: Standalone `vinput` CLI for manual recording, profile switching, and status inspection.
- `src/common/`: Shared types, configuration structs (`nlohmann-json`), D-Bus XML interfaces.
- `po/` & `i18n/`: Localization catalogs (Gettext and Qt `.ts` / `.qm`).

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

## 3. Dual-Planning Model for AI Agents

To avoid ambiguity between functional task planning and toolchain validation, agents must distinguish between two distinct planning phases:

| Phase | Concept & Terminology | Timing | Tool & Output | Purpose |
| :--- | :--- | :--- | :--- | :--- |
| **Phase A** | **Task Planning**<br>*(Feature / Bugfix Breakdown)* | **Pre-development**<br>*(Before coding)* | GitHub Issue & Draft PR body (`- [ ]` checklist) | Defines *what* code to write, module boundaries, and task sequencing. |
| **Phase B** | **Quality Gate Pre-check**<br>*(hk --plan)* | **Post-edit**<br>*(Before committing)* | `hk run check --files0-from - --safe --plan` | Previews *which* linters/formatters will run and their side-effects on edited files. |

---

## 4. Standard 5-Step Agent Execution Workflow

All coding agents must strictly operate within this closed-loop lifecycle:

```
+-------------------------------------------------------------+
| 1. Task Planning & Draft PR Initialization                  |
|    gh issue view <id>                                       |
|    git checkout -b <type>/issue-<id>-<desc>                 |
|    gh pr create --draft --body "Closes #<id>\n- [ ] Task..."|
+------------------------------+------------------------------+
                               |
                +--------------v--------------+
                | 2. Single-Item Implementation|
                |    Code ONLY the topmost - [ ]
                +--------------+--------------+
                               |
                +--------------v--------------+
                | 3. Quality Gate & Pre-check |
                |    hk --plan (preview steps)|
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
| 5. Full Validation, Ready & Merge           |         |
|    gh pr checks                             |         |
|    gh pr ready                              |         |
|    gh pr merge --squash --delete-branch     |         |
+---------------------------------------------+         |
                               ^                         |
                               +-------------------------+
```

### Step 1: Task Planning (Issue Analysis & Draft PR Setup)
1. Inspect the issue requirements: `gh issue view <issue_id>`
2. Create a clean feature branch: `git checkout -b <type>/issue-<id>-<short-description>`
3. Create a **Draft PR** containing the structured implementation checklist:
   ```bash
   gh pr create --draft \
     --title "<type>: <description> (#<issue_id>)" \
     --body "Closes #<issue_id>

   ### Implementation Tasks
   - [ ] 1. Define data structures in src/common/
   - [ ] 2. Implement logic in src/daemon/
   - [ ] 3. Update i18n and tests"
   ```

### Step 2: Single-Item Focused Execution
- **Strict rule**: Focus ONLY on the topmost unchecked task (`- [ ]`).
- Do not modify unrelated files or bundle multiple tasks together.

### Step 3: Quality Gate & Pre-check (hk)
- Inspect which checks match modified files:
  ```bash
  {
    git diff --name-only -z
    git diff --cached --name-only -z
    git ls-files --others --exclude-standard -z
  } | hk run check --files0-from - --safe --plan
  ```
- Run safe validation and auto-format:
  ```bash
  hk run check --safe
  hk fix
  python3 scripts/check-i18n.py
  ```
- If local machine hardware permits: `mise run build-debug`. If resource-constrained, rely on CI.

### Step 4: Atomic Commit, Push & Check Off
1. Commit with Conventional Commits format referencing the issue:
   ```bash
   git add <modified_files>
   git commit -m "<type>(<scope>): <concise message> (#<issue_id>)"
   git push origin <branch_name>
   ```
2. Update the Draft PR body to check off the completed item (`- [x]`):
   ```bash
   gh pr edit --body "..."
   ```

### Step 5: Final Validation & Merge
1. Repeat Steps 2-4 until all checklist items are checked (`- [x]`).
2. Verify PR CI checks: `gh pr checks`.
3. (Optional for core changes) Trigger remote matrix dry build: `gh workflow run release.yml && gh run watch`.
4. Mark PR ready and squash-merge: `gh pr ready && gh pr merge --squash --delete-branch`.

---

## 5. Compilation Strategy: Hardware-Adaptive (CI-First on Modest Hardware)

Compilation strategy should adapt to local hardware capabilities:

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

## 6. Agent Hard Constraints (Red Lines)

1. **No Direct Main Commits**: Never push implementation code directly to `main`.
2. **No Multi-Task Lump Commits**: Every commit must map to exactly one `- [ ]` task in the PR checklist.
3. **No Push Without Quality Gate**: Never push code before running `hk run check --safe`.
4. **Transparent Progress Tracking**: When asked for status, agents must report progress based on the PR checklist (e.g., "Completed 2 of 4 tasks; currently implementing task 3").
5. **Hardware-Adaptive Compilation**: On modest hardware, prioritize GitHub Actions CI (`ci.yml` / `release.yml`) over heavy local full builds.
6. **User-Facing Strings**: Must be wrapped in `_("...")` or `ki18n` for gettext localization. Run `mise run check-i18n` to validate po files.
