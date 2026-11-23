# AGENTS.md

Instructions for coding agents working in this repository. People should read
[CONTRIBUTING.md](CONTRIBUTING.md); this is the short form of the same rules.

## Build and test

Always use the presets; never invent build directories or flags.

| Task | Command |
|---|---|
| Build and run every test | `cmake --workflow --preset dev` |
| What CI checks (warnings are errors) | `cmake --workflow --preset ci-debug` |
| One module's tests | `ctest --preset dev -R <module>` |
| Coverage with its floors | `cmake --workflow --preset coverage` |
| Sanitizers | `cmake --workflow --preset asan-ubsan`, `cmake --workflow --preset tsan` |
| API docs (warnings fail) | `cmake --preset dev -DBUILD_DOCS=ON && cmake --build --preset dev --target docs` |
| Benchmarks (Release only) | `cmake --workflow --preset bench` |
| Static analysis | `run-clang-tidy -p build/dev 'src/\|test/unit/\|bench/'` |
| Formatting | `clang-format -i <changed first-party files>` |

A change is done when `ci-debug`, `coverage` and the `docs` target all pass and
clang-format and clang-tidy report nothing.

## Rules

- Recoverable errors are `std::expected`; do not add `throw` to `src/` or `inc/`.
- Every file in `src/` and header in `inc/util/` needs `test/unit/src/<stem>.cpp`.
- Do not reformat or rename inside vendored files (`inc/util/thread_pool.hpp`,
  `cmake/CPM.cmake`); record any change to them in `THIRD_PARTY_NOTICES.md`.
- Dependencies are `CPMAddPackage` with a `URL_HASH`; do not add `FetchContent`,
  `ExternalProject` or anything that downloads at configure time.
- Warning flags go on `util_warnings`, never on the exported `util` target.
- New files get SPDX headers (see `REUSE.toml` for files that cannot).
- Commits are Conventional Commits with the issue in the body (`Closes #N` or
  `Refs #N`); each commit must build and pass on its own.
