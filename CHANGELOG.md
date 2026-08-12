# Changelog

All notable changes to this project will be documented in this file.

## [2.3.6](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.3.6) — 2026-08-12

### Bug Fixes

- **asr:** Route hotwords through the matching sherpa-onnx model-family API: Transducer contextual biasing, FunASR Nano prompts, or Qwen3-ASR prompts
- **asr:** Support Transducer hotword modeling units `cjkchar`, `bpe`, `bbpe`, and `cjkchar+bpe`, and select `modified_beam_search` when contextual biasing is active
- **models:** Generate and validate sherpa-onnx text `bpe.vocab` assets from packaged SentencePiece `bpe.model` files during installation, enabling hotwords for offline and streaming X-ASR models
- **menu:** Avoid a use-after-free when an input context is destroyed while a Vinput menu is visible (#115)
- **config:** Preserve configured defaults when saving instead of replacing them with value-initialized fields (#118)

### Refactor

- **registry:** Use one model index (`registry/models.json`) instead of the versioned v3 path

### Documentation

- Correct the user-facing per-hotword score syntax to `word:2.0`

### Testing

- Add automated coverage for prompt hotword conversion, Transducer modeling-unit validation, and SentencePiece vocabulary export
- Verify the packaged Arch Linux artifact with real offline and streaming X-ASR hotword inference

### Miscellaneous

- Ignore pi-subagents artifacts and drop AGENTS.md

## [2.3.5](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.3.5) — 2026-07-24

### Bug Fixes

- **audio:** Reuse the PipeWire capture stream with `pw_stream_set_active` instead of destroy/create on every recording start, cutting cold PTT open latency after idle (median first non-silent ~405ms → ~50ms in local tests)
- **audio:** Open capture before ASR session creation so model/session setup no longer delays the first mic buffers
- **audio:** Keep an inactive connected stream for a bounded idle grace (default 15s), then destroy it; cancel grace on the next start
- **audio:** Harden deferred stop vs rapid Tap-stop-Tap so a warm reusable stream is not torn down under a new recording
- **asr:** Make offline Silero VAD threshold / min-speech / pad configurable, softens defaults, and log leading/trailing trim separately

### Documentation

- Document capture warm-path privacy bounds and VAD knobs (EN/ZH settings)

### Miscellaneous

- Add `scripts/bench-capture-cold-start.sh` to scrape cold/warm capture timing from daemon journals
- Env knobs: `VINPUT_CAPTURE_REUSE` (default on; `0` = legacy destroy/create), `VINPUT_CAPTURE_IDLE_DESTROY_MS` (default `15000`)

## [2.3.4](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.3.4) — 2026-07-21

### Bug Fixes

- **asr:** Shallow-scan installed models for the F8 menu, reuse one local model listing per open, and read titles from cached registry i18n
- **gui:** Show resource titles and scene labels in user-facing text instead of raw ids
- **gui:** Improve registry i18n preload, refresh, and reload completion

### Miscellaneous

- Drop unused includes and fix i18n cache nodiscard handling
- Ignore local MCP/repowise state and drop stale gitignore paths
- **deps:** Bump sherpa-onnx to v1.13.4
- **deps:** Bump nixpkgs
- **ci:** Only rebuild nix-cache on flake/Nix path changes

## [2.3.3](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.3.3) — 2026-07-04

### Bug Fixes

- Route dbus activation through systemd

### Features

- Reduce system output volume while recording (#105) (#108)

### Miscellaneous

- **deps:** Bump nixpkgs (#106)

## [2.3.2](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.3.2) — 2026-06-27

### Bug Fixes

- **postprocess:** Update default command-mode prompt to use `{{selected}}`/`{{asr}}` interpolation with vinput-scoped XML tags and add migration for existing configs
- **ci:** Add automated build verification workflow for all targets on push/PR

## [2.3.1](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.3.1) — 2026-06-25

### CI

- Split nix cachix cache workflow (#96)

### Documentation

- Add archlinuxcn install instructions
- Add Codex project map

### Miscellaneous

- **deps:** Bump nixpkgs (#98)
- **deps:** Bump sherpa-onnx to v1.13.3 (#104)

### Refactor

- Use XML tags for prompt data isolation, matching interpolation variable names

### Packaging

- PKGBUILD improvements (#102)

## [2.3.0](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.3.0) — 2026-05-25

### Features

- **daemon:** Support file:// prompts and {{result}}/{{context}} interpolation

### Refactor

- **daemon:** Single user message, add {{asr}}/{{selected}}, dump request

### I18n

- **zh_CN:** Translate prompt_file_load_failed messages

### Release

- V2.3.0
## [2.2.4](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.2.4) — 2026-05-24

### Bug Fixes

- **ci:** Quote nixpkgs commit-message to satisfy YAML parser
- **addon:** Suppress OS auto-repeat across daemon-initiated session end
- **audio:** Treat "default" as sentinel and shorten device labels

### Miscellaneous

- **deps:** Bump nixpkgs (#95)

### Release

- V2.2.4
## [2.2.3](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.2.3) — 2026-05-24

### CI

- Add nightly sherpa-onnx upstream sync workflow

### Documentation

- Add Contributors section to README

### Features

- **remote:** Show LAN endpoints in ASR list and drop env fallback
- **daemon:** Add RemoteTextService for remote ASR backend support
- **addon:** Add Tap/Hold/Both trigger mode option

### Miscellaneous

- **deps:** Bump sherpa-onnx to v1.13.2 (#92)
- **deps:** Bump sherpa-onnx to v1.13.1 (#91)
- **deps:** Add nixpkgs update workflow (#90)
- **deps:** Bump sherpa-onnx-flake (#89)
- **deps:** Bump sherpa-onnx to v1.13.0 (#88)
- **sherpa:** Drop unused 1.12.31 sha256 entry

### Refactor

- **config:** Move TriggerMode option to top of config dialog
- **cmake:** Rename targets to short semantic names

### Release

- V2.2.3
## [2.2.2](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.2.2) — 2026-05-03

### Bug Fixes

- **build:** Move VinputConfigDir/VinputDataDir out of anonymous namespace
- Extract shared URL path joining to fix double-slash in models fetch
- Trim all user-input string fields that get persisted to config
- **debian:** Make docker network overrides optional

### Features

- **i18n:** Support local i18n overrides via i18n.local.json
- Trim api_key when adding for convenience (#87)
- **debian:** Implement automated multi-arch docker build workflow

### Nix

- Wrap binaries with python3 and libopus for command providers

### Release

- V2.2.2
## [2.2.1](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.2.1) — 2026-04-18

### Bug Fixes

- **i18n:** Move Extra body strings to vinput::gui::LlmPage context

### Release

- V2.2.1
## [2.2.0](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.2.0) — 2026-04-18

### Bug Fixes

- Rm unused import
- **nix:** Bump sherpa-onnx in flake.lock and revert CI update step

### CI

- **channels:** Update sherpa-onnx flake input before nix build

### Features

- **llm:** Support extra_body passthrough per provider (#77)

### Miscellaneous

- Bump version to 2.2.0

### Refactor

- **cli:** Drop custom CLI11 formatter labels
- **flake:** Use system variable directly instead of pkgs.stdenv
## [2.1.9](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.1.9) — 2026-04-15

### Bug Fixes

- **cli:** Localize help output labels
- **ci:** Remove aarch64 from nix-cachix job

### CI

- **nix:** Add cachix publishing to channels workflow

### Documentation

- Document scene context lines in site

### Features

- **daemon:** Pass model provider to VAD for GPU support
- **addon:** Buffer user commit strings before writing context

### Refactor

- **scene:** Remove context_lines upper limit and cap file dynamically

### Addon

- Record context entry source and timestamp

### Release

- V2.1.9
## [2.1.8](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.1.8) — 2026-04-13

### Bug Fixes

- Add missing Qt translation for context lines
- Expose context lines in GUI and normalize notification locales
- Add xdg-cache Flatpak permission for input context sharing
- Make hero image responsive instead of fixed size
- Use media query for mobile hero centering, revert header change
- Center hero and show header controls on mobile
- Prevent horizontal scroll on mobile hero page

### Documentation

- Add Star History to README
- Add sponsor section to README and simplify sponsor page text
- Update CONTRIBUTING with docs site links and registry guide

### Features

- Input context injection for LLM auto-correction + GUI notifications
- Add SEO optimization (robots.txt, OG meta, JSON-LD)

### Miscellaneous

- Remove deprecated builtin fields from default config
- Add FUNDING.yml with afdian sponsor link

### Refactor

- Drop deprecated scene builtin config field

### Release

- V2.1.8

### Site

- Add sponsor page with donate QR code
## [2.1.7](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.1.7) — 2026-04-11

### Bug Fixes

- Use shebang scripts in justfile release/channels recipes
- Prevent Fcitx5 crash by delaying DBus slot destruction in async callbacks

### CI

- Fix checkout action version v5 -> v4
- Add docs pages workflow

### Documentation

- Add Cachix config and other distro downloads to README
- Restore install sections in README, fix license to GPL-3.0
- Slim READMEs to overview + links to documentation site
- Unify icon color to #7c93ee and add Space Grotesk heading font
- Add landing page gradient, hero image, and tsconfig alias
- Redesign favicon to minimal mic outline
- Add OneDark/OneLight theme and site logo
- Delete unused custom.css
- Remove custom CSS, use Starlight default theme
- Rewrite homepage with real feature content (en + zh-cn)
- Scaffold starlight site

### Miscellaneous

- Split release and channels in justfile
- Bump version to 2.1.7

### Site

- Document LLM adapter response format contract
- Translate zh-cn sidebar titles for ASR and Registry
- Clarify registry scripts are language-agnostic, stdlib-only
- Add registry contribution guide
- Fix table to display:table for full-width row borders
- Add full-width row separators to tables
- Make tables full-width
- Move config file paths to quick start, add provider/adapter paths
- Add ASR, scenes & LLM, and settings concept pages
- Add fcitx5 restart step to load addon after install
- Rename guide page to Quick Start / 快速上手
- Fix video embed to responsive 16:9 aspect ratio
- Slim guide to quick-start with embedded video
- Add configuration guide with GUI screenshots
- Expand install page with full Nix and build-from-source details
- Split install page into repo/local sections with dynamic downloads
- Restore Get Started / 快速上手 hero button text
- Replace placeholder pages with install page
- Refresh homepage product messaging
- Target header title font with site-title hook
- Update docs title font
## [2.1.6](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.1.6) — 2026-04-10

### Miscellaneous

- **release:** Bump version to 2.1.6
## [2.1.5](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.1.5) — 2026-04-10

### CI

- **channels:** Remove blocking publish verification

### Miscellaneous

- **release:** Bump version to 2.1.5
## [2.1.4](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.1.4) — 2026-04-10

### Bug Fixes

- **addon:** Avoid newer dispatcher helper api
- **addon:** Use local event dispatcher for async menu refresh
- **addon:** Refresh ASR menu state off-thread
- **addon:** Avoid blocking ASR menu refresh
- **channels:** Verify exact ppa revision and copr publish

### Miscellaneous

- **release:** Bump version to 2.1.4

## [2.1.3](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.1.3) — 2026-04-10

### Bug Fixes

- **addon:** Make ASR backend switching use async reload handling and cached backend state so menu feedback stays responsive and reflects the active runtime more accurately

## [2.1.2](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.1.2) — 2026-04-10

### Packaging

- **opensuse:** Fix the RPM spec to build and install with an explicit Ninja-based CMake invocation during release packaging

## [2.1.1](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.1.1) — 2026-04-10

### Packaging

- **release:** Add openSUSE Leap RPM artifacts to the release pipeline
- **ci:** Cover openSUSE Leap packaging in CI before release publication

## [2.1.0](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.1.0) — 2026-04-10

### Breaking Changes

- **asr:** Remove Vosk backend support and related runtime packaging pipeline across release channels

### Build System

- **build:** Publish Sherpa runtime artifacts through the main release flow and simplify distro/runtime packaging paths
- **ci:** Extend opensuse and manylinux runtime handling needed by the new Sherpa-only packaging flow

## [2.0.29](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.0.29) — 2026-04-09

### Bug Fixes

- **gui:** Normalize URL trimming in the LLM test flow with `QString` handling so endpoint edits no longer produce malformed requests

### Build System

- **cmake:** Add explicit Vosk runtime mode switches so distro packaging selects system vs bundled behavior in configuration instead of hiding policy in workflows

### Packaging

- **packaging:** Switch Fedora and Arch packaging to distro-native system Vosk dependencies
- **ci:** Make distro-specific Vosk dependency modes explicit in CI jobs
- **release:** Align release packaging modes with the distro support policy
- **channels:** Align channel publishing flows with the same Vosk policy and verify that Ubuntu PPA publication reaches a real installable binary before succeeding

## [2.0.28](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.0.28) — 2026-04-08

### Bug Fixes

- **channels:** Default the PPA revision input to `1` for new manual channel publishes
- **ppa:** Align the Ubuntu channel runtime build with the Ubuntu release packaging flow by building Vosk from source in the target Ubuntu 24.04 environment
- **ppa:** Use an absolute runtime build path during channel packaging so Vosk source builds can resolve Kaldi headers correctly
- **ppa:** Package the Launchpad runtime component as `runtime/` and rebuild invalid cached runtime tarballs instead of reusing incompatible `orig-runtime` archives

## [2.0.27](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.0.27) — 2026-04-08

### Bug Fixes

- **ci:** Install the `file` utility in the Ubuntu 24.04 release container so `cpack -G DEB` can resolve shared-library dependencies and produce the Ubuntu package

## [2.0.26](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.0.26) — 2026-04-08

### Bug Fixes

- **ci:** Build Debian package runtimes inside the target distro containers so release packaging matches the final ABI environment
- **ci:** Validate Kaldi patch headers before applying packaging patches
- **ci:** Configure Kaldi-based runtime builds from the extracted repo root instead of assuming the caller's working directory
- **ci:** Disable unsupported Kaldi test targets during source builds so release packaging no longer fails on missing test data

## [2.0.25](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.0.25) — 2026-04-07

### Bug Fixes

- **addon:** Match trigger-key release events against modifier state so combos like `Alt+R` do not stop recording on unrelated keys such as `Ctrl+R`

## [2.0.24](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.0.24) — 2026-04-06

### Bug Fixes

- **addon:** Lower per-key addon logs to `Debug` so normal Fcitx log output is not flooded during typing

## [2.0.23](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.0.23) — 2026-04-05

### Bug Fixes

- **ppa:** Skip `dh_dwz` and `dh_strip` during Debian packaging so Noble Launchpad builds do not fail on incompatible DWARF and bundled runtime debug processing

## [2.0.22](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.0.22) — 2026-04-05

### Bug Fixes

- **ppa:** Skip `dh_dwz` during Debian packaging so Noble Launchpad builds do not fail on unsupported `.debug_addr` DWARF sections

## [2.0.16](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.0.16) — 2026-04-04

### Bug Fixes

- **ci:** Avoid Fedora weak dependencies that pull the flaky Cisco openh264 mirror during release packaging

## [2.0.15](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.0.15) — 2026-04-04

### Miscellaneous

- **release:** Bump version for channel publishing

## [2.0.14](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.0.14) — 2026-04-03

### Bug Fixes

- **daemon:** Defer ASR backend reload until the daemon is idle
- **ci:** Prefer Qt6 in CI and packaging
- **build:** Require Qt6 for GUI builds

## [2.0.8](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.0.8) — 2026-03-31

### Bug Fixes

- **daemon:** Reload ASR backends inside the daemon with atomic swap semantics so broken model/provider changes keep the previous working backend
- **gui:** Stop forcing daemon restarts for local model and ASR provider changes
- **gui:** Surface daemon start/restart failures through the existing addon notification path

## [2.0.7](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.0.7) — 2026-03-31

### Bug Fixes

- **addon:** Include scene IDs in scene menu filtering without showing them in the menu labels

## [2.0.6](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.0.6) — 2026-03-31

### Bug Fixes

- **postprocess:** Preserve duplicate LLM candidates so result menus still appear when providers return repeated outputs

## [2.0.5](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.0.5) — 2026-03-31

### Bug Fixes

- **ci:** Verify `VERSION` matches manual channels input and release tags
- **build:** Read project version from the top-level `VERSION` file for CMake and Nix builds

### Features

- **daemon:** Add unified `VINPUT_DEBUG` logging with `[vinput-debug]` labels for ASR, recording, adapter, and LLM tracing

## [2.0.4](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.0.4) — 2026-03-30

### Bug Fixes

- **gui:** Ignore invalid `QT_STYLE_OVERRIDE` values before creating `QApplication`
- **gui:** Fetch provider `/models` with auth and HTTP/1.1 fallback to avoid authentication and HTTP/2 issues
- **addon:** Drop unreliable multiline wrapping from preedit and result display paths

### Features

- **gui:** Add internal resource tabs and per-list filtering for models, ASR providers, and LLM adapters
- **addon:** Add `/filter` search to scene selection and shorten menu titles to `Scenes /filter` and `Models /filter`

## [2.0.3](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.0.3) — 2026-03-30

### Bug Fixes

- **models:** Accept tokenizer-based metadata for `funasr_nano` and `qwen3_asr` local models
- **sherpa-offline:** Only pass `model.tokens` when the model family actually uses it

### Features

- **gui:** Add visible download status/progress UI for resource downloads
- **downloads:** Report transfer speed and throttle duplicate progress updates

## [2.0.2](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.0.2) — 2026-03-30

### Bug Fixes

- **command:** Fix null command scene lookup in command mode

## [2.0.1](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v2.0.1) — 2026-03-30

### Bug Fixes

- **streaming:** Accumulate committed segments for multi-segment ASR

### Features

- **addon:** Add unified ASR provider / model selection menu with configurable `ASR Menu Keys` (`F8` by default)

### Internationalization

- Add translations for ASR menu strings

## [1.1.1](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.1.1) — 2026-03-19

### Features

- **gui:** Add candidate count to scene dialog, cache model list
## [1.1.0](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.1.0) — 2026-03-18

### Miscellaneous

- Bump version to 1.1.0, add changelog generation

### Refactor

- Per-scene provider/candidates + i18n fixes + check-i18n.py
- Per-scene provider binding + command scene unification
## [1.0.20](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.0.20) — 2026-03-18

### Bug Fixes

- **cli:** Allow recording stop without active scene, output raw ASR result

### Documentation

- Add AUR, COPR, PPA installation instructions
## [1.0.19](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.0.19) — 2026-03-18

### Bug Fixes

- **ci:** Use new passphrase-free GPG key for PPA signing
- **ci:** Use --no-tty with loopback pinentry for debsign
- **ci:** Use debsign with explicit GPG batch/loopback flags for PPA signing
- **ci:** Remove batch mode from GPG config, conflicts with loopback pinentry
- **ci:** Add batch mode to GPG config for headless signing
- **ci:** Use dpkg-buildpackage with direct GPG signing for PPA
- **ci:** Use publish-ppa-package action for PPA upload
- **ci:** Remove --batch from GPG wrapper to allow signing
- **ci:** Fix GPG wrapper script heredoc indentation
- **ci:** Use GPG wrapper script for headless debsign
- **ci:** Configure gpg.conf for loopback pinentry
- **ci:** Pass empty passphrase for GPG signing
- **ci:** Separate debuild and debsign for headless GPG
- **ci:** Fix debuild lintian flag ordering
- **ci:** Fix GPG signing in headless CI for PPA upload
- **ci:** Provide orig tarball for PPA source package
- **debian:** Remove conflicting compat file
- **spec:** Correct addon .so filename and remove nonexistent inputmethod conf
- **ci:** Skip build-deps check for PPA source package
- **ci:** Use template version in spec for COPR compatibility
- **ci:** Install copr-cli via pip instead of apt

### CI

- Add PPA upload job and debian packaging files
- Add COPR build job and Fedora spec file
- Add build workflow on push/PR and CI badge

### Documentation

- Update demo video filename
- Update demo video
- Use GitHub user-attachments URL for demo video
- Fix demo video URL and clean up features list
- Update trigger modes in README (tap/hold/CLI)
- Add demo video, issue templates, and contributing guide
- Add AUR downloads badge to README

### Miscellaneous

- Bump version to 1.0.19
- Remove unused assets/demo.mp4
## [1.0.18](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.0.18) — 2026-03-17

### Features

- **asr:** Add decoding params, peak normalization, and VAD trimming
## [1.0.17](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.0.17) — 2026-03-17

### Bug Fixes

- **addon:** Support toggle-off on second keypress and track result state
- **cli:** Fix recording toggle reporting start instead of stop
- **daemon:** Prevent capturing desktop audio via PipeWire
## [1.0.16](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.0.16) — 2026-03-17

### Bug Fixes

- **gui:** Use system palette colors and right-align size column

### Documentation

- Fix AUR badge package name
- Beautify README for English and Chinese versions

### Features

- **cli:** Add recording subcommand with D-Bus control

### Refactor

- Extract shared utilities and deduplicate CLI helpers
## [1.0.15](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.0.15) — 2026-03-15

### Bug Fixes

- Use character offsets and validate UTF-8 for command mode selected text
## [1.0.14](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.0.14) — 2026-03-15

### Refactor

- Unify payload format and add candidate count prompt injection
## [1.0.13](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.0.13) — 2026-03-13

### Bug Fixes

- Show result menu only when LLM returns more than one candidate
## [1.0.12](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.0.12) — 2026-03-13

### Documentation

- Split README into separate English and Chinese versions

### Refactor

- Unify LLM response to JSON format and simplify candidate count logic
## [1.0.11](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.0.11) — 2026-03-13

### Bug Fixes

- Create model base directory if not exists before mkdtemp
## [1.0.10](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.0.10) — 2026-03-13

### Bug Fixes

- Auto-commit command result when candidate count is 1
## [1.0.9](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.0.9) — 2026-03-13

### Bug Fixes

- Improve LLM processing logic and prevent duplicate requests
- Prevent use-after-free crash when InputContext is destroyed
## [1.0.8](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.0.8) — 2026-03-12

### Bug Fixes

- **addon:** Guard setComment with version check for fcitx5 < 5.1.9
## [1.0.7](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.0.7) — 2026-03-12

### Features

- LLM error notifications, force-remove model/scene, fix GUI download log
## [1.0.5](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.0.5) — 2026-03-12

### Bug Fixes

- **ci:** Add git to Debian 12 build deps for CLI11 FetchContent clone
## [1.0.4](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.0.4) — 2026-03-12

### CI

- Add Debian 12 build job to release workflow
## [1.0.3](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.0.3) — 2026-03-12

### Bug Fixes

- **ci:** Add cli11 and git to Arch build deps; add cli11 to PKGBUILD makedepends
## [1.0.2](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.0.2) — 2026-03-12

### Bug Fixes

- **addon:** Support FCITX_ADDON_FACTORY_V2 only on fcitx5 >= 5.1.12
## [1.0.1](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.0.1) — 2026-03-12

### Bug Fixes

- **ci:** Move CXX_STANDARD after Fcitx5CompilerSettings; restore packaging/arch/PKGBUILD.in with updated deps
## [1.0.0](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v1.0.0) — 2026-03-12

### Bug Fixes

- **addon:** Persist active scene to config on scene switch
- **daemon:** Robustness and safety improvements
- **common:** Security and robustness fixes
- **addon:** Declare clipboard as hard dependency, fix include path
- **daemon:** Eliminate cross-thread sd-bus access via eventfd emit queue

### CI

- Add missing build deps (libarchive, openssl, qt5) to release workflow

### Documentation

- Remove outdated ARCHITECTURE.md

### Features

- **addon:** Check LLM enabled before command mode; update default keybindings
- **gui:** Replace QListWidget with QTableWidget for model lists
- **cli:** I18n support, CJK-aware table formatting, supports_hotwords column
- Add command mode with dedicated trigger key
- **gui:** Integrate daemon control into general tab and add zh_CN translations
- Complete GUI with scene/LLM tabs, unify i18n, add PipeWire device API
- Support multiple download URLs with fallback in model registry
- Add vinput CLI with init, model/scene/llm/config/daemon management
- Add support for new API endpoint and update data processing logic.
- Implement dynamic model loading via vinput-model.json metadata

### Miscellaneous

- Untrack IDE/build artifacts, update gitignore
- Ignore common backup files (*.po~, *.orig, *~)
- Remove po~ backup file, add to gitignore
- **i18n:** Update zh_CN translations
- **i18n:** Update zh_CN translations and po template
- Add .claude/ to .gitignore
- Add .cache/ and compile_commands.json to .gitignore

### Refactor

- Address all LOW-level review items
- **addon:** Conform to fcitx5 addon conventions
- **hotwords:** Replace word list with file path
- Simplify scene model, remove type and llm fields
- Remove per-provider candidate_count, use global config
- Flatten CoreConfig by removing nested "core" wrapper
- Restructure vinput as a Module addon to coexist with RIME
## [0.1.6](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v0.1.6) — 2026-03-07

### Miscellaneous

- Bump version to 0.1.6
- Add .ace-tool/ to .gitignore

### Refactor

- Normalize LLM base URL at save time instead of request-layer fallback
## [0.1.5](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v0.1.5) — 2026-03-06

### CI

- Publish releases without local git checkout
## [0.1.4](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v0.1.4) — 2026-03-06

### Bug Fixes

- Support older fcitx candidate list APIs
## [0.1.3](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v0.1.3) — 2026-03-06

### Bug Fixes

- Use legacy fcitx standard path API for compatibility
## [0.1.2](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v0.1.2) — 2026-03-06

### CI

- Add missing Debian fcitx5 utils dependency
## [0.1.1](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v0.1.1) — 2026-03-06

### Bug Fixes

- Add nlohmann_json packaging dependency
## [0.1.0](https://github.com/xifan2333/fcitx5-vinput/releases/tag/v0.1.0) — 2026-03-06

### CI

- Add GitHub Actions workflow for automated builds

### Documentation

- Document release packaging

### Features

- Initial completion of first version
