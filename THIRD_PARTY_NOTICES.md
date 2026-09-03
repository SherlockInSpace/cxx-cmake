# Third-party notices

This repository vendors (copies into the tree) the components listed below.
Each keeps its upstream licence and copyright notice, and each is exempt from
the house style: vendored files keep upstream formatting, naming and notices,
and must be excluded from any clang-format / clang-tidy enforcement introduced
later. Update this file whenever a vendored file is added, refreshed from
upstream, or removed.

The rest of the repository is licensed under the terms in [LICENSE](LICENSE).

## thread-pool

| | |
|---|---|
| File(s) | `inc/thread_pool.hpp` |
| Origin | <https://github.com/DeveloperPaul123/thread-pool> |
| Licence | MIT (full text reproduced at the top of `inc/thread_pool.hpp`) |
| Copyright | Copyright (c) 2021-2023 Paul Tsouchlos (copyright line taken from upstream `main` at time of writing; the `0.5.1` LICENSE reads 2021) |
| Derived from | tag `0.5.1` (commit `80b3c8b40e881a3b4f88599e873321a46d66f232`, 2022-10-20) |
| Upstream at time of writing (2026-09-03) | latest tag `0.7.0` (commit `2417b702026b56303486dc1442fed9658990185f`, 2025-01-01); `main` tip `0de593cd446ac1ee5428910f38f522c0549bbf26` (2026-03-05) |

`inc/thread_pool.hpp` combines two upstream headers,
`include/thread_pool/thread_pool.h` and `include/thread_pool/thread_safe_queue.h`.
It was vendored on 2022-12-02 (commit `a09e5c5`) and does not carry an upstream version marker, so
the base version was identified by diffing it against `thread_pool.h` at every
upstream tag: `0.5.1` is the closest match (83 differing lines, all accounted
for by the local modifications below), while `0.5.0` (170), `0.6.0` (120),
`0.6.1`/`0.6.2` (139) and `0.7.0` (246) differ substantially — the vendored
copy predates upstream's priority-queue scheduler (0.6.x) and `ThreadType`
template parameter (0.7.0).

Local modifications relative to upstream `0.5.1`:

- Namespace `dp` renamed to `util` (the closing-brace comment still reads
  `// namespace dp`).
- `thread_safe_queue.h` inlined: the `is_lockable` concept is placed in
  namespace `util`, `thread_safe_queue` becomes a private nested class of
  `thread_pool`, `#include "thread_pool/thread_safe_queue.h"` is replaced by
  `#include <optional>`, and `dp::thread_safe_queue` references drop the
  qualifier.
- In `enqueue`, the parameter and lambda-capture names `f` / `func` are
  swapped relative to upstream.
- Doxygen `@param` tags changed to `@param[in]`.
- The upstream `LICENSE` text is reproduced as a comment block above
  `#pragma once`, and the dangling `@example mandelbrot/source/main.cpp`
  doc block (an upstream example not present in this repository) is removed.

`src/thread_pool.cpp` and `test/unit/src/thread_pool.cpp` are local code,
not vendored.

## CodeCoverage.cmake

| | |
|---|---|
| File(s) | `cmake/CodeCoverage.cmake` |
| Origin | <https://github.com/bilke/cmake-modules> (Lars Bilke), `CodeCoverage.cmake` |
| Licence | BSD-3-Clause (full text in the file header) |
| Copyright | Copyright (c) 2012 - 2017, Lars Bilke. All rights reserved. |
| Derived from | upstream commit `877bab9dd1b17468c5d939cacaa2ad7ba99d1977` (2023-01-05); the last upstream change to the file before vendoring is `70a0b520`, the 2022-09-28 (Sebastian Mueller) `CHANGES` entry |

Local modifications relative to upstream `877bab9d` (first vendored in local
commit `85e613d`, 2023-06-14, byte-identical apart from the trailing newline):

- In `append_coverage_compiler_flags_to_target`, `target_compile_options(...)`
  and `target_link_libraries(... gcov)` changed from `PRIVATE` to `PUBLIC`
  (commits `ce63b4a`, `177692f`) -- the source of the known coverage-flag leak
  into consumers.
- `get_target_property(target_type ${PROJECT_NAME} TYPE)` added in the same
  function (`7cc40b7`).
- `OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU"` added to the gcov-link condition
  (`cfec72f`).
- `--xml-pretty` added to the gcovr XML command (`8b87109`).
- Trailing newline dropped.

This module is scheduled for removal (the coverage tooling is being replaced);
delete this entry together with the file.
