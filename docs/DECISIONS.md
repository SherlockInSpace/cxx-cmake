# Decisions

Why the template is shaped the way it is, for whoever forks it and wonders whether a choice was
deliberate. To change one, open an issue or send a pull request that edits this file.

## One Yocto LTS baseline

The library targets one Yocto long-term-support release, the newest: Wrynose 6.0, pinned to point
release 6.0.2 (GCC 15.3, CMake 4.3.1, glibc 2.43, supported until April 2030). Two toolchains would
mean a lowest-common-denominator language subset or a matrix of conditionals, so the previous LTS
is not carried; the next baseline move is the next LTS.

## Reproducible environments

Two developers on two machines must produce the same build, so the toolchain lives in its own
repository, `cxx-cmake-container`, and every build of the library runs inside an image it publishes
to GHCR as `ci`, `dev` and (once the SDK image exists) `yocto` under one shared semver, never
`latest`, so an environment bump is a one-line pull request that sits in history and bisects like
any other change.

"Matching the baseline" means version-level parity, not bit-for-bit identity. The base is Ubuntu
26.04, the newest LTS and the distribution whose packages land nearest Wrynose, with apt pinned to
an archive snapshot so a rebuild resolves identical package versions on both architectures, plus
CMake 4.3.1 from a checksummed tarball because no Ubuntu archive carries 4.3. CMake, Ninja, glibc,
LLVM 22 and GoogleTest match exactly; GCC, binutils and OpenSSL trail by a patch level (the table is
in [DESIGN.md](DESIGN.md)). We accept those deltas because the ABI and glibc are the same and none
of the alternatives closes the gap. Building GCC 15.3 from source would still not behave like
Yocto's: Ubuntu's compiler defaults to PIE, `_FORTIFY_SOURCE=3` and control-flow protection, while
Yocto's is FSF-default with hardening from distro flags and its own patches. Yocto's prebuilt
`buildtools-extended` tarball carries the exact GCC but no sanitizer runtimes, so the ASan, UBSan
and TSan jobs could not run on it, and it hard-wires the SDK's own dynamic loader into every binary
it builds. A BitBake-built SDK is a cross toolchain whose output uses the target loader path and
cannot execute natively, and no aarch64-host installers exist; it becomes the `yocto` flavour
instead. A Debian base ships older packages than any of these. Exact parity therefore exists only
in the `yocto` flavour, which carries the SDK BitBake produced, and only for target builds.

## CMake minimum

`cmake_minimum_required(VERSION 4.3)` equals the baseline's CMake so that the declared minimum, the
container, BitBake and the SDK agree on one number. A lower minimum would promise compatibility
with a version nothing we run ever exercises. Distribution packages lag, so outside the container
CMake comes from `pip install cmake==4.3.1` or the tarball; the plain-distribution CI leg does
exactly that. The library installs a CMake package config and no pkg-config file; one appears when
a non-CMake consumer needs it.

## C++23 and `std::expected`

The standard is C++23 in full: with GCC 15 as the only toolchain nothing in it needs avoiding, and
C++26 waits until compiler support stops being experimental.

First-party library code returns `std::expected<T, Error>` for recoverable failures and never
throws; the vendored thread pool is exempt and keeps upstream's future-based interface. Contract
violations are `assert`-guarded and covered by death tests, and an empty `catch (...)` is banned:
the vendored thread pool's catch-alls are the case in point, since a library that swallows
everything is the failure mode this rule exists to prevent. The cost is small but not zero: an
`expected` carries a discriminant plus padding and one well-predicted branch per check. It never
allocates and small trivially copyable payloads return in registers. Exceptions are cheaper on the
happy path and far more expensive on the error path; for shallow utility APIs the difference is
noise, and error payloads are enums so they stay small. Whether an application throws is the
application template's call, not this one's.

## Dependencies through CPM, resolved locally

Every dependency is declared with `CPMAddPackage` from a vendored, SHA256-pinned `CPM.cmake`, with
`CPM_USE_LOCAL_PACKAGES=ON` and `CPM_LOCAL_PACKAGES_ONLY=ON` as the default: each declaration
resolves through `find_package` against the baseline, which is the pinned container image for host
builds and the Yocto sysroot for target builds, never whatever distribution a developer happens to
run, and errors rather than downloads. That is what BitBake demands anyway, since nothing after
`do_fetch` has network access. `CPM_LOCAL_PACKAGES_ONLY=OFF` opens fetching for work beyond the
baseline, and `CPM_<NAME>_SOURCE=/path` points a dependency at a local checkout. We chose CPM over
`find_package` plus `FetchContent` because the application template needs third-party fetching,
and one mechanism in both templates beats a slightly smaller one here.

CPM has three warts worth knowing. `OPTIONS` are silently ignored when a package resolves locally,
so a dependency that needs non-default options is baked into the baseline image or forced to source
with `CPM_DOWNLOAD_<NAME>`. `VERSION` is a `find_package` minimum, not a pin, so the real pin is the
container tag and the declared version is the floor the code needs. The local lookup is quiet, so
read the configure log to learn what resolved.

## OpenSSL as the worked example

The Bloom filter hashes with OpenSSL on purpose: it is a real system dependency present in every
Yocto sysroot, which makes the demonstration honest where a toy dependency would not. The pattern is
`find_package(OpenSSL REQUIRED)`, `PRIVATE OpenSSL::Crypto` for the one component used, and the EVP
API with SHA-256 in place of the deprecated `MD5()`. The rule a fork must keep is that no OpenSSL
header appears in any installed header: the digest size is a plain `constexpr` and the
implementation lives in the `.cpp`, so compiling against `util` never needs OpenSSL's headers.

## The vendored thread pool

The thread pool is third-party code and it stays, because it shows how to vendor third-party code
properly: the upstream MIT notice restored verbatim in the header, an entry in
`THIRD_PARTY_NOTICES.md` with the upstream URL, version, checksum and local modifications, and an
exemption from the house naming style so the file stays close to upstream for future syncs.

## Testing

Tests come in four tiers selected by CTest label from one build: `unit` (GoogleTest, every pull
request), `functional` (the installed package consumed by a separate project), `integration` (the
application template's) and `performance` (weekly and at release).

There is one unit-test binary: `gtest_discover_tests` registers every case with CTest individually,
so selecting a module is `ctest -R bloom` regardless of how files are linked, and per-source
binaries would only add a link step and a CI touchpoint each. Every source file and every public
header has a test file of the same name, enforced at configure time, because a missing file is the
cheapest missing test to detect. A name check cannot see whether the file tests anything, so a
per-file coverage floor from gcovr's JSON backstops it.

## Coverage

Coverage is gcov plus gcovr, driven by a few dozen project-owned lines rather than a vendored
coverage module. One aggregate run produces Cobertura, HTML, a Markdown summary and the
`--fail-under-line` gate, with the per-file floor on top. There is no Codecov account or badge token
by default, only a commented-out step for a fork that wants one; a clang-only fork uses llvm-cov.

## Documentation as a gate

Doxygen runs through `doxygen_add_docs()` with `WARN_AS_ERROR`, so an undocumented public symbol
fails post-merge CI, and the site, with the coverage HTML, deploys to GitHub Pages from the
post-merge run. Doc comments state intent and contracts; inline comments are for the non-obvious.

## Performance and profiling

Google Benchmark binaries live under `bench/` behind an option, Release only, and run weekly for the
trend dashboard and at release against the previous baseline, where a regression can block the
release. Pull requests run no benchmarks and there is no per-commit gate, because single runs on
shared virtual machines are noise where a trend over weeks is not; a `perf` label opts one pull
request into a run. Tracy never appears in library sources: a profiling-enabled library build would
be a second shipped artifact carrying a dependency, and we do not want that variant to exist. Zones
live in the `performance` tier's own harnesses around the library calls, and Tracy's sampling mode
sees function-level detail inside the library from debug symbols alone.

## Naming, warnings and local checks

The project and namespace are `util`; a template's example name should be forgettable. House style
is camelCase, encoded in clang-tidy's identifier-naming checks: `UpperCamelCase` types, `camelBack`
functions, `camelBack_` members and `kUpperCamel` constants. Vendored code keeps upstream style.
Warning flags live on a `util_warnings` INTERFACE target linked `PRIVATE`, so they never reach a
consumer's compile line, and `-Werror` is on in CI and off in the `dev` preset: a developer's build
should not stop dead on a warning from a newer compiler, but a pull request should. pre-commit
hooks for format, tidy and commit messages are optional; CI is the gate, so nothing depends on
every developer installing them. The application template's Python is linted with ruff.

## Versioning, commits and releases

Versions are semver and the single source is the `project(VERSION ...)` line in `CMakeLists.txt`.
Commits follow Conventional Commits and squash-merge is the only merge method, so the pull-request
title is the only commit subject that matters and a title lint in the gate is the only commit
policing needed; issue references go in bodies and footers. release-please computes the bump from
those subjects and cuts each release from a rolling pull request; we chose it over hand-tagged
releases because the version line, the changelog and the tag then come from one commit and cannot
disagree. The file is the source rather than the tag because reading `git describe` at configure
time breaks under Yocto and in tarball builds. A Yocto recipe pins `v${PV}`; git-cliff is the
escape hatch if the flow ever leaves GitHub.

## Hosting on GitHub and the CI cadence

GitLab does one thing better for a C++ repository: it ingests JUnit and Cobertura XML natively, so a
merge request shows per-test results and paints line coverage onto the diff with no setup. GitHub
renders neither; `mikepenz/action-junit-report` (`dorny/test-reporter` is the fallback) turns JUnit
XML into a check with inline annotations, gcovr writes a table into the job summary, and the
coverage HTML lives on GitHub Pages. In exchange GitHub gives a public repository unlimited Actions
minutes and free 4-vCPU arm64 hosted runners, and gives any repository the "Use this template"
button. A nicer coverage diff did not outweigh a real ARM correctness leg at no cost. That button
plus a rename of `util` is all it takes to start a project, so there is no cookiecutter or copier
generator to keep in step with the code, and no umbrella repository: each README names its siblings.

The pull-request gate asks "did I break anything" in under fifteen minutes: GCC and clang in Debug
and Release, unit and functional tests, format and tidy checks, ASan and UBSan, coverage thresholds.
Post-merge runs the full suite on every push to `main`: the complete matrix with the arm64 leg,
TSan, CodeQL and the Doxygen gate. A weekly run repeats it with no code change and opens an issue
on failure, because a scheduled run nobody watches silently rots. A release tag adds a benchmark
comparison against the previous release.

We run the full suite per merge rather than nightly because public-repository minutes are free and
per-merge makes failures self-attributing: a gate failure points at the pull request, a post-merge
failure at the one merge that triggered it, a weekly failure with no new commits at the environment.

## BitBake, tests and hardware

`meta-cxx-cmake` is the consumer-side proof: a recipe for `util` built by BitBake against the
baseline, with packaging QA as the acceptance test for the install layout. The build is composed
from `bitbake`, `openembedded-core`, `meta-yocto` and `meta-openembedded` at the 6.0.2 commits
pinned in a kas lock file; the combined `poky` repository is retired. Full BitBake builds run on a
self-hosted runner on a workstation, dispatch- and push-only and never for fork pull requests: a
six-hour job cap and a ten-gigabyte cache against a multi-gigabyte sstate make hosted BitBake
impractical, and we have not measured how long a single recipe takes on a hosted runner.

Nothing in cloud CI runs under emulation. ARM correctness comes from GitHub's arm64 runners, real
machines running the unit and functional tests natively in the container, and the Yocto build
compiles, packages and runs QA with no execution step, since an emulated pass proves little about
the target and an emulated benchmark proves nothing. The recipe packages ptest; it runs on a board.

## Licensing

Everything is MIT, with REUSE metadata: SPDX headers on first-party source, `LICENSES/MIT.txt`,
`REUSE.toml` for files that cannot carry a header, and `reuse lint` in CI. Vendored files keep their
upstream notices and are listed in `THIRD_PARTY_NOTICES.md`. The container images carry an
`org.opencontainers.image.licenses=MIT` label that covers the repository content; the image itself
holds GPL and LGPL toolchain packages under their own licences, and its README says so.
