# Issue + PR Driven Development Workflow (SOP)

Development must follow an iterative, atomic **Issue -> Draft PR -> Single-Item Loop -> Merge** process.

---

## 1. Dual-Planning Model

To avoid confusing functional planning with toolchain validation:

1. **Phase A: Task Planning (Pre-development)**:
   - Defining *what* to build: Issue analysis, module boundaries, and creating the Draft PR `- [ ]` checklist.
2. **Phase B: Quality Gate Pre-check (Post-edit)**:
   - Previewing *which* linter steps will execute on edited files via `hk --plan`.

---

## 2. The 5-Step Lifecycle

```
+------------------------------------------------------------------------+
| 1. Identify Requirement -> Create Branch -> Open Draft PR              |
|    gh issue view <id>                                                  |
|    git checkout -b <type>/issue-<id>-<name>                            |
|    gh pr create --draft --body "Closes #<id>\n\n### Tasks\n- [ ] ..."  |
+-----------------------------------+------------------------------------+
                                    |
                +-------------------v-------------------+
                | 2. Single-Item Focused Development    |
                |    Only implement the first - [ ]     |
                +-------------------+-------------------+
                                    |
                +-------------------v-------------------+
                | 3. Local Quality Gate & Pre-check     |
                |    hk --plan (preview steps)          |
                |    hk run check --safe                |
                |    hk fix (if formatting needed)      |
                +-------------------+-------------------+
                                    |
                +-------------------v-------------------+
                | 4. Atomic Commit + Check Off + Push   |
                |    git add <files>                    |
                |    git commit -m "feat(scope): ..."   |
                |    git push                           |
                |    gh pr edit --body (update to - [x])|
                +-------------------+-------------------+
                                    | (Remaining tasks?)
                                    +-------- Yes -------+
                                    | No                 |
+-----------------------------------v-------------------+|
| 5. Full Validation, Ready & Merge                     ||
|    gh pr checks (verify PR CI passed)                 ||
|    gh pr ready (mark as ready for review)             ||
|    gh pr merge --squash --delete-branch               ||
+-------------------------------------------------------+|
                                    ^                    |
                                    +--------------------+
```

---

## 3. Detailed Execution Steps

### 1. Initialize Task & Draft PR
```bash
gh issue view <issue_id>
git checkout -b feat/issue-<id>-<short-description>
gh pr create --draft \
  --title "<type>: <concise description> (#<issue_id>)" \
  --body "Closes #<issue_id>

### Implementation Tasks
- [ ] 1. Core data structures & config in src/common
- [ ] 2. Daemon audio stream handling in src/daemon
- [ ] 3. Update i18n & unit tests"
```

### 2. Execute Single Item
Pick ONLY the topmost unchecked `- [ ]` item. Do not touch unrelated files.

### 3. Verify with `hk`
```bash
# Preview matched linters
{
  git diff --name-only -z
  git diff --cached --name-only -z
  git ls-files --others --exclude-standard -z
} | hk run check --files0-from - --safe --plan

# Run safe check and auto-format
hk run check --safe
hk fix
```

### 4. Atomic Commit, Push & Check Off
```bash
git add <files>
git commit -m "<type>(<scope>): <message> (#<issue_id>)"
git push origin <branch>

# Update PR body checklist (replace - [ ] with - [x])
gh pr edit --body "..."
```

### 5. Finalize & Merge
```bash
# Verify PR CI status
gh pr checks

# (Optional for core changes) Trigger remote matrix dry build
gh workflow run release.yml && gh run watch

# Mark PR ready and merge
gh pr ready
gh pr merge --squash --delete-branch
```
