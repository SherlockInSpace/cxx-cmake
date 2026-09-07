# Decisions

Why the template is shaped the way it is, for whoever forks it and wonders whether a choice was
deliberate. To change one, open an issue or send a pull request that edits this file.

## One Yocto LTS baseline

The library targets one Yocto long-term-support release. For the moment, we've chosen Wrynose 6.0
(GCC 15.3, CMake 4.3.1, glibc 2.43, supported until April 2030). It's the latest LTS release and
the included toolchains are very close to those of Ubuntu 26.04, described below.

## Reproducible environments

We want to enable two developers on two different machines to reproduce the same build.
We believe that it is an important part of the development lifecycle that builds are reproducible
and that a developer on one machine isn't chasing a bug because their toolchain is different
from another developer.

The toolchain itself lives in the `cxx-cmake-container` repository. Every build of the library runs
inside an image and publishes to GHCR as `ci`, `dev` and `yocto` under one shared semver.
We never publish a `latest` image, which means an environment change is simply a one-line pull request.

Matching toolchains means version-level parity, not that the toolchains themselves are bit-for-bit
identical. While possible, we deem it overkill.
Ubuntu 26.04 is the newest LTS and the distribution has package version parity with Wrynose.
There are some differences though. No Ubuntu release ships CMake 4.3, so the image installs Kitware's prebuilt 4.3.1, verified by
checksum. Ubuntu GCC, binutils, and OpenSSL trail by a patch level which is documented in a table
in [DESIGN.md](DESIGN.md).
Ubuntu's compiler also defaults to PIE, `_FORTIFY_SOURCE=3` and control-flow protection, while
Yocto's is FSF-default with hardening from distro flags and its own patches.

The Yocto layer builds the library with BitBake on a self-hosted machine rather than in hosted CI:
hosted runners have a six-hour job cap and a ten-gigabyte cache, against a multi-gigabyte sstate.

## CMake

In order to minimize overhead in supporting tooling, we decided to go with the version of CMake
shipped with Wrynose since it's a fairly recent release.
Had we left Ubuntu with the default version that shipped, we would've had to work around features
that are supported in some versions and deprecated in others, increasing the overhead.
It is a simpler choice to ship Kitware's CMake in the container.

## C++23 and `std::expected`

The standard is C++23 in full. At the time of this, C++26 is not fully ratified or ready in
all of the different compilers.

We've decided to adopt `std::expected<T, Error>` for recoverable failures instead of using exceptions.
The vendored thread pool is exempt and keeps upstream's future-based interface.

## Dependencies through CPM, resolved locally

We are using CPM as the dependency package manager in order to support dependencies against upstream
and local builds when developing.
We chose CPM over `find_package` plus `FetchContent` because the application template needs third-party fetching,
and a single mechanism doing this is better than managing two solutions.

Every dependency is declared with `CPMAddPackage` using a SHA256 in `CPM.cmake`, with
`CPM_USE_LOCAL_PACKAGES=ON` and `CPM_LOCAL_PACKAGES_ONLY=ON` as the default.
Each declaration resolves through `find_package` which uses the container image for host builds and
Yocto sysroot for target builds.
`CPM_LOCAL_PACKAGES_ONLY=OFF` opens fetching for builds and `CPM_<NAME>_SOURCE=/path` points a
dependency at a local checkout.

Be aware that CPM isn't perfect and there are some caveats worth noting:
- `OPTIONS` are silently ignored when a package resolves locally, so a dependency that needs non-default
  options is baked into the baseline image or forced to source with `CPM_DOWNLOAD_<NAME>`.
- `VERSION` is a `find_package` minimum, not a pin. The real pin is the container tag; the declared
  version is the minimum the code needs.
- The local lookup is quiet, so read the configure log to learn what resolved.

## Testing

Tests come in four tiers selected by CTest label from one build:
- `unit` (GoogleTest, every pull request)
- `functional` (the installed package consumed by a separate project)
- `integration` (the application template's)
- `performance` (weekly and at release).

There is one unit-test binary: `gtest_discover_tests` registers every case with CTest individually,
so selecting a module is `ctest -R bloom` regardless of how files are linked.
Every source file and every public header has a test file of the same name, which is enforced at
configure time. We chose this because a missing file is the cheapest test to detect.
However, a name check cannot see whether the file tests anything, so a per-file coverage test from gcovr's JSON backstops it.

Tests run natively: in CI on real x86-64 and arm64 machines, and on a board once one is attached.
Nothing runs under emulation.

## Coverage

Coverage is gcov plus gcovr. One run produces Cobertura, HTML, and a Markdown summary.

## Documentation as a gate

Doxygen runs through `doxygen_add_docs()` with `WARN_AS_ERROR`, so an undocumented public symbol
fails post-merge CI, and the site, with the coverage HTML, deploys to GitHub Pages from the
post-merge run. Doc comments state intent and contracts; inline comments are for the non-obvious.

## Performance and profiling

Google Benchmark binaries live under the directory `bench/`, and run weekly for the
trend dashboard and at release against the previous baseline.
A regression can block a release.
We chose not to benchmark per commit because single runs on shared CI machines are noisy.
It's better to look at the trend over longer periods of time.

We also use Tracy for intensive scrutiny and profiling. It's also an excellent tool that works both
on x86 and ARM.
Tracy never appears in library sources: a profiling-enabled library build would
be a second shipped artifact carrying a dependency, and we do not want that variant to exist.
Zones live in the `performance` tier's own harnesses around the library calls, and Tracy's sampling mode
sees function-level detail inside the library from debug symbols alone.

## Naming, warnings and local checks

The project and namespace are `util`; a template's example name should be forgettable. Code style
is camelCase, encoded in clang-tidy's identifier-naming checks: `UpperCamelCase` types, `camelBack`
functions, `camelBack_` members and `kUpperCamel` constants. Vendored code keeps upstream style.
Warning flags live on a `util_warnings` INTERFACE target linked `PRIVATE`, so they never reach a
consumer's compile line, and `-Werror` is on in CI and off in the `dev` preset.
A developer's build should not stop dead on a warning from a newer compiler, but a pull request should.
`pre-commit` hooks for format, tidy and commit messages are optional; CI is the gate, so nothing depends on
every developer installing them. The application template's Python is linted with ruff.

## Versioning, commits and releases

Versions are semver and the single source is the `project(VERSION ...)` line in `CMakeLists.txt`.
The version lives in that file rather than being read from the tag because `git describe` at
configure time breaks under Yocto and in tarball builds.
We've decided to use the Conventional Commit style and squash-merging. There are advantages to
rebasing but for now we've decided squash-merge.
Titles are lint checked and issues references go in the body and footer of the commit messages.
`release-please` computes the bump from those subjects and cuts a release from a rolling pull request.
We chose it over hand-tagged releases because the version line, the changelog and the tag then come
from one commit.

## Licensing

Everything is MIT, with REUSE metadata: SPDX headers on first-party source, `LICENSES/MIT.txt`,
`REUSE.toml` for files that cannot carry a header, and `reuse lint` in CI. Vendored files keep their
upstream notices and are listed in `THIRD_PARTY_NOTICES.md`. The container images carry an
`org.opencontainers.image.licenses=MIT` label that covers the repository content; the image itself
holds GPL and LGPL toolchain packages under their own licences, and its README says so.
