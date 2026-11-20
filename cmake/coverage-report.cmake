# SPDX-FileCopyrightText: 2022-2026 Ryan Sherlock
# SPDX-License-Identifier: MIT

# Script mode (cmake -P), run by the coverage target:
#   1. delete the .gcda files from any earlier run,
#   2. run the unit tests,
#   3. run gcovr once for Cobertura XML, HTML, JSON and a JSON summary,
#   4. write a Markdown summary from the JSON summary (gcovr 7 has no
#      Markdown output of its own).
# Inputs: SOURCE_DIR, BINARY_DIR, GCOVR, CTEST.

foreach(var SOURCE_DIR BINARY_DIR GCOVR CTEST)
  if(NOT DEFINED ${var})
    message(FATAL_ERROR "coverage-report.cmake needs -D${var}=...")
  endif()
endforeach()

set(out "${BINARY_DIR}/coverage")
file(REMOVE_RECURSE "${out}")
file(MAKE_DIRECTORY "${out}/html")

file(GLOB_RECURSE stale "${BINARY_DIR}/*.gcda")
if(stale)
  file(REMOVE ${stale})
endif()

execute_process(
  COMMAND "${CTEST}" --test-dir "${BINARY_DIR}" -L unit --output-on-failure
  COMMAND_ERROR_IS_FATAL ANY)

# Library code only: src/ and the public headers, which includes the vendored
# thread pool because the unit tests exercise it.
execute_process(
  COMMAND "${GCOVR}"
    --root "${SOURCE_DIR}"
    --object-directory "${BINARY_DIR}"
    --filter "${SOURCE_DIR}/src/"
    --filter "${SOURCE_DIR}/inc/"
    --exclude-unreachable-branches
    --exclude-throw-branches
    --print-summary
    --cobertura "${out}/cobertura.xml" --cobertura-pretty
    --html-details "${out}/html/index.html" --html-title "util coverage"
    --json "${out}/coverage.json"
    --json-summary "${out}/summary.json" --json-summary-pretty
  COMMAND_ERROR_IS_FATAL ANY)

file(READ "${out}/summary.json" summary)

# One decimal place from the integer counts: CMake reads JSON numbers back
# as doubles (87.1 comes out as 87.099999999999994) and has no float maths.
function(percent json kind var)
  string(JSON covered GET "${json}" "${kind}_covered")
  string(JSON total GET "${json}" "${kind}_total")
  if(total EQUAL 0)
    set(${var} "n/a" PARENT_SCOPE)
    return()
  endif()
  math(EXPR tenths "(${covered} * 1000 + ${total} / 2) / ${total}")
  math(EXPR whole "${tenths} / 10")
  math(EXPR frac "${tenths} % 10")
  set(${var} "${whole}.${frac}%" PARENT_SCOPE)
endfunction()

set(md "## Coverage\n\n| File | Lines | Functions | Branches |\n|---|---:|---:|---:|\n")
string(JSON count LENGTH "${summary}" files)
math(EXPR last "${count} - 1")
foreach(i RANGE ${last})
  string(JSON file GET "${summary}" files ${i})
  string(JSON name GET "${file}" filename)
  percent("${file}" line lines)
  percent("${file}" function functions)
  percent("${file}" branch branches)
  string(APPEND md "| `${name}` | ${lines} | ${functions} | ${branches} |\n")
endforeach()
percent("${summary}" line lines)
percent("${summary}" function functions)
percent("${summary}" branch branches)
string(APPEND md "| **Total** | **${lines}** | **${functions}** | **${branches}** |\n")
file(WRITE "${out}/summary.md" "${md}")

message(STATUS "Coverage reports in ${out}: cobertura.xml, html/index.html, summary.md")
