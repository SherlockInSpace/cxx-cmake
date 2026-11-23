# Contributing

This file describes how changes land in this repository and in any project
created from it. Rename `util` to your project's name as you read.

## From issue to merge

1. **Start from an issue.** Every change has one. Its title is a Conventional
   Commit subject (`fix(bloom): ...`, `build(cmake): ...`) and its body ends in a
   "Done when" list.
2. **Branch** as `issue/<number>-<short-slug>` from `main`.
3. **Commit** in Conventional Commit style. The subject is the issue title, the
   body says what changed and why, and the issue goes in the body:
   `Closes #N` when the commit finishes it, `Refs #N` when it does not. Every
   commit builds and passes the tests on its own, because pull requests are
   rebase-merged and each commit lands as it was reviewed.
4. **Keep pull requests small**: one issue, one to a few commits. Say in the
   description what you verified and how, and what you could not.
5. **CI is the gate.** A pull request merges when its checks are green; local
   hooks are optional and nothing depends on them.

Releases are cut from the commit subjects: `fix:` is a patch, `feat:` a minor,
`!` a major (until 1.0, a minor). Choose the type for what the change means to a
consumer of the library.

## Definition of done

Run these from the repository root before asking for review. CI runs the same
commands.

```sh
# Warnings are errors; unit and functional tests. Configure also checks that
# every source and public header has a test file.
cmake --workflow --preset ci-debug

# Coverage: fails under 90% of lines in total or 80% in any one file.
cmake --workflow --preset coverage

# API docs: any Doxygen warning fails the target.
cmake --preset dev -DBUILD_DOCS=ON && cmake --build --preset dev --target docs

# Layout and static analysis, first-party files only.
clang-format --dry-run -Werror $(git ls-files 'src/*.cpp' 'inc/util/bit.hpp' 'inc/util/bloom.hpp' 'test/*.cpp' 'bench/*.cpp')
run-clang-tidy -p build/ci-debug 'src/|test/unit/|bench/'
```

A change that touches threading also runs `cmake --workflow --preset tsan`, and
one that touches memory handling `cmake --workflow --preset asan-ubsan`.

## Code

- C++23. Recoverable failures are `std::expected<T, Error>`, not exceptions;
  nothing in `src/` or `inc/` throws, apart from vendored code.
- camelCase as `.clang-tidy` encodes it: `UpperCamelCase` types, `camelBack`
  functions and variables, `camelBack_` private members, `kUpperCamel`
  constants.
- Every public symbol has a doc comment that states its intent and contract.
  Inline comments are for what the code cannot say.
- A new source file or public header gets a test file of the same name in
  `test/unit/src/`. A header with nothing to test goes in `UTIL_TEST_EXEMPT`
  with a comment saying why.
- A new dependency is a `CPMAddPackage` with a minimum `VERSION`, a release
  `URL` and its `URL_HASH`, and goes into the container image too. Nothing may
  download at configure time by default.
- Vendored code keeps its upstream style and notice, and is recorded in
  `THIRD_PARTY_NOTICES.md` with its origin, version, checksum and every local
  change.
- Every new file carries the two SPDX lines (`SPDX-FileCopyrightText`,
  `SPDX-License-Identifier`), or an entry in `REUSE.toml` if it cannot.

## Where things are

[README.md](README.md) is the manual, [docs/DESIGN.md](docs/DESIGN.md) says how
the repositories fit together, and [docs/DECISIONS.md](docs/DECISIONS.md) says
why. To change one of the decisions, open an issue or a pull request that edits
that file.
