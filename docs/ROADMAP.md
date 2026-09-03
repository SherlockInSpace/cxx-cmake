# Roadmap

The cxx-cmake family is four repositories that ship together: the library template
([cxx-cmake](https://github.com/SherlockInSpace/cxx-cmake), renamed to `cxx-cmake-library` at the
Phase 2 split; GitHub redirects), the build-environment images
([cxx-cmake-container](https://github.com/SherlockInSpace/cxx-cmake-container)), the Yocto layer
([meta-cxx-cmake](https://github.com/SherlockInSpace/meta-cxx-cmake)) and the application template
([cxx-cmake-app](https://github.com/SherlockInSpace/cxx-cmake-app)). Work is planned as five phases;
each phase is a milestone of the same name in every repo it touches, and every PR-sized unit is one
issue (see [DECISIONS.md D17](DECISIONS.md#d17)). The tables below were generated from the
milestones on 2026-09-03; the milestone pages are the live view.

**Sequencing rule.** The container image and the Yocto layer land first, because everything after
them is built and verified inside that image against the Wrynose 6.0 baseline
([D3](DECISIONS.md#d3), [D4](DECISIONS.md#d4), [D15](DECISIONS.md#d15)). The library's legal and
correctness fixes follow, inside the image. Then the repo split and the CI cutover, then quality
gates, then the performance tier, and last the application template with the Yocto-flavor container.
A phase does not need to be fully closed before the next starts, but each phase's ordering
dependencies are recorded as native issue dependencies ("Blocked by") on the issues themselves.

## Phase 1 — Foundations

Establish the ground the rest stands on. The container gains its `ci` and `dev` stages on a
snapshot-pinned Ubuntu 26.04 with the GCC 15 / CMake 4.3.1 toolchain, a smoke test that asserts the
toolchain contract, and multi-arch GHCR publishing on release. The Yocto layer starts in parallel
with pinned kas configs, a SRCREV-pinned `util` recipe and `yocto-check-layer` so BitBake packaging
QA is the acceptance test for the library's install-layout work. The library gets its documentation
set (decisions, design, this roadmap), licensing repair, formatting, the CMake 4.3 / C++23 floor,
the single unit-test binary with the one-test-file-per-source check, the Bloom-filter and
thread-pool correctness fixes under the `std::expected` contract, a proper install/export layout,
and CPM with local-only defaults. Milestones:
[cxx-cmake](https://github.com/SherlockInSpace/cxx-cmake/milestone/1) ·
[cxx-cmake-container](https://github.com/SherlockInSpace/cxx-cmake-container/milestone/1) ·
[meta-cxx-cmake](https://github.com/SherlockInSpace/meta-cxx-cmake/milestone/1).

| Repo | Issue | Title | State |
|---|---|---|---|
| cxx-cmake | [#1](https://github.com/SherlockInSpace/cxx-cmake/issues/1) | ops: repository settings, labels, milestones | closed |
| cxx-cmake | [#2](https://github.com/SherlockInSpace/cxx-cmake/issues/2) | chore: remove GitLab CI and stale README instructions | open |
| cxx-cmake | [#3](https://github.com/SherlockInSpace/cxx-cmake/issues/3) | docs: add decision log (docs/DECISIONS.md) | open |
| cxx-cmake | [#4](https://github.com/SherlockInSpace/cxx-cmake/issues/4) | docs: add design rationale (docs/DESIGN.md) | open |
| cxx-cmake | [#5](https://github.com/SherlockInSpace/cxx-cmake/issues/5) | docs: add roadmap (docs/ROADMAP.md) | open |
| cxx-cmake | [#6](https://github.com/SherlockInSpace/cxx-cmake/issues/6) | chore(license): restore thread-pool upstream MIT notice and add THIRD_PARTY_NOTICES.md | open |
| cxx-cmake | [#7](https://github.com/SherlockInSpace/cxx-cmake/issues/7) | style: add .clang-format and .editorconfig | open |
| cxx-cmake | [#8](https://github.com/SherlockInSpace/cxx-cmake/issues/8) | style: one-time format of first-party files | open |
| cxx-cmake | [#9](https://github.com/SherlockInSpace/cxx-cmake/issues/9) | build(cmake): require CMake 4.3 and C++23; explicit source lists | open |
| cxx-cmake | [#10](https://github.com/SherlockInSpace/cxx-cmake/issues/10) | ci: minimal build-and-test job in the container image | open |
| cxx-cmake | [#11](https://github.com/SherlockInSpace/cxx-cmake/issues/11) | build(cmake): BUILD_TESTING replaces BUILD_TEST; drop dead test branches and the vendored coverage module | open |
| cxx-cmake | [#12](https://github.com/SherlockInSpace/cxx-cmake/issues/12) | test: single util_unit_tests binary with CTest labels | open |
| cxx-cmake | [#13](https://github.com/SherlockInSpace/cxx-cmake/issues/13) | chore(license): SPDX headers and REUSE metadata | open |
| cxx-cmake | [#14](https://github.com/SherlockInSpace/cxx-cmake/issues/14) | test: configure-time one-test-file-per-source check over sources and public headers; drop empty translation units | open |
| cxx-cmake | [#15](https://github.com/SherlockInSpace/cxx-cmake/issues/15) | build(cmake): require OpenSSL and link Crypto privately | open |
| cxx-cmake | [#16](https://github.com/SherlockInSpace/cxx-cmake/issues/16) | test: hygiene — stack-allocated Bloom fixtures, timing test removed, sleeps shrunk, stdout silenced | open |
| cxx-cmake | [#17](https://github.com/SherlockInSpace/cxx-cmake/issues/17) | fix(bloom): remove aliasing UB, the shared mutable buffer, and the init-order mismatch | open |
| cxx-cmake | [#18](https://github.com/SherlockInSpace/cxx-cmake/issues/18) | fix(bloom): migrate MD5 to EVP SHA-256 and drop OpenSSL from the public header | open |
| cxx-cmake | [#19](https://github.com/SherlockInSpace/cxx-cmake/issues/19) | feat(bloom): expected-based create() factory replacing the throwing constructor | open |
| cxx-cmake | [#20](https://github.com/SherlockInSpace/cxx-cmake/issues/20) | refactor(thread_pool): replace empty catch-all handlers | open |
| cxx-cmake | [#21](https://github.com/SherlockInSpace/cxx-cmake/issues/21) | build(cmake): install headers under include/util via FILE_SET | open |
| cxx-cmake | [#22](https://github.com/SherlockInSpace/cxx-cmake/issues/22) | build(cmake): export to lib/cmake/util with util::util alias, VERSION/SOVERSION and BUILD_SHARED_LIBS | open |
| cxx-cmake | [#23](https://github.com/SherlockInSpace/cxx-cmake/issues/23) | chore(cmake): vendor CPM.cmake v0.43.1 | open |
| cxx-cmake | [#24](https://github.com/SherlockInSpace/cxx-cmake/issues/24) | build(cmake): declare OpenSSL and GoogleTest through CPM with local-only defaults | open |
| cxx-cmake | [#25](https://github.com/SherlockInSpace/cxx-cmake/issues/25) | refactor(bit): move into the util namespace and document the header-only specimen | open |
| cxx-cmake | [#60](https://github.com/SherlockInSpace/cxx-cmake/issues/60) | chore(tools): planning scripts — labels, milestones, issues, project | open |
| cxx-cmake-container | [#1](https://github.com/SherlockInSpace/cxx-cmake-container/issues/1) | chore: remove GitLab CI template and stock README | closed |
| cxx-cmake-container | [#2](https://github.com/SherlockInSpace/cxx-cmake-container/issues/2) | build(docker): ci stage on ubuntu:26.04 with snapshot-pinned apt and the GCC 15 toolchain | open |
| cxx-cmake-container | [#3](https://github.com/SherlockInSpace/cxx-cmake-container/issues/3) | build(docker): CMake 4.3.1 pin, baseline dependencies and quality tooling | open |
| cxx-cmake-container | [#4](https://github.com/SherlockInSpace/cxx-cmake-container/issues/4) | build(docker): dev stage (non-root user, zsh, sudo) | open |
| cxx-cmake-container | [#5](https://github.com/SherlockInSpace/cxx-cmake-container/issues/5) | test: smoke script asserting the toolchain contract | open |
| cxx-cmake-container | [#6](https://github.com/SherlockInSpace/cxx-cmake-container/issues/6) | ci: PR gate — build both stages on amd64 and arm64, smoke test, PR-title lint | open |
| cxx-cmake-container | [#7](https://github.com/SherlockInSpace/cxx-cmake-container/issues/7) | ci: release-please maintaining CHANGELOG and tags | open |
| cxx-cmake-container | [#8](https://github.com/SherlockInSpace/cxx-cmake-container/issues/8) | ci: publish multi-arch images to GHCR on release | open |
| cxx-cmake-container | [#9](https://github.com/SherlockInSpace/cxx-cmake-container/issues/9) | ops: flip both GHCR packages public and confirm the repo link | open |
| cxx-cmake-container | [#14](https://github.com/SherlockInSpace/cxx-cmake-container/issues/14) | ops: repository settings | closed |
| cxx-cmake-container | [#15](https://github.com/SherlockInSpace/cxx-cmake-container/issues/15) | chore: add MIT license and REUSE metadata | open |
| cxx-cmake-container | [#16](https://github.com/SherlockInSpace/cxx-cmake-container/issues/16) | chore: dependabot for actions and base image | open |
| cxx-cmake-container | [#17](https://github.com/SherlockInSpace/cxx-cmake-container/issues/17) | docs: README, AGENTS.md, ROADMAP slice | open |
| meta-cxx-cmake | [#1](https://github.com/SherlockInSpace/meta-cxx-cmake/issues/1) | ops: create repo, settings, labels, milestones | closed |
| meta-cxx-cmake | [#2](https://github.com/SherlockInSpace/meta-cxx-cmake/issues/2) | feat: layer skeleton | closed |
| meta-cxx-cmake | [#3](https://github.com/SherlockInSpace/meta-cxx-cmake/issues/3) | build(kas): pinned kas configs for the wrynose 6.0.2 component repos and this layer | open |
| meta-cxx-cmake | [#4](https://github.com/SherlockInSpace/meta-cxx-cmake/issues/4) | feat(recipes): util recipe pinned by SRCREV to the library tip | open |
| meta-cxx-cmake | [#8](https://github.com/SherlockInSpace/meta-cxx-cmake/issues/8) | ci: yocto-check-layer and PR-title lint | open |
| meta-cxx-cmake | [#9](https://github.com/SherlockInSpace/meta-cxx-cmake/issues/9) | docs: local build guide (kas-container loop) | open |

## Phase 2 — Portability & CI cutover

Split the repository (archive branches, rename to `cxx-cmake-library`, rewrite `main`, set the
template flag) and move CI to GitHub Actions running in the Phase 1 image: presets for `dev` and
`release`, a PR gate on gcc Debug and Release with JUnit rendering and a format check,
release-please with PR-title lint, the first automated release `v0.2.0`, a post-merge full suite
with a native arm64 leg, a weekly drift cron, an installed-package consumer smoke test, sanitizer
and coverage presets with thresholds, and the README rewritten as the template manual. The layer
re-pins `util` to the tag and packages ptest; the app repo only updates its family links.
Milestones: [cxx-cmake](https://github.com/SherlockInSpace/cxx-cmake/milestone/2) ·
[meta-cxx-cmake](https://github.com/SherlockInSpace/meta-cxx-cmake/milestone/2) ·
[cxx-cmake-app](https://github.com/SherlockInSpace/cxx-cmake-app/milestone/1).

| Repo | Issue | Title | State |
|---|---|---|---|
| cxx-cmake | [#26](https://github.com/SherlockInSpace/cxx-cmake/issues/26) | ops: repo split — archive branches, rename, rewrite main, template flag | open |
| cxx-cmake | [#27](https://github.com/SherlockInSpace/cxx-cmake/issues/27) | build(cmake): CMakePresets.json (dev, release) | open |
| cxx-cmake | [#28](https://github.com/SherlockInSpace/cxx-cmake/issues/28) | ci: PR gate on presets — gcc Debug+Release, unit tier, JUnit rendering, format check | open |
| cxx-cmake | [#29](https://github.com/SherlockInSpace/cxx-cmake/issues/29) | ci: PR-title lint and release-please | open |
| cxx-cmake | [#30](https://github.com/SherlockInSpace/cxx-cmake/issues/30) | ops: first automated release v0.2.0 | open |
| cxx-cmake | [#31](https://github.com/SherlockInSpace/cxx-cmake/issues/31) | ci: post-merge full suite (adds arm64 leg) | open |
| cxx-cmake | [#32](https://github.com/SherlockInSpace/cxx-cmake/issues/32) | ci: weekly drift cron with auto-issue on failure | open |
| cxx-cmake | [#33](https://github.com/SherlockInSpace/cxx-cmake/issues/33) | test(functional): installed-package consumer smoke test under CTest, joined to the PR gate | open |
| cxx-cmake | [#35](https://github.com/SherlockInSpace/cxx-cmake/issues/35) | build(cmake): sanitizer presets | open |
| cxx-cmake | [#36](https://github.com/SherlockInSpace/cxx-cmake/issues/36) | ci: clang leg and plain-distro proof leg | open |
| cxx-cmake | [#37](https://github.com/SherlockInSpace/cxx-cmake/issues/37) | build(coverage): coverage preset with an aggregate gcovr run | open |
| cxx-cmake | [#38](https://github.com/SherlockInSpace/cxx-cmake/issues/38) | ci(coverage): thresholds, per-file floor, step summary | open |
| cxx-cmake | [#39](https://github.com/SherlockInSpace/cxx-cmake/issues/39) | docs: README as the template manual | open |
| cxx-cmake | [#40](https://github.com/SherlockInSpace/cxx-cmake/issues/40) | docs: README "using this template" section | open |
| meta-cxx-cmake | [#5](https://github.com/SherlockInSpace/meta-cxx-cmake/issues/5) | feat(recipes): re-pin util to tag v0.2.0 | open |
| meta-cxx-cmake | [#10](https://github.com/SherlockInSpace/meta-cxx-cmake/issues/10) | feat(recipes): ptest packaging for util | open |
| cxx-cmake-app | [#1](https://github.com/SherlockInSpace/cxx-cmake-app/issues/1) | docs: update family links after the library rename | open |

## Phase 3 — Quality gates, docs, agents

Turn the CI from "it builds" into a gate: a warnings target with `-Werror` in CI presets,
`.clang-tidy` with the camelCase naming rules and the renames it demands, sanitizer jobs (asan/ubsan
on PR, tsan post-merge), Doxygen with `WARN_AS_ERROR` and a Pages deploy of docs and coverage,
CodeQL and dependabot, and the reusable `workflow_call` workflows the app template consumes later.
Agent-readiness lands here too: `AGENTS.md`, a `CLAUDE.md` pointer, a devcontainer pinned to the
image and an optional pre-commit config. The layer proves the recipes build under BitBake on the
self-hosted runner and publishes SDK installers as release assets. Milestones:
[cxx-cmake](https://github.com/SherlockInSpace/cxx-cmake/milestone/3) ·
[meta-cxx-cmake](https://github.com/SherlockInSpace/meta-cxx-cmake/milestone/3).

| Repo | Issue | Title | State |
|---|---|---|---|
| cxx-cmake | [#34](https://github.com/SherlockInSpace/cxx-cmake/issues/34) | ci: extract gate, post-merge and weekly into reusable workflow_call workflows | open |
| cxx-cmake | [#41](https://github.com/SherlockInSpace/cxx-cmake/issues/41) | build(cmake): warnings INTERFACE target, fixes, and -Werror in CI presets | open |
| cxx-cmake | [#42](https://github.com/SherlockInSpace/cxx-cmake/issues/42) | style: .clang-tidy with camelCase naming rules | open |
| cxx-cmake | [#43](https://github.com/SherlockInSpace/cxx-cmake/issues/43) | refactor: identifier renames to satisfy clang-tidy | open |
| cxx-cmake | [#44](https://github.com/SherlockInSpace/cxx-cmake/issues/44) | ci: clang-tidy job | open |
| cxx-cmake | [#45](https://github.com/SherlockInSpace/cxx-cmake/issues/45) | ci: sanitizer jobs — asan-ubsan on PR, tsan post-merge | open |
| cxx-cmake | [#46](https://github.com/SherlockInSpace/cxx-cmake/issues/46) | docs: doxygen_add_docs with the image's doxygen-awesome-css | open |
| cxx-cmake | [#47](https://github.com/SherlockInSpace/cxx-cmake/issues/47) | docs: fix headers to pass WARN_AS_ERROR; docs job in the post-merge suite | open |
| cxx-cmake | [#48](https://github.com/SherlockInSpace/cxx-cmake/issues/48) | ci: Pages deploy of docs and coverage HTML | open |
| cxx-cmake | [#49](https://github.com/SherlockInSpace/cxx-cmake/issues/49) | chore: dependabot for actions | open |
| cxx-cmake | [#50](https://github.com/SherlockInSpace/cxx-cmake/issues/50) | ops: enable CodeQL default setup | open |
| cxx-cmake | [#51](https://github.com/SherlockInSpace/cxx-cmake/issues/51) | docs: AGENTS.md and CLAUDE.md pointer | open |
| cxx-cmake | [#52](https://github.com/SherlockInSpace/cxx-cmake/issues/52) | chore: devcontainer.json pinned to the container image | open |
| cxx-cmake | [#53](https://github.com/SherlockInSpace/cxx-cmake/issues/53) | chore: pre-commit config (optional) | open |
| meta-cxx-cmake | [#6](https://github.com/SherlockInSpace/meta-cxx-cmake/issues/6) | ci: kas build of the recipes — the "builds under BitBake" proof | open |
| meta-cxx-cmake | [#7](https://github.com/SherlockInSpace/meta-cxx-cmake/issues/7) | ci: SDK job (populate_sdk) publishing installers as release assets | open |

## Phase 4 — Performance tier

Add the performance tier on the cadence [D9](DECISIONS.md#d9) sets: Google Benchmark behind
`BUILD_BENCH`, thread-pool throughput and expected-vs-throw benchmarks, weekly and release benchmark
runs with a `perf` label and a trend dashboard, Tracy harness targets ([D10](DECISIONS.md#d10):
test-side only) with `.tracy` captures from the weekly runs, and `docs/profiling.md`. The container
gains Tracy and Google Benchmark. Milestones:
[cxx-cmake](https://github.com/SherlockInSpace/cxx-cmake/milestone/4) ·
[cxx-cmake-container](https://github.com/SherlockInSpace/cxx-cmake-container/milestone/2).

| Repo | Issue | Title | State |
|---|---|---|---|
| cxx-cmake | [#54](https://github.com/SherlockInSpace/cxx-cmake/issues/54) | feat(bench): bench/ with Google Benchmark behind BUILD_BENCH | open |
| cxx-cmake | [#55](https://github.com/SherlockInSpace/cxx-cmake/issues/55) | feat(bench): thread-pool throughput and expected-vs-throw benchmarks | open |
| cxx-cmake | [#56](https://github.com/SherlockInSpace/cxx-cmake/issues/56) | ci: weekly and release benchmark runs, perf label, trend dashboard | open |
| cxx-cmake | [#57](https://github.com/SherlockInSpace/cxx-cmake/issues/57) | feat(perf): Tracy harness targets in the performance tier | open |
| cxx-cmake | [#58](https://github.com/SherlockInSpace/cxx-cmake/issues/58) | ci: .tracy capture artifacts on weekly runs | open |
| cxx-cmake | [#59](https://github.com/SherlockInSpace/cxx-cmake/issues/59) | docs: docs/profiling.md | open |
| cxx-cmake-container | [#12](https://github.com/SherlockInSpace/cxx-cmake-container/issues/12) | build(docker): Tracy | open |
| cxx-cmake-container | [#19](https://github.com/SherlockInSpace/cxx-cmake-container/issues/19) | build(docker): Google Benchmark | open |

## Phase 5 — App template & Yocto

Seed the application template from the archived monorepo and bring it to parity with the library:
C++23 and CMake 4.3, spdlog via CPM, `util` consumed from the image baseline with a local-checkout
override, unit and pytest-driven integration tiers, the shared style configs, an `ENABLE_TRACY`
option, and CI through the library's reusable workflows. The container gains the app tooling, an
installed `util` baseline, build provenance attestation and the yocto flavor consuming the layer's
SDK; the layer adds the app recipe. Milestones:
[cxx-cmake-app](https://github.com/SherlockInSpace/cxx-cmake-app/milestone/2) ·
[cxx-cmake-container](https://github.com/SherlockInSpace/cxx-cmake-container/milestone/3) ·
[meta-cxx-cmake](https://github.com/SherlockInSpace/meta-cxx-cmake/milestone/4).

| Repo | Issue | Title | State |
|---|---|---|---|
| cxx-cmake-container | [#10](https://github.com/SherlockInSpace/cxx-cmake-container/issues/10) | build(docker): app-template tooling — spdlog, pytest, ruff | open |
| cxx-cmake-container | [#11](https://github.com/SherlockInSpace/cxx-cmake-container/issues/11) | build(docker): install util vX.Y.Z into ci/dev | open |
| cxx-cmake-container | [#13](https://github.com/SherlockInSpace/cxx-cmake-container/issues/13) | build(docker): yocto flavor consuming the SDK from Y-10 | open |
| cxx-cmake-container | [#18](https://github.com/SherlockInSpace/cxx-cmake-container/issues/18) | ci: build provenance attestation | open |
| meta-cxx-cmake | [#11](https://github.com/SherlockInSpace/meta-cxx-cmake/issues/11) | feat(recipes): app recipe | open |
| meta-cxx-cmake | [#12](https://github.com/SherlockInSpace/meta-cxx-cmake/issues/12) | docs: README, AGENTS.md, ROADMAP slice | open |
| cxx-cmake-app | [#2](https://github.com/SherlockInSpace/cxx-cmake-app/issues/2) | ops: settings, MIT license + REUSE, labels, milestones | closed |
| cxx-cmake-app | [#3](https://github.com/SherlockInSpace/cxx-cmake-app/issues/3) | chore: seed app/ from tag archive/monorepo-main | open |
| cxx-cmake-app | [#4](https://github.com/SherlockInSpace/cxx-cmake-app/issues/4) | fix(app): index argv inside the loop | open |
| cxx-cmake-app | [#5](https://github.com/SherlockInSpace/cxx-cmake-app/issues/5) | build(cmake): modernise — C++23, CMake 4.3, explicit sources, presets, warnings target, CPM vendored | open |
| cxx-cmake-app | [#6](https://github.com/SherlockInSpace/cxx-cmake-app/issues/6) | build(cmake): spdlog via CPM | open |
| cxx-cmake-app | [#7](https://github.com/SherlockInSpace/cxx-cmake-app/issues/7) | build(cmake): consume util from the image baseline with the local-checkout override | open |
| cxx-cmake-app | [#8](https://github.com/SherlockInSpace/cxx-cmake-app/issues/8) | test(unit): unit tier and the one-test-file-per-source check | open |
| cxx-cmake-app | [#9](https://github.com/SherlockInSpace/cxx-cmake-app/issues/9) | style: .clang-format/.clang-tidy/.editorconfig from the library | open |
| cxx-cmake-app | [#10](https://github.com/SherlockInSpace/cxx-cmake-app/issues/10) | feat: ENABLE_TRACY option (default OFF) | open |
| cxx-cmake-app | [#11](https://github.com/SherlockInSpace/cxx-cmake-app/issues/11) | test(integration): pytest driving the binary under CTest, ruff for Python | open |
| cxx-cmake-app | [#12](https://github.com/SherlockInSpace/cxx-cmake-app/issues/12) | ci: gate, post-merge, weekly via the library's reusable workflows; PR-title lint; release-please | open |
| cxx-cmake-app | [#13](https://github.com/SherlockInSpace/cxx-cmake-app/issues/13) | docs: README, AGENTS.md, devcontainer, ROADMAP refreshed | open |
| cxx-cmake-app | [#14](https://github.com/SherlockInSpace/cxx-cmake-app/issues/14) | build(cmake): target preset for the yocto-flavor container | open |

## Deferrals

- **The `target` preset.** A `CMakePresets.json` preset that builds against the Yocto SDK sysroot is
  deferred from the Phase 2 presets work and lands with the yocto-flavor container
  ([cxx-cmake-container#13](https://github.com/SherlockInSpace/cxx-cmake-container/issues/13) —
  backlog C-20, [cxx-cmake-app#14](https://github.com/SherlockInSpace/cxx-cmake-app/issues/14)) —
  until that image exists there is nothing for the preset to point at.
- **The board-runner lane.** [D9](DECISIONS.md#d9) and [D21](DECISIONS.md#d21) reserve a lane that
  executes ptest and benchmarks on real hardware. There is no issue for it and there will not be one
  until hardware exists; the design seam (what the lane consumes, how a board's `MACHINE` sits
  beside `qemuarm64`) is recorded in `docs/profiling.md` when Phase 4 writes it.
