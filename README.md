# cxx-cmake-library

A template for C++ libraries built with CMake: C++23, CMake 4.3, dependencies
declared through CPM but resolved from the build environment, GoogleTest unit and
functional tests, gcov coverage with per-file floors, sanitizer presets, Doxygen
docs that fail on warnings, and Google Benchmark. The example library is `util`
(a Bloom filter, a vendored thread pool and a header-only `bit` module); replace
it with your own and keep the rest.

It is one of four repositories:

- [cxx-cmake-container](https://github.com/SherlockInSpace/cxx-cmake-container)
  builds the `ci`, `dev` and `yocto` images every build runs in;
- this repository, the library template;
- [cxx-cmake-app](https://github.com/SherlockInSpace/cxx-cmake-app), the
  application template, which links the installed `util`;
- [meta-cxx-cmake](https://github.com/SherlockInSpace/meta-cxx-cmake), the Yocto
  layer that packages both for the board.

[docs/DESIGN.md](docs/DESIGN.md) explains how the pieces fit together, and
[docs/DECISIONS.md](docs/DECISIONS.md) explains why they are the way they are.

## Quick start

Builds run inside the `dev` image, so every developer and CI job has the same
compiler, CMake and libraries. From the directory above your checkouts:

```sh
docker run --rm -it -e HOST_UID=$(id -u) -e HOST_GID=$(id -g) \
  -v "$PWD":/work -v cxx-cmake-ccache:/home/dev/.cache/ccache \
  ghcr.io/sherlockinspace/cxx-cmake-container/dev:0.1.1@sha256:0f1c38471816ff6b39bbc646aca13743a58da67a84e0cde18ab1e844495c3e58
```

`HOST_UID`/`HOST_GID` make the files you create under `/work` yours, and the
named volume keeps ccache's cache between containers. Then, in the container:

```sh
cd /work/cxx-cmake-library
cmake --workflow --preset dev
```

That configures, builds and runs every test. A healthy configure log looks like
this; CPM's lines say where each dependency came from:

```text
-- The CXX compiler identification is GNU 15.2.0
-- CPM: Using local package OpenSSL@3.5.5
-- CPM: Using local package GTest@1.17.0
-- Configuring done
-- Generating done
-- Build files have been written to: /work/cxx-cmake-library/build/dev
```

### Without the container

You need GCC 14 or newer (or Clang 22), CMake 4.3, Ninja, the OpenSSL 3
development files and GoogleTest 1.14 or newer. If `find_package` cannot see one
of them, point it there with `CMAKE_PREFIX_PATH` (a list of install prefixes) or
the package's own `<Name>_ROOT` variable, for example
`-DOPENSSL_ROOT_DIR=/opt/openssl`. Machine settings like these belong in a
`CMakeUserPresets.json`, which git ignores.

## Presets

Every build is a preset, in `build/<preset>`, with `compile_commands.json`.
`cmake --workflow --preset <name>` runs a preset's configure, build and test
steps; each can also be run on its own (`cmake --preset dev`,
`cmake --build --preset dev`, `ctest --preset dev`).

| Preset | What it is for |
|---|---|
| `dev` | Day-to-day Debug build. Warnings are not errors. |
| `release` | Release build. |
| `ci-debug`, `ci-release` | What CI builds: warnings are errors (`UTIL_WARNINGS_AS_ERRORS`), test results go to `junit.xml`. |
| `asan-ubsan` | AddressSanitizer and UndefinedBehaviorSanitizer on everything; the first report fails the test. |
| `tsan` | ThreadSanitizer, the same way. |
| `coverage` | GCC Debug build with gcov; its workflow also writes the coverage reports (see below). |
| `bench` | Release build of the benchmarks; its tests run them. |

Warning flags live on the `util_warnings` target, which `util` and its tests link
privately, so they never reach a consumer's compile line.

## Tests

Tests are selected by CTest label from one build:

| Label | What runs | When |
|---|---|---|
| `unit` | Every GoogleTest case in the one `util_unit_tests` binary. | Every pull request. |
| `functional` | Installs `util` into a scratch prefix, then configures, builds and runs `test/functional/consumer` against it with `find_package(util CONFIG REQUIRED)`. | Every pull request. |
| `performance` | The benchmarks, one test per `bench/src` file (needs `BUILD_BENCH`). | Weekly and at release. |

`ctest --preset dev -L unit` runs one tier and `ctest --preset dev -R bloom`
one module. `-DBUILD_TESTING=OFF` builds only the library.

Every file in `src/` and every public header in `inc/util/` needs a test file of
the same name in `test/unit/src/`; configure stops and says so when one is
missing. A header with nothing to test on its own goes in `UTIL_TEST_EXEMPT` in
`test/unit/CMakeLists.txt`, with a comment saying why.

## Dependencies

Every dependency is a `CPMAddPackage` call. By default
(`CPM_USE_LOCAL_PACKAGES=ON`, `CPM_LOCAL_PACKAGES_ONLY=ON`) each one resolves
through `find_package`, from the container image or the Yocto sysroot, and
configure fails when one is missing; nothing is downloaded.

| To | Pass |
|---|---|
| let CPM download what `find_package` cannot find | `-DCPM_LOCAL_PACKAGES_ONLY=OFF` |
| build one dependency from source even though a local copy exists | `-DCPM_LOCAL_PACKAGES_ONLY=OFF -DCPM_DOWNLOAD_<Name>=ON` |
| build one dependency from your own checkout | `-DCPM_<Name>_SOURCE=/path/to/checkout` |
| keep downloaded sources between build directories | `CPM_SOURCE_CACHE=$HOME/.cache/CPM` (environment or `-D`) |
| point `find_package` somewhere else | `CMAKE_PREFIX_PATH` or `<Name>_ROOT` |

A download is always a release tarball checked against the SHA-256 in its
declaration. OpenSSL is not a CMake project, so it can only come from the system.

Things CPM does that are easy to trip over (more in
[docs/DECISIONS.md](docs/DECISIONS.md)):

- `OPTIONS` are ignored when a package resolves locally, so a dependency that
  needs non-default build options has to be built that way in the image.
- `VERSION` is the minimum the code needs, not a pin. The pin is the container
  image: a dependency's real version changes when the image does.
- Under `CPM_LOCAL_PACKAGES_ONLY` CPM only reports a missing package and carries
  on to download it; this template turns FetchContent fully disconnected in that
  mode, so it cannot.
- The local lookup is quiet. Read the configure log to see what resolved.

To add a dependency: add a `CPMAddPackage` with the minimum `VERSION`, a release
`URL` and its `URL_HASH`; if it appears in a public header or the exported link
interface, add a matching `find_dependency()` to `cmake/utilConfig.cmake.in`; and
add it to the container image so local-only builds can find it.

## Installing and using the library

```sh
cmake --preset release -DCMAKE_INSTALL_PREFIX=/opt/util
cmake --build --preset release
cmake --install build/release
```

installs:

```text
<prefix>/include/util/*.hpp
<prefix>/lib/libutil.so -> libutil.so.0 -> libutil.so.0.1.0
<prefix>/lib/cmake/util/utilConfig.cmake, utilConfigVersion.cmake, utilTargets*.cmake
```

`-DBUILD_SHARED_LIBS=OFF` builds `libutil.a` instead. A consumer needs two lines:

```cmake
find_package(util 0.1 CONFIG REQUIRED)
target_link_libraries(app PRIVATE util::util)
```

Before 1.0 only the same minor version is compatible; at 1.0 the version file
switches to the same major version.

## Coverage

```sh
cmake --workflow --preset coverage
```

configures a GCC Debug build with `UTIL_COVERAGE=ON`, builds it, then builds the
`coverage` target. That target deletes old `.gcda` files, runs the unit tests and
runs gcovr once over `src/` and `inc/`, writing `build/coverage/coverage/`:
`cobertura.xml` for CI annotations, `html/index.html` to browse, `coverage.json`
and `summary.json`, and `summary.md`, a Markdown table built from the JSON summary
because gcovr 7 has no Markdown output. Only `util` and its unit tests are
instrumented, never anything that links `util`.

The target fails when total line coverage is under
`UTIL_COVERAGE_FAIL_UNDER_LINE` (90%) or any single file is under
`UTIL_COVERAGE_FILE_FLOOR` (80%). The per-file floor is what catches a test file
that exists but tests nothing, which the configure-time test-file check cannot
see. The vendored thread pool's coverage moves by several points between runs
because work stealing depends on timing, which is why the per-file floor sits
below the total.

`UTIL_COVERAGE` needs GCC. A fork that builds with Clang should use its
source-based coverage instead: compile and link with
`-fprofile-instr-generate -fcoverage-mapping`, run the tests with
`LLVM_PROFILE_FILE=%p.profraw`, then `llvm-profdata merge` and `llvm-cov export
-format=lcov` (or `llvm-cov show -format=html`).

To publish to Codecov, add a step like this after the coverage step in CI and
add the `CODECOV_TOKEN` secret:

```yaml
# - uses: codecov/codecov-action@v5
#   with:
#     files: build/coverage/coverage/cobertura.xml
#     token: ${{ secrets.CODECOV_TOKEN }}
```

## Benchmarks

```sh
cmake --workflow --preset bench
```

builds `bench/` with Google Benchmark in Release and runs it. Each
`bench/src/<name>.cpp` names its benchmarks `<name>/...`, is one CTest test
labelled `performance`, and writes Google Benchmark JSON to
`build/bench/bench/results/<name>.json`. `BUILD_BENCH` refuses Debug and coverage
builds, which time the wrong thing. Benchmarks run weekly and at release rather
than per commit, because a single run on a shared machine is noisy; read the trend.

## Documentation

`-DBUILD_DOCS=ON` adds a `docs` target that runs Doxygen over the public headers,
this README and `docs/`, writing HTML to `build/<preset>/docs/html`:

```sh
cmake --preset dev -DBUILD_DOCS=ON && cmake --build --preset dev --target docs
```

Any Doxygen warning fails the target, so an undocumented public symbol or a stale
`@param` fails the docs job. The container image ships the doxygen-awesome-css
theme, which the build picks up from `/usr/share/doxygen-awesome-css`; elsewhere
the docs use Doxygen's own theme.

## Formatting and static analysis

`.clang-format` owns layout and `.clang-tidy` owns diagnostics and the camelCase
naming rules. Both apply to first-party code only; vendored files keep their
upstream style.

```sh
clang-format --dry-run -Werror $(git ls-files 'src/*.cpp' 'inc/util/bit.hpp' 'inc/util/bloom.hpp' 'test/*.cpp' 'bench/*.cpp')
run-clang-tidy -p build/dev 'src/|test/unit/|bench/'
```

## Changes that span repositories

A change that needs something new from another repository lands in dependency
order, each step released before the next starts:

1. the dependency first: a new package or tool goes into
   `cxx-cmake-container`, which releases a new image version;
2. this repository moves to that image: bump the image tag and digest (see the
   manual pins below) in one pull request, with the code that needs it;
3. downstream follows: `cxx-cmake-app` bumps its `util` version, and
   `meta-cxx-cmake` bumps the recipe's `SRCREV` to the new tag.

To try a change before the dependency is released, build against a checkout
instead: `-DCPM_<Name>_SOURCE=/work/<checkout>` here, or `EXTERNALSRC` in the
Yocto layer.

## Vendoring

Prefer a dependency the image provides. When you must copy code in, record where
it came from at the time you copy it, in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md): the upstream URL, the exact
tag or commit, a checksum for whole files, the licence and copyright, and every
local change. The thread pool shows why: it was copied without a version marker,
and its base had to be reconstructed later by diffing it against every upstream
tag. Vendored files keep their upstream style and notice and are left out of
formatting and naming checks.

## Manual pins

Dependabot keeps the GitHub Actions versions current. Two pins it cannot see are
bumped by hand:

- **The container image** (`ghcr.io/sherlockinspace/cxx-cmake-container/{ci,dev}`),
  referenced by tag and digest in the workflows and the devcontainer. Take the new
  version and its digests from the container repository's release notes and
  change every reference in one pull request.
- **`cmake/CPM.cmake`**, the vendored CPM release. Replace it with the newer
  release's `CPM.cmake` asset, check its SHA-256 against the release, and update
  the version and hash in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).

## Using this template

Create a repository from this template, then make it yours. The example name
`util` is deliberately forgettable; replace it everywhere.

Rename checklist, with `mylib` standing for your name:

- [ ] `project("util" ...)` in `CMakeLists.txt`, and its `DESCRIPTION` and
      `HOMEPAGE_URL`;
- [ ] the `util` namespace in every header and source;
- [ ] `inc/util/` to `inc/mylib/`, and every `#include <util/...>`;
- [ ] `cmake/utilConfig.cmake.in` to `cmake/mylibConfig.cmake.in`;
- [ ] the `UTIL_` options and helper targets: `UTIL_TEST_EXEMPT`,
      `UTIL_COVERAGE*`, `UTIL_WARNINGS_AS_ERRORS`, `UTIL_INSTALL_CMAKEDIR`,
      `util_warnings`, `util_unit_tests`, `util_bench`;
- [ ] `util::util` in `test/` and `test/functional/consumer/`;
- [ ] `HeaderFilterRegex` in `.clang-tidy`;
- [ ] the copyright lines in `LICENSE`, `REUSE.toml` and the SPDX headers;
- [ ] this README and `docs/` where they name `util` or this repository.

Delete the example code and keep the machinery:

| Delete | Keep |
|---|---|
| `src/bloom.cpp`, `inc/util/bit.hpp`, `inc/util/bloom.hpp` | `CMakeLists.txt`, `CMakePresets.json`, `cmake/` |
| `test/unit/src/*.cpp` (write your own, one per source and header) | `test/CMakeLists.txt`, `test/unit/CMakeLists.txt` |
| `bench/src/*.cpp` (write your own) | `bench/CMakeLists.txt` |
| `inc/util/thread_pool.hpp` and its `THIRD_PARTY_NOTICES.md` entry, unless you use it | `test/functional/` (update `consumer/main.cpp` to use your API) |
| the OpenSSL `CPMAddPackage`, and its `find_dependency` in the config template, if you do not need it | `.clang-format`, `.clang-tidy`, `.editorconfig`, `REUSE.toml`, `LICENSES/` |
| | `docs/DECISIONS.md` and `docs/DESIGN.md`: edit them to say what your project decided |

Then reset the version in `project()` to `0.1.0` and run
`cmake --workflow --preset ci-debug` and `cmake --workflow --preset coverage`
before your first commit.
