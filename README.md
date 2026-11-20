# cxx-cmake
Template for building C++ projects/libraries/applications with CMake

> This README is being rewritten as the template manual; tracked in
> <https://github.com/SherlockInSpace/cxx-cmake/issues/39>.

## Building
Configure, build, and run the tests from the repository root with a preset:

```
cmake --preset dev && cmake --build --preset dev && ctest --preset dev
```

`cmake --workflow --preset dev` runs the same three steps. `dev` is a Debug
build and `release` a Release build; each lives in `build/<preset>` and writes
`compile_commands.json`. Machine-specific settings go in a
`CMakeUserPresets.json`, which git ignores.

Tests are picked by CTest label: `ctest --preset dev -L unit` runs the
GoogleTest cases, and `-L functional` installs `util` into a scratch prefix under
the build directory, then configures, builds and runs a separate project that
uses it through `find_package(util CONFIG REQUIRED)`. Without `-L` both run.

Warning flags live on the `util_warnings` target, which `util` and its tests
link privately, so they never reach a consumer. The `ci-debug` and `ci-release`
presets add `-Werror` (`UTIL_WARNINGS_AS_ERRORS=ON`) and write `junit.xml` into
their build directory; `dev` leaves warnings as warnings so a newer compiler
does not stop a local build.

`asan-ubsan` (AddressSanitizer with UndefinedBehaviorSanitizer) and `tsan`
(ThreadSanitizer) are Debug builds with the sanitizer on everything; their test
presets stop at the first report.

Tests build by default. Pass `-DBUILD_TESTING=OFF` at configure time to build
only the library.

Dependencies (OpenSSL, and GoogleTest for the tests) are declared through CPM
but resolve only through `find_package` by default, so configure fails rather
than download anything when one is missing. Pass `-DCPM_LOCAL_PACKAGES_ONLY=OFF`
to let CPM fetch what it cannot find locally (GoogleTest only; OpenSSL always
comes from the system), or `-DCPM_<Name>_SOURCE=/path/to/checkout` to build a
dependency from a local checkout.

To install the library, pass `-DCMAKE_INSTALL_PREFIX=<dir>` at configure time
and run `cmake --install build`.

## Coverage

```
cmake --workflow --preset coverage
```

configures a GCC Debug build with `UTIL_COVERAGE=ON`, builds it, then builds the
`coverage` target. That target deletes old `.gcda` files, runs the unit tests and
runs gcovr once over `src/` and `inc/`, writing `build/coverage/coverage/`:
`cobertura.xml` for CI annotations, `html/index.html` to browse, `coverage.json`
and `summary.json` for the per-file checks, and `summary.md`, a Markdown table
built from the JSON summary because gcovr 7 has no Markdown output. Only `util`
and its unit tests are instrumented, never anything that links `util`.

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

## Documentation

[docs/DESIGN.md](docs/DESIGN.md) explains how the pieces fit together; [docs/DECISIONS.md](docs/DECISIONS.md)
explains why they are the way they are.
