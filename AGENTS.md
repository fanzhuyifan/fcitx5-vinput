# AGENTS.md

## Project Map

When working in this repository, treat `fcitx5-vinput` as the main application
repo. Some tasks require checking sibling repositories before making a change or
review decision.

### Main Repository

Local path: `~/code/fcitx5-vinput`
GitHub repo: `xifan2333/fcitx5-vinput`

Role: Fcitx5 voice input application: addon, daemon, CLI, GUI, documentation,
packaging templates, and GitHub release workflows.

Check here for:

- application behavior, daemon/addon/CLI/GUI changes
- default resource registry configuration
- release assets consumed by downstream packaging automation
- packaging manifests/templates for deb, rpm, Arch, and Flatpak
- non-Nix `sherpa-onnx` version pins

Key files:

- `data/default-config.json`: default `registry.base_urls`
- `flake.nix`: Nix package and `sherpa-onnx-flake` input
- `scripts/sherpa-onnx-vars.sh`: upstream `sherpa-onnx` version and hashes for non-Nix builds
- `.github/workflows/release.yml`: release artifact production
- `.github/workflows/sync-sherpa-onnx.yml`: automated upstream `sherpa-onnx` bump PRs
- `site/src/content/docs/registry/`: registry contribution docs

### Runtime Resource Registry

Local path: `~/code/vinput-registry`
GitHub repo: `xifan2333/vinput-registry`

Role: remote runtime resource catalog for installed Vinput clients.

Check this repo when a task touches:

- downloadable ASR models
- cloud ASR provider scripts
- managed local LLM adapter scripts
- resource IDs, `short_id`, script URLs, env declarations, or i18n display text
- compatibility between registry metadata and `fcitx5-vinput` fetch/install code

Key files:

- `registry/models.json`: local ASR model entries and `vinput_model` metadata
- `registry/providers.json`: cloud ASR provider entries
- `registry/adapters.json`: LLM adapter entries
- `i18n/en_US.json`, `i18n/zh_CN.json`: display text keyed by `<id>.title` and `<id>.description`
- `resources/providers/**/entry.py`: provider scripts downloaded by clients
- `resources/adapters/**/entry.py`: adapter scripts downloaded by clients

Boundary: this repo changes what resources users can discover or install after
the app is installed. It does not publish app binaries.

### Nix Sherpa Runtime

Local path: `~/code/sherpa-onnx-flake`
GitHub repo: `xifan2333/sherpa-onnx-flake`
Upstream repo: `kakapt/sherpa-onnx-flake`

Role: Nix flake packaging for `sherpa-onnx` and its native dependencies.

Check this repo when a task touches:

- Nix builds of `fcitx5-vinput`
- `flake.nix` or `flake.lock` behavior in this repo
- `sherpa-onnx` build flags, patches, or dependency packaging under Nix
- failures that happen only in Nix/NixOS paths

Key files:

- `flake.nix`: exported package set
- `packages/sherpa-onnx.nix`: `sherpa-onnx` derivation
- `packages/*.nix`: dependency derivations
- `patches/*.patch`: local build patches

Boundary: this repo affects Nix packaging. Non-Nix release builds in
`fcitx5-vinput` use `scripts/build-sherpa-onnx.sh` and upstream release
archives.

### AUR Publishing Automation

Local path: `~/code/aur-auto`
GitHub repo: `xifan2333/aur-auto`

Role: automation for maintaining AUR packages, including `fcitx5-vinput-bin`.

Check this repo when a task touches:

- Arch/AUR package publishing
- `fcitx5-vinput-bin` version, checksum, install script, or metadata
- automation that tracks `fcitx5-vinput` GitHub Releases
- AUR user-facing install path from README

Key files:

- `pkgs/fcitx5-vinput-bin/PKGBUILD`: AUR binary package definition
- `pkgs/fcitx5-vinput-bin/upstream.sh`: latest release detection and checksum update
- `pkgs/fcitx5-vinput-bin/fcitx5-vinput-bin.install`: post-install hooks
- `.github/workflows/build-and-publish.yml`: AUR validation and publishing
- `scripts/update-package.sh`: generic package update driver

Boundary: this repo consumes released Arch package assets from
`fcitx5-vinput`; do not use it to change app behavior.

### Flatpak Publishing Automation

Local path: `~/code/flatpak-auto`
GitHub repo: `xifan2333/flatpak-auto`

Role: automation for importing Flatpak release bundles into a reusable OSTree
repository and publishing `.flatpakrepo` / `.flatpakref` files.

Check this repo when a task touches:

- Flatpak install instructions or distribution
- `fcitx5-vinput.flatpak` release bundle ingestion
- published `xifan.flatpakrepo` or `org.fcitx.Fcitx5.Addon.Vinput.flatpakref`
- Flatpak product metadata/state

Key files:

- `products/fcitx5-vinput/product.env`: app ID, upstream repo, release asset name
- `products/fcitx5-vinput/upstream.sh`: latest release and bundle URL detection
- `products/fcitx5-vinput/state.env`: last synced version state
- `.github/workflows/sync-and-publish.yml`: sync and publish workflow
- `scripts/sync-product.sh`, `scripts/publish-repo.sh`: import and publish flow

Boundary: this repo consumes `fcitx5-vinput.flatpak` from GitHub Releases. The
Flatpak build manifest itself lives in `fcitx5-vinput/packaging/flatpak/`.

## Routing Rules

- Runtime resource discovery/install: inspect `fcitx5-vinput`, then `vinput-registry`.
- New or changed model/provider/adapter: inspect `vinput-registry` and matching parser/materialization code in `fcitx5-vinput`.
- Default registry URL changes: inspect `data/default-config.json` and verify the target layout in `vinput-registry`.
- Nix build issue: inspect `flake.nix`, `flake.lock`, and `sherpa-onnx-flake`.
- Upstream `sherpa-onnx` bump: inspect `scripts/sherpa-onnx-vars.sh`, `packaging/flatpak/*.yaml`, and `sherpa-onnx-flake` for Nix impact.
- AUR publishing issue: inspect `aur-auto`; inspect this repo's release workflow only if the expected release asset is missing or named differently.
- Flatpak publishing issue: inspect `flatpak-auto`; inspect `packaging/flatpak/` if the bundle content or manifest is wrong.
- README install command issue: check `README*.md`, then the relevant downstream repo (`aur-auto` or `flatpak-auto`) for truth.
