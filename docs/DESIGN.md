# Design rationale

This document explains *why* the cxx-cmake template family is shaped the way it is and *how* the
pieces fit together. It is the narrative behind the [decision log](DECISIONS.md): the log records
each decision, its date and the configuration that enforces it; this document records the reasoning
that connects them. Where a heading carries a decision number, that decision is the settled form of
the argument made here — read the log entry for the exact terms, and read this for the trade-offs.
Neither document is the source of truth for a value: the configuration files the log points at are.

The family is four repositories: the library template (this repo), the application template, the
build-environment container images
([cxx-cmake-container](https://github.com/SherlockInSpace/cxx-cmake-container)) and the Yocto
layer. They are separate because a template is instantiated by copying one repo, and a fork should
carry exactly what it needs ([D1](DECISIONS.md#d1)).

## The beliefs underneath

The design started from a short list of convictions about library code, revisited before the
planning rounds and found to hold — mostly they needed sharpening into something a tool can check,
not replacement.

- **Small reusable functions with good names.** Encoded rather than exhorted: clang-tidy carries
  the naming rules, and a style note stays short because the tool does the work.
- **Documentation is key**, with a modern tilt: doc comments explain intent and contracts; inline
  comments are reserved for the genuinely non-obvious *why*. Doxygen's `WARN_AS_ERROR` makes an
  undocumented public symbol a CI failure, not a review comment.
- **Testing is key**, and the instinct that there are several kinds of test became four named
  tiers with a cadence each (below).
- **Performance and coverage are validated**, but measuring regressions and understanding hotspots
  are different jobs with different tools, so they are separate lanes.
- **Libraries don't print.** The flip side, error handling, gets a real contract instead of a
  habit: `std::expected` for recoverable errors, and no empty `catch (...)` anywhere.
- **Minimise library dependencies.** Now a stated, enforced policy rather than a preference.
- **Profiling never ships in the library.** Confirmed as originally stated, and the strictest form
  of it: zero profiling code and zero profiling dependency in library sources in every
  configuration.

The thread running through all of these is the same: a convention that lives only in prose drifts,
so every belief above is backed by a check that fails loudly.

## Platform — [D0](DECISIONS.md#d0)

GitHub supports everything the plan needs: template repositories, hosted arm64 runners that are
real ARM hardware, job containers, Pages for the documentation and coverage sites, GHCR for the
environment images, scheduled workflows, release automation and native issue dependencies. Public
repositories get unlimited Actions minutes and the 4-vCPU runner tier for free, and the design
assumes public repositories throughout: on a private repository the same cadence bills minutes and
the post-merge tier below would be batched. Larger hosted runners are never free and require an
organisation on a Team plan; the BitBake lane runs self-hosted because hosted runners' 6-hour cap
and 10 GB cache cannot hold an sstate ([D20](DECISIONS.md#d20)), with a larger-runner label kept as
the flag-flip option should the repos ever move into such an organisation.

The one thing GitLab does better natively is merge-request widgets: it ingests JUnit and Cobertura
XML with zero setup and paints per-line coverage into the diff. GitHub renders neither natively.
The gap closes with one workflow step each — a marketplace action for the per-test PR check
([D18](DECISIONS.md#d18)), gcovr's Markdown table in the step summary, and the full coverage HTML on
Pages. `CMakePresets.json` keeps every CI job forge-portable regardless, so the platform choice is
reversible at the workflow layer rather than the build layer.

## Build system and the Yocto contract — [D4](DECISIONS.md#d4), [D15](DECISIONS.md#d15)

The whole family tracks **one** Yocto LTS. The baseline is Wrynose 6.0.2 (GCC 15.3, CMake 4.3.1,
glibc 2.43), supported to April 2030; the next move is at the 7.0 LTS. One LTS means one toolchain
to verify and one parity table to maintain. A second supported release would have doubled the
matrix and, because older releases ship older compilers, would have capped the language level at
whatever the oldest one supports.

`cmake_minimum_required(4.3)` follows from the same principle: the declared minimum, the container,
Yocto's `cmake-native` and the SDK are one version, so drift between them is impossible by
construction. The cost is that no distro package manager satisfies it — no Ubuntu archive carries
CMake ≥ 4.3 — so anyone building outside the container installs 4.3.1 from PyPI or Kitware's
checksummed tarball. That is one documented command, and the plain-distro CI leg does exactly the
same thing, which keeps it an honest test of the declared minimum rather than a test of whatever
CMake a runner image happens to ship.

The build system is written to be packaged by BitBake without any BitBake-specific code. The
contract has four parts:

1. **No network at configure time.** A recipe build is offline; the dependency policy below makes
   offline the default posture, not a special case.
2. **`GNUInstallDirs` plus a relocatable export.** Headers go to `include/util/` through
   `target_sources(FILE_SET HEADERS)`, the config package to `lib/cmake/util`, and the exported
   targets carry no absolute paths, so the same install works in a scratch prefix, a sysroot and
   an SDK.
3. **No toolchain file in the presets.** A Yocto SDK environment exports `CMAKE_TOOLCHAIN_FILE`
   and CMake honours it, so the repo cross-compiles inside an SDK shell with zero changes.
   Presets are the single source of configure truth, and build directories are per preset
   (`build/<presetName>`), so host and target artifacts never collide in a shared workspace.
4. **Tests and docs are switchable.** `BUILD_TESTING` is the recipe's ptest switch; `BUILD_DOCS` is
   off and outside `ALL`, so a plain configure builds the library and nothing else.

Why the container comes before the Yocto flavour is a toolchain-mechanics argument, recorded in
D15: Yocto's prebuilt buildtools carry no sanitizer runtimes and hard-wire the SDK loader, and a
BitBake-built SDK is a cross toolchain whose binaries cannot run natively on the host. Neither can
host the sanitizer legs. So the first image is a distro flavour — Ubuntu 26.04 pinned by digest and
apt snapshot, with the distro's GCC 15.2 accepted as a patch-level delta against Wrynose's 15.3
([D15.1](DECISIONS.md#d15-1)) — and the Yocto flavour lands later as the image that carries the
real cross toolchain. Version-level parity is the distro flavour's promise; behavioural parity is
the Yocto flavour's job.

## Dependency policy — [D3](DECISIONS.md#d3)

"The system" is the pinned environment image for host builds and the Yocto sysroot for target
builds — never the developer's host distro. Building against whatever libraries a workstation
happens to have is not a supported flow. The plain-distro CI leg exists as a *proof of
buildability*, and the README says so, because a template that quietly works on one developer's
machine and not another's has failed at its only job.

Both templates use exactly one dependency mechanism, CPM, so a developer learns one flow. Its
default posture is resolve-from-baseline: `CPM_USE_LOCAL_PACKAGES=ON` and
`CPM_LOCAL_PACKAGES_ONLY=ON`, so every `CPMAddPackage` tries `find_package` against the
image or sysroot first and **errors instead of downloading** when the package is missing. That is
reproducible by default and Yocto-safe by construction: `cmake.bbclass` forces
`FETCHCONTENT_FULLY_DISCONNECTED=ON` and root-path-only lookups, which is the same posture, so a
recipe still just declares `DEPENDS` and the sysroot provides. Beyond-baseline work flips one knob:
`-DCPM_LOCAL_PACKAGES_ONLY=OFF` fetches a pinned copy of a declared dependency (with
`CPM_SOURCE_CACHE` for cross-project reuse), and `CPM_<NAME>_SOURCE` — settable from the
environment — points a dependency at a local checkout.

Uniformity has a price, and CPM's known deficiencies are documented with their workarounds rather
than hidden behind a second mechanism:

1. **`OPTIONS` are silently ignored when a package resolves locally.** Rule: a dependency that
   needs non-default build options is baked into the baseline image, where its options are
   controlled and versioned, or forced to source with `CPM_DOWNLOAD_<name>`. Never rely on
   `OPTIONS` for a dependency that may resolve locally.
2. **`VERSION` is a `find_package` minimum, not a pin, in local mode.** True pinning lives in the
   container tag — reproducibility by image, not by declaration. Declare `VERSION` as the floor
   the code actually needs.
3. **The local lookup is `QUIET`.** The configure log's `CPM: Using local package …` lines are the
   audit trail.
4. **It is a vendored third-party script.** Pinned by version and SHA256, one file, bumped by PR
   like any other dependency.

The example system dependency is OpenSSL ([D6](DECISIONS.md#d6)) because it is in every Yocto
sysroot, which is what makes the demo honest. It is linked `PRIVATE` and no OpenSSL header appears
in any installed header, so the installed package needs no `find_dependency` and consumers compile
self-contained; runtime linkage rides the shared object's `DT_NEEDED`. The docs then show the
contrast: a dependency whose types *do* appear in public headers would be `PUBLIC` plus
`find_dependency` in the config template. The private form is demonstrated; the public form is
taught.

## Testing — [D7](DECISIONS.md#d7), [D12](DECISIONS.md#d12)

Four tiers, each a CTest label, one build, `ctest -L <tier>`:

| Tier | What it answers | Lives in | Cadence |
|------|-----------------|----------|---------|
| `unit` | Does each API do what its doc comment says? | library, app | every PR |
| `functional` | Can a consumer install the package, `find_package(util CONFIG REQUIRED)`, compile and run? — the Yocto contract, tested on the host | library | every PR |
| `integration` | Does the application behave as a process? PyTest drives the binary, JUnit under CTest | app | every PR |
| `performance` | Did it get slower, and where? Google Benchmark plus profiling captures | both | weekly, release |

`functional` is the tier that earns its place least obviously. It is the only test of the install
layout, the config package and the exported targets — exactly the surface a BitBake build consumes
and a unit test never touches. It runs against a scratch prefix so it needs no privileges.

The one-test-file-per-source convention is kept because it is the cheapest possible answer to "is
this file tested at all?", and it is enforced at configure time because a convention that is only
prose drifts. The check runs over the explicit source list and the public header set and fails
with a message that names the missing file and the exemption variable. A per-file coverage floor
backstops the semantic gap the name check cannot see: a test file that exists but tests nothing
fails coverage, not the honour system.

**Why one unit-test binary rather than one per source.** Selection does not require separate
binaries: `gtest_discover_tests` registers every `TEST()` as an individual CTest test regardless of
how the files are linked, so `ctest -R Bloom` or `--gtest_filter='Bloom*'` gives the same
granularity, and IDE test explorers list individual tests from one binary just as they do from
several. Per-source binaries cost N link steps instead of one — link time is the slow part of a
small project's rebuild and grows with every source — N target definitions, and N CI and coverage
touchpoints; CI that hard-codes per-source target names is the failure mode where adding a source
file means editing the pipeline. What they buy is crash isolation, which CTest's per-test reporting
already mostly provides, and marginally faster incremental links. For a three-module template the
trade is clearly one binary, and it is also what ptest packages and what coverage merges. A fork
that grows large can split by module cluster later without touching the per-file convention, since
the *files* are the same either way.

## CI cadence and failure attribution — [D8](DECISIONS.md#d8), [D21](DECISIONS.md#d21)

Three tiers, each answering a different question, plus the release:

- **Every PR — "did I break anything?"** Target under 15 minutes: gcc and clang across Debug and
  Release, `unit` and `functional` with JUnit rendering, format check, clang-tidy, ASan+UBSan,
  coverage summary and thresholds. Fast, deterministic, merge-blocking.
- **Post-merge, every push to the default branch — "test everything, with exact attribution."**
  The full suite: complete matrix including the arm64 leg, TSan, CodeQL, a benchmark trend point,
  Doxygen `WARN_AS_ERROR`, the consumer test. Public-repo minutes are free (the assumption stated
  under Platform), so the full suite runs per merge rather than nightly — and per merge is what
  makes failures self-attributing. Each run covers exactly one squash-merged PR, so the run that
  went red names the commit.
- **Weekly cron — the drift catcher.** The same full suite with no code change, valuable precisely
  because a red weekly run points at the environment: runner-image updates, action deprecations,
  expired snapshots, benchmark-runner drift. On failure the workflow opens an issue, because a
  scheduled run nobody watches is a run that silently rots.
- **Release tag — "ship it."** Everything above plus the benchmark comparison against the previous
  release (which can block), docs deploy to Pages, packaged artifacts, release notes.

Attribution is therefore structural rather than procedural: PR-tier failures point at the PR,
post-merge failures at the one merge that triggered the run, weekly failures with no new commits at
the outside world. The residual bisect case is a bug inside a large PR's own commits, and
`git bisect run` with a preset-driven build-and-test script handles that locally in log₂ N builds.
Per-commit CI of every individual commit is what large monorepos with merge queues do; it is
overkill here because PRs merge as units. An environment bump is a one-line PR that changes the
image tag, so it lands in the same attribution model as any other commit.

**Cloud CI is for correctness only, and nothing runs under emulation.** ARM correctness comes from
GitHub's arm64 runners, which are real ARM hardware, running the unit and functional tests natively
inside the dev container. The Yocto build in CI is cross-compile, package and QA with no execution
step; `qemuarm64` is a build target, and `runqemu` is never invoked. Emulated results are neither
correctness evidence (a different kernel and CPU model) nor performance evidence, so the design
refuses to produce them. ptest is packaged now and executed only when real hardware exists.

Jobs run in the pinned GHCR image so **CI is the development environment**; one plain-distro leg —
a stock Ubuntu runner image, the distro's `gcc-15`, CMake 4.3.1 from PyPI or the tarball — stays
as proof that the build has no hidden dependence on the container.

## Coverage

gcov plus gcovr, driven by a few project-owned lines rather than a vendored CMake module; a line
threshold and the per-file floor from the testing section are the gate, and the HTML lands on
Pages. A hosted coverage service is opt-in only: a template should not ask a fork for an account
before its first build.

## Documentation

Doxygen builds the reference. The docs target stays outside `ALL`, so it never slows a plain
build, and `WARN_AS_ERROR` turns it into a gate: a public symbol without a doc comment is a CI
failure, not a review comment. The README is the template's manual — how to instantiate it, the
canonical commands, the develop-against-a-local-checkout flow — and stays short enough to read in
one sitting.

## Performance — [D9](DECISIONS.md#d9), [D10](DECISIONS.md#d10)

Performance work has two lanes because measurement and insight are different jobs.

**Measurement and gating** is Google Benchmark under `bench/`, behind `BUILD_BENCH=OFF`,
Release-only and guarded against the coverage configuration. Benchmarks run weekly and on release
tags, where a comparison against the previous release's baseline can block the release; PRs run
none by default and a perf-sensitive PR opts in. Per-commit gating on shared cloud runners is
noise, and the numbers cannot support it; trends over weeks are meaningful where single runs are
not.

**Insight** is Tracy, and it lives **only in the `performance` tier's own targets**. The objection
that settled this was simple: a profiling-ON library build is a shipped-artifact variant carrying a
dependency, and that variant must not exist. So library sources contain zero Tracy references in
every configuration — no macros, no option, no dependency — and the library that ships is
byte-for-byte the library that was built. Profiling harnesses and benchmark executables link the
Tracy client and put zones around library calls *from the harness side*; Tracy's sampling mode,
which needs only debug symbols, still sees function-level detail inside the library. The honest
trade: internal timings are statistical rather than exact zones. For a utility library that is
ample, and a developer hunting one internal hotspot can add temporary zones on a working branch
that never merges. The application template is different because an application is the final
binary: it may demonstrate in-code zones behind a default-OFF `ENABLE_TRACY`, since instrumenting
an app is its author's shipping decision, not a library contract. The two lanes meet in the weekly
run: the benchmark says *that* something regressed, a capture says *where*.

**Power profiling is parked**, and deliberately so: no board exists, so nothing is built. What the
design keeps is the seam. The `performance` tier is label-selected and its results are benchmark
JSON, so a future self-hosted board runner wrapping the same command with a power-capture tool and
emitting `{"name": …, "unit": "J", "value": …}` flows into the same dashboard with no new
infrastructure. That paragraph costs nothing and keeps the door open.

## Error contract — [D5](DECISIONS.md#d5)

With GCC 15 the only toolchain, the language floor is the full C++23 standard — `std::expected`,
`std::move_only_function`, `std::print`, `std::generator`, `ranges::to`, deducing `this` — with no
"older-compiler-clean" subset. C++26 stays off the table until compiler support is more than
experimental.

Recoverable errors in library code return `std::expected<T, ErrorEnum>`; contract violations are
`assert`-guarded and covered by death tests; empty `catch (...)` is banned, so a swallowed exception
needs either a documented rationale or an error callback. This keeps the API usable from
`-fno-exceptions` consumers and makes failure part of the signature. Whether *applications* use
exceptions is deliberately deferred to the application template's design — an application owns its
process and may reasonably choose differently.

**The overhead is not zero and the design does not claim it is.** `expected` and `optional` cost a
discriminant byte plus padding per object and one well-predicted branch per check. What the
current compilers do deliver: no heap allocation ever, register returns for small trivially copyable
payloads (roughly 16 bytes and under — so error *enums*, not strings, in the error channel), and
codegen with the historical weaknesses fixed. Exceptions are cheaper on the happy path (zero
instructions) and much more expensive on the error path (heap plus unwind). For shallow-call
utility APIs the `expected` cost is a branch, which is noise, and the `performance` tier can carry
an expected-versus-throw microbenchmark so the claim stays measurable in-repo rather than asserted.

## Naming — [D11](DECISIONS.md#d11)

The house style is camelCase — types `UpperCamelCase`, functions and methods `camelBack`, members
`camelBack_`, constants `kUpperCamel` — encoded in clang-tidy's `readability-identifier-naming` so
a fork inherits it unchanged. Vendored third-party code is exempt and keeps upstream style: that is
standard practice, it is declared in `THIRD_PARTY_NOTICES.md`, and it keeps the file close to
upstream for future syncs. Vendoring done right — notice restored verbatim, provenance and local
modifications listed — is itself a lesson the template teaches ([D2](DECISIONS.md#d2)).

The rest of the quality tooling follows one pattern: a warnings `INTERFACE` target applied
`PRIVATE` with `-Werror` only in CI; `.clang-format` and `.editorconfig` for layout; a curated
`.clang-tidy` as a CI job; sanitizer jobs; CodeQL's default setup. pre-commit runs the same tools
locally for fast feedback, but **CI remains the gate**, so nothing depends on every developer
installing hooks.

## Versioning and commit conventions — [D13](DECISIONS.md#d13), [D19](DECISIONS.md#d19)

SemVer, Conventional Commits and squash-merge-only PRs fit together mechanically. Squash-merge
makes the PR title the commit subject and appends the PR number automatically, so the only commit
message worth linting is the PR title, and one action in the PR gate does it — no commit-by-commit
policing, and every default-branch commit is well-formed and PR-linked. Issue references live in
the body or footer, never the subject. `feat!:` and `BREAKING CHANGE:` tie the commit style to the
SemVer bump.

release-please closes the loop from day one: it parses those commits, maintains a rolling release
PR that accumulates the changelog and computes the bump, and merging that PR tags and publishes the
GitHub Release with no manual step. Its generic updater rewrites the annotated `project(VERSION)`
line, so the file is the single version source and the tag is created *from* that commit —
file-first, which matters because reading `git describe` at configure time breaks under Yocto and
tarball builds. git-cliff is the documented escape hatch should the flow ever have to leave GitHub;
it reads the same commits but leaves bumping, tagging and publishing to be scripted.

The same reasoning fixes the container tags: SemVer only, no `latest`, no suffix or sub-patch tags,
because SemVer reads `x.y.z-N` as a pre-release and sorts it backwards, and because `latest` was
exactly the old setup's reproducibility killer — CI pulled it, and builds changed underneath.

## Reproducible environments — [D3](DECISIONS.md#d3), [D15](DECISIONS.md#d15)

The requirement is that two developers on two machines produce byte-identical builds, and that a
toolchain fix rolls out to everyone at once. That is answered by an **environment repository**,
not by the dependency mechanism. The container repo publishes versioned, Wrynose-matching images to
GHCR — a `ci` stage that runs as root because Actions job containers only work reliably that way,
and a `dev` stage with a non-root user whose UID and GID match the host so files on the mounted
workspace stay owned by the developer. Images are multi-arch, which is nearly free given real arm64
runners.

Three consumers share every image: a `devcontainer.json` in each template pins the exact tag, so
developers, Codespaces and coding agents get the identical toolchain automatically; CI runs in the
same tag, so CI *is* the dev environment; and a documented `docker run` one-liner covers everyone
else. Bumping the environment is a one-line PR — visible, reviewable, bisectable.

CPM sits on top of this layer, not beside it. The container provides the *baseline* every
`CPMAddPackage` resolves against by default; CPM's knobs provide the *override* — a local checkout,
or one dependency fetched at a chosen version — so a developer can target a specific release or
swap one library to test it without rebuilding the environment.

**The two developer scenarios**, step to mechanism:

1. **Library developer.** "Use this template" → open in the devcontainer or `docker run -v
   <workspace>:/work` on the pinned image. The shared workspace mount works across flavours because
   build directories are per preset, so host and target artifacts never collide. Mid-development
   they need a change in an internal dependency → check it out beside the repo and point at it
   with one knob, the same in both templates: `CPM_<NAME>_SOURCE=/work/otherlib` →
   `ctest --preset dev` → push.
2. **Application developer.** Same start, plus a target build: the Yocto flavour carries the SDK
   cross toolchain, so `cmake --preset target` cross-compiles because the SDK environment exports
   the toolchain and the presets stay toolchain-free. On-target profiling is the app's
   `ENABLE_TRACY` build plus `tracy-capture` from the host over TCP.

Two seams the scenarios surface are named rather than solved: the **cross-repo change flow** is two
PRs — merge the dependency, then bump the pin — deterministic because baselines are versioned, and
**on-target test execution** stays a designed-later seam until a board exists.

## Agent-readiness and the genericity constraint — [D14](DECISIONS.md#d14)

Most of the plan already *is* the agent setup, because agents succeed exactly where correctness is
machine-checkable rather than tribal. What sits on top is small: `AGENTS.md` at each repo root with
`CLAUDE.md` as a one-line pointer; a definition of done where every item is a command — tests, the
test-file check, coverage floors, format and tidy, the docs gate — so completion is verified rather
than asserted; presets as the single command vocabulary, so nobody guesses flags; error messages
written as instructions, so a configure failure says what to add; the devcontainer pinned to the
environment image, so an agent gets the exact toolchain a developer does; and this pair of
documents committed, so an agent can read *why* before "fixing" something.

The **genericity constraint** is what keeps `AGENTS.md` honest: it must survive a fork
**unmodified**. Every line has to pass the test *"still true after the project is renamed and ten
sources are added?"* Concretely, commands are named only via presets, which are fork-stable by
design; conventions are pointers to the enforcing configuration (`.clang-format`, `.clang-tidy`,
the configure-time test check) rather than prose that could drift; the vendored-code rule points at
`THIRD_PARTY_NOTICES.md` instead of naming files; and nothing template-instantiation-specific
appears, since the rename checklist belongs in the README's "using this template" section. Dynamic
facts live in machine-readable files, and `AGENTS.md` references them instead of duplicating them.

Planning follows the same principle ([D17](DECISIONS.md#d17)): one issue per PR-sized unit with an
executable "Done when", mechanical changes in their own PR so every diff stays reviewable line by
line, and the scripts that maintain the backlog committed so the process is reproducible rather
than remembered.

## What the design deliberately does not do

Each of these has a revisit trigger, and none is an oversight.

- **Power-profiling infrastructure** — parked to the design paragraph above until a board exists.
- **Per-commit benchmark gating** — shared runners cannot support it; weekly trends and a release
  gate can.
- **Emulated test execution** — neither correctness nor performance evidence.
- **BitBake on hosted runners** — the 6-hour job cap and 10 GB cache cannot hold an sstate.
- **Coverage-service badges and tokens by default** — opt-in, commented out.
- **pkg-config files** — until a non-CMake consumer exists.
- **An umbrella or index repo, cookiecutter or copier** — a template is a repo you copy; the
  rename checklist is a README section.
- **More utilities in the example library** — the template teaches structure, not a library.
