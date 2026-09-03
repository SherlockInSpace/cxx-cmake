# Decision log

This is the decision log for the cxx-cmake template family: the library template (this repo), the
application template, the build-environment container images
([cxx-cmake-container](https://github.com/SherlockInSpace/cxx-cmake-container)) and the Yocto
layer. It records *why* things are the way they are so that a reader, human or agentic, can check
the reasoning before "fixing" it. D0–D14 were settled in the planning rounds that preceded this
repo's restructure; D15–D21 were settled when the issue-level backlog was drafted. Each entry points
at the configuration that enforces it where one exists: the log is a map, not the source of truth.
Some pointers name where enforcement lands per the roadmap and may precede the file they name; the
same goes for defaults an amendment records ahead of the change that implements them.

**Proposing a change.** Open an issue labelled `decision-needed` stating the decision ID, the change
and the reason, then open a PR against this file. Amend an entry in place with a dated *Amendment*
line rather than rewriting its history; a superseded decision keeps its number and gains a pointer
to its replacement. Issues and PRs reference decisions by anchor: `docs/DECISIONS.md#d15`, and
`#d15-1` for the sub-decision.

---

<a id="d0"></a>
### D0 — Platform: GitHub
- **Settled:** 2026-09-02.
- **Decision.** The family lives on GitHub. Everything the plan needs — arm64 runners, job
  containers, Pages, GHCR, release automation, native issue dependencies — is available there. All
  GitLab files and mentions are removed from every repo.
- **Rationale.** GitLab's remaining edge was its merge-request test and coverage widgets, and
  marketplace actions plus step summaries close that gap (D18).
- **See also.** `.github/workflows/` (lands with the CI cutover).

<a id="d1"></a>
### D1 — Topology: separate template repos
- **Settled:** 2026-09-02.
- **Decision.** Library and application are two separate GitHub template repos; the container images
  and the Yocto layer are a third and fourth repo. History on the default branch may be rewritten
  before the split; no history preservation is required.
- **Rationale.** A template is instantiated by copying one repo; a monorepo would force every fork
  to carry both halves and the environment definitions.

<a id="d2"></a>
### D2 — thread_pool stays, attribution restored
- **Settled:** 2026-09-02.
- **Decision.** The vendored `thread_pool.hpp` stays in the library template with its upstream MIT
  notice restored verbatim, listed in `THIRD_PARTY_NOTICES.md` (upstream URL, version, SHA256, local
  modifications).
- **Rationale.** Vendoring done right is itself a lesson the template should teach.
- **See also.** `THIRD_PARTY_NOTICES.md`; D16.

<a id="d3"></a>
### D3 — Dependencies and reproducible environments
- **Settled:** 2026-09-02.
- **Decision.** "The system" means the pinned container image or the Yocto sysroot, never the
  developer's host distro. Both templates use one dependency mechanism, CPM (a pinned, SHA-verified
  vendored script), defaulting to `CPM_USE_LOCAL_PACKAGES=ON` + `CPM_LOCAL_PACKAGES_ONLY=ON`: every
  dependency resolves `find_package`-first against the baseline and errors rather than downloads;
  one knob opens fetching for beyond-baseline work and `CPM_<NAME>_SOURCE` is the single
  local-checkout override. The container repo publishes versioned, Wrynose-matching images to GHCR
  for devcontainers, CI and a `docker run` one-liner; one plain-distro CI leg is the buildability
  proof.
- **Rationale.** Resolve-from-baseline is reproducible and Yocto-safe by default — a recipe build
  has no network, so a template that downloads at configure time cannot be packaged. CPM's known
  gaps (options ignored on local resolve, `VERSION` as a minimum, quiet lookup) are documented with
  workarounds rather than papered over by a second mechanism.
- **Amendment (2026-09-03).** The container's `ci` stage runs as **root**, because GitHub Actions
  job containers only work reliably as root (runner-owned mounts); the `dev` stage is a **non-root**
  user with sudo, built with UID/GID build-args so files created on the mounted workspace belong to
  the host user.
- **See also.** `cmake/CPM.cmake`, `CMakePresets.json`; the container repo's `Dockerfile`.

<a id="d4"></a>
### D4 — Baseline: a single Yocto LTS
- **Settled:** 2026-09-02.
- **Decision.** The toolchain baseline is Yocto **Wrynose 6.0** only (supported to April 2030); no
  Scarthgap support; the next move is at the 7.0 LTS. `cmake_minimum_required(4.3)` — the declared
  minimum, the container, `cmake-native` and the SDK are one version.
- **Rationale.** One LTS means one toolchain to verify and one parity table to maintain;
  off-container users get CMake 4.3+ from pip or the Kitware tarball, so the old 3.28 courtesy floor
  bought only a second matrix.
- **Amendment (2026-09-03).** The baseline is pinned to the current point release, **Wrynose 6.0.2
  (GCC 15.3)**; the distro-flavor image's patch-level deltas are accepted per D15.1.
- **See also.** `CMakeLists.txt` (`cmake_minimum_required`); the container README's parity table.

<a id="d5"></a>
### D5 — Language and error contract
- **Settled:** 2026-09-02.
- **Decision.** Full C++23 floor. `std::expected` replaces exceptions in library code: no `throw`,
  no empty `catch (...)`. The application-side exception policy is deferred to the app-template
  design.
- **Rationale.** With GCC 15 the only toolchain, a GCC 13-clean blocklist had no purpose;
  `std::expected` makes failure part of the signature at a small, deterministic cost.
- **See also.** `.clang-tidy`; `CMakeLists.txt` (`cxx_std_23`).

<a id="d6"></a>
### D6 — OpenSSL as the system-dependency demo
- **Settled:** 2026-09-02.
- **Decision.** OpenSSL stays as the example system dependency: `REQUIRED`, linked `PRIVATE
  OpenSSL::Crypto`, EVP/SHA-256, no OpenSSL header in any installed header.
- **Rationale.** A template needs one real `find_package` dependency to show the private-link,
  no-leak pattern; the deprecated MD5 call is the migration exercise.

<a id="d7"></a>
### D7 — Test enforcement: one test file per source
- **Settled:** 2026-09-02.
- **Decision.** The one-test-file-per-source convention is kept and enforced by a configure-time
  check over the explicit source list and the public header set, plus a per-file coverage floor.
  Binary granularity is D12.
- **Rationale.** A convention that is only prose drifts; a configure error naming the missing file
  is the cheapest possible enforcement.
- **Amendment (2026-09-03).** The exemption variable is **`UTIL_TEST_EXEMPT`**; the failure text
  reads *"add test/unit/src/<stem>.cpp or list <stem> in UTIL_TEST_EXEMPT"*. **`BUILD_TESTING` keeps
  CMake's default ON** (`include(CTest)` is the only switch) and the Yocto recipe controls it
  through its ptest `PACKAGECONFIG`. The documentation switch `BUILD_DOCS` defaults **OFF** and is
  not in `ALL`, so a plain configure builds only the library and its tests.
- **See also.** `CMakeLists.txt`, `test/CMakeLists.txt`, `CMakePresets.json`.

<a id="d8"></a>
### D8 — CI cadence
- **Settled:** 2026-09-02.
- **Decision.** PR fast gate → post-merge full suite on every push to the default branch → weekly
  cron as the environment-drift catcher (auto-issue on failure) → release tag with baseline
  comparison.
- **Rationale.** Post-merge failures attribute themselves to one commit; the weekly run catches what
  a pinned image cannot (expired snapshots, action deprecations).
- **See also.** `.github/workflows/`.

<a id="d9"></a>
### D9 — Performance cadence
- **Settled:** 2026-09-02.
- **Decision.** Benchmarks run weekly and at release (release-blocking against the previous
  baseline), optionally on `perf`-labelled PRs. The power lane is a design paragraph, no
  infrastructure.
- **Rationale.** Per-PR benchmarks on shared runners are noise; trend-first with a gate at release
  is what the numbers can support.

<a id="d10"></a>
### D10 — Tracy: test-side only
- **Settled:** 2026-09-02.
- **Decision.** Zero Tracy references in library sources in any configuration. Zones live in
  `performance`-tier harnesses (the fourth test tier) and sampling covers library internals; the app
  template may show in-app zones behind a default-OFF `ENABLE_TRACY`.
- **Rationale.** A library's public surface must not depend on a profiler; instrumenting the harness
  keeps the library binary identical in every build.

<a id="d11"></a>
### D11 — Naming: `util`, camelCase
- **Settled:** 2026-09-02.
- **Decision.** The example library keeps the name `util`; the house style is camelCase, encoded in
  clang-tidy; vendored code is exempt and keeps upstream style.
- **Rationale.** A style enforced by a tool rather than a document is inherited by forks unchanged.
- **See also.** `.clang-tidy`, `.clang-format`, `THIRD_PARTY_NOTICES.md`.

<a id="d12"></a>
### D12 — Unit-test binary granularity
- **Settled:** 2026-09-02.
- **Decision.** One `util_unit_tests` binary; per-module selection via `ctest -R` or
  `--gtest_filter`.
- **Rationale.** One binary is what ptest packages and what coverage merges; selection is a filter,
  not a build graph.

<a id="d13"></a>
### D13 — Versioning and commit conventions
- **Settled:** 2026-09-02.
- **Decision.** SemVer + Conventional Commits + squash-merge-only PRs (PR numbers auto-appended);
  PR-title lint in the PR gate; issue references in bodies and footers; clang-format/clang-tidy (and
  ruff when Python arrives) via pre-commit locally with CI as the gate. release-please from day one:
  a rolling release PR produces the CHANGELOG, SemVer bump, tag and GitHub Release, and its generic
  updater keeps `project(VERSION)` the single version source.
- **Rationale.** Squash-merge makes the PR title the one commit message worth linting, and a
  file-first version means the tag and the CMake version come from the same commit.
- **Amendment (2026-09-03).** **git-cliff is the documented, forge-portable escape hatch**: it reads
  the same Conventional Commits and needs no GitHub-specific automation should the release flow ever
  have to leave GitHub.
- **See also.** `release-please-config.json`, `.pre-commit-config.yaml`, `.github/workflows/`; D19.

<a id="d14"></a>
### D14 — Agent-readiness
- **Settled:** 2026-09-02.
- **Decision.** `AGENTS.md` at each repo root (with `CLAUDE.md` as a one-line pointer) is written to
  survive a fork unmodified: commands are named only via presets, conventions are pointers to the
  enforcing config, nothing template-instantiation-specific appears. The definition of done is
  executable (tests, the test-file check, coverage floors, format/tidy, the docs gate — each a
  command); error messages are written as instructions; the devcontainer is pinned to the
  environment image; this decision log is committed.
- **Rationale.** Agents succeed where correctness is machine-checkable; each item above turns tribal
  knowledge into a command or a file an agent can read.
- **Amendment (2026-09-03).** The test-file check's instruction names `UTIL_TEST_EXEMPT` (D7).
- **See also.** `AGENTS.md`, `CMakePresets.json`, `.devcontainer/devcontainer.json`.

<a id="d15"></a>
### D15 — Where Yocto enters the sequence
- **Settled:** 2026-09-03.
- **Decision.** Distro-flavor container first: `ubuntu:26.04` pinned by index digest with apt pinned
  to a snapshot ID, CMake 4.3.1 as a checksummed Kitware tarball, version-level parity with Wrynose
  6.0.2 except patch-level deltas in GCC (15.2 vs 15.3), binutils (2.46 vs 2.46.1) and OpenSSL
  (3.5.5 vs 3.5.7). In parallel the Yocto layer starts in Phase 1 — skeleton, kas config pinned to
  the 6.0.2 component repos, a SRCREV-pinned `util` recipe, a workstation `kas build` loop
  (`externalsrc` override) and a `yocto-check-layer` CI job — so BitBake packaging QA is the
  acceptance test for the library's install-layout fixes. Full BitBake builds run on the self-hosted
  runner (D20).
- **Rationale.** Yocto's prebuilt buildtools carry no sanitizer runtimes and hard-wire the SDK
  loader, so they cannot host the sanitizer legs; a BitBake-built SDK is a cross toolchain whose
  binaries cannot execute natively on Ubuntu, so it becomes the later Yocto-flavor image rather than
  the Phase 1 source. Behavioural parity with the Yocto toolchain is that flavor's job.
- **See also.** The container README's parity table; the Yocto layer's `kas/` configs.

<a id="d15-1"></a>
### D15.1 — Toolchain exactness in the distro flavor
- **Settled:** 2026-09-03.
- **Decision.** Accept the Ubuntu archive's patch-level deltas, pinned and tabled; no from-source
  toolchain in the distro flavor. The parity target is the latest Wrynose 6.0.x point release
  (currently 6.0.2, GCC 15.3).
- **Rationale.** Zero build cost with the same ABI and glibc; a Wrynose point-release bump is a
  container MINOR version plus a one-line table change.
- **See also.** D4, D19; the container README's parity table.

<a id="d16"></a>
### D16 — Licensing mechanics
- **Settled:** 2026-09-03.
- **Decision.** MIT everywhere, REUSE 3.3 compliant: SPDX headers on first-party source,
  `LICENSES/MIT.txt`, `REUSE.toml` for headerless files, `reuse lint` in CI; `COPYING.MIT`
  additionally in the Yocto layer; the thread-pool notice restored verbatim (D2); container images
  carry `org.opencontainers.image.licenses=MIT` with a README note that the label covers the repo's
  content while the image contains GPL/LGPL toolchain packages under their own licences.
- **Rationale.** REUSE makes licensing machine-checkable, the only kind a template can promise
  forks.
- **See also.** `LICENSE`, `REUSE.toml`, `THIRD_PARTY_NOTICES.md`.

<a id="d17"></a>
### D17 — Planning mechanics
- **Settled:** 2026-09-03 (with one condition).
- **Decision.** One issue per PR-sized unit, in the repo it changes; the body carries the decision
  ID (linking here), scope, "Done when" and "Blocked by"; issue title = PR title; one squash-merged
  PR per issue (D13). Mechanical changes (formatting, renames, vendored or generated files,
  deletions) get their own PR. Milestones per repo are the roadmap phases; one shared label set
  (`area:*`, `ops`, `decision-needed`, `mechanical`); native issue dependencies; one user-level
  GitHub Project fed by script. **Condition:** the mechanics are documented here and in `AGENTS.md`,
  and the scripts are committed under `tools/planning/` so another agentic system or session can
  follow and modify them.
- **Rationale.** PR-sized issues keep every diff reviewable line by line; committed scripts make the
  process reproducible rather than remembered.
- **See also.** `tools/planning/`, `AGENTS.md`, `docs/ROADMAP.md`.

<a id="d18"></a>
### D18 — JUnit rendering action
- **Settled:** 2026-09-03.
- **Decision.** `mikepenz/action-junit-report` renders test results in PRs; fall back to
  `dorny/test-reporter` if it disappoints.
- **Rationale.** The marketplace answer to the widget gap noted in D0; it is a trial, so the
  alternative is named up front.
- **See also.** `.github/workflows/`.

<a id="d19"></a>
### D19 — Naming and versioning
- **Settled:** 2026-09-03.
- **Decision.** Container packages `ghcr.io/sherlockinspace/cxx-cmake-container/ci`, `…/dev` and
  later `…/yocto` share one SemVer; no `latest`, no suffix or sub-patch tags. MAJOR = toolchain
  baseline change; MINOR = tool added or Wrynose point-release bump; PATCH = rebuild without version
  change. Library and app: `project(VERSION)` is the single source, rewritten by release-please's
  generic updater, and the tag is created from that commit (file-first, fully automatic). SOVERSION
  = `PROJECT_VERSION_MAJOR`; the package version file is `SameMinorVersion` before 1.0 and
  `SameMajorVersion` after; the Yocto recipe pins `v${PV}`; the app declares its library requirement
  as a CPM `VERSION` floor.
- **Rationale.** SemVer reads `x.y.z-N` as a pre-release, so sub-patch tags sort backwards; reading
  `git describe` at configure time breaks under Yocto and tarball builds, so the version lives in a
  file.
- **See also.** `CMakeLists.txt` (`project(VERSION)`), `release-please-config.json`; D13.

<a id="d20"></a>
### D20 — Where BitBake runs for CI
- **Settled:** 2026-09-03 (with one condition).
- **Decision.** A self-hosted runner on the owner's workstation, `workflow_dispatch` and push-only;
  fork PRs are never scheduled. **Condition:** the workflow reads its runner labels from one
  repository variable (`BITBAKE_RUNS_ON`) so moving to a rented VM under the same labels is a flag
  flip; the docs cover registering the workstation, registering a VM, and pointing the variable at a
  larger-runner label should the repos ever move into an organisation on a plan that offers them.
- **Rationale.** Hosted runners' 6-hour job cap and 10 GB cache cannot hold a multi-GB sstate.
- **See also.** The Yocto layer's `.github/workflows/`.

<a id="d21"></a>
### D21 — Layer target MACHINE and where tests execute
- **Settled:** 2026-09-03.
- **Decision.** **Cloud CI is for correctness only, and nothing is ever executed under emulation.**
  ARM correctness comes from GitHub's arm64 runners — real ARM hardware — running the unit and
  functional tests natively in the dev container. The Yocto build is cross-compile + package + QA
  with no execution step; `MACHINE = qemuarm64` is a build target only and `runqemu` is never
  invoked. ptest is packaged now and executed only on real hardware via the board lane, where the
  board's MACHINE is added beside `qemuarm64`.
- **Rationale.** Emulated results are neither correctness nor performance evidence; native arm64
  runners give the former for free and a board gives the latter when one exists.
- **See also.** `.github/workflows/`; the Yocto layer's `kas/` configs; D9, D20.

