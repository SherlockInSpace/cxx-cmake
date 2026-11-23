# Profiling

The performance tier has two lanes. Benchmarks say whether something got
slower; Tracy shows where the time goes. Both are in `bench/`, outside the
library, and `util` itself builds the same way in every configuration.

## Benchmarks

`bench/src/` holds Google Benchmark code, one file per area, each a CTest test
labelled `performance`:

```sh
cmake --workflow --preset bench
```

builds them in Release, runs them one at a time and writes Google Benchmark
JSON to `build/bench/bench/results/`. CI runs them weekly onto the
[trend dashboard](https://sherlockinspace.github.io/cxx-cmake-library/bench/),
on pull requests labelled `perf`, and on every release pull request, which
fails if anything is more than 1.5 times slower than the last release (README,
"Benchmarks").

Shared CI runners are noisy, so one run says little. Read the dashboard for
a step that stays, and rerun a `perf` pull request before believing a single
number.

## Tracy

`bench/profile/` is a harness: it runs the Bloom filter and the thread pool
through a workload, with Tracy zones around each call into `util` and a frame
mark per round. It is the only target that links `Tracy::TracyClient`. The
library has no Tracy code, so its internals show up through Tracy's sampling
rather than zones of its own.

```sh
cmake --workflow --preset profile
```

builds the harness (RelWithDebInfo, so sampling has symbols) and runs it once.
To record a trace in the dev image, start `tracy-capture` and run the harness
with `TRACY_NO_EXIT=1`, which keeps it alive until the capture has everything:

```sh
tracy-capture -a 127.0.0.1 -o util.tracy -f &
TRACY_NO_EXIT=1 build/profile/bench/profile/util_profile
wait
```

Open `util.tracy` in the Tracy profiler of the same version as the image's
client (0.15.0). The weekly CI run attaches the same trace as its `tracy`
artifact.

Sampling uses Linux perf events. Docker's default seccomp profile blocks
them, so a trace from a normal container, CI's included, has the zones and
plots but no samples from inside `util`. For samples, run the dev container
with `--privileged`, or run the harness on the host as root or with
`kernel.perf_event_paranoid` set to -1.

To chase one hotspot inside the library, add temporary zones to `src/` on a
working branch and drop them before merging. Shipping a Tracy build of `util`
would mean a second library that depends on Tracy, and we don't want that
variant to exist ([DECISIONS.md](DECISIONS.md)).

## Power

Nothing measures power yet, because there is no board to measure. The tier is
shaped so it can: benchmarks are picked by CTest label (`ctest -L performance`)
and write JSON. A self-hosted runner on a board, wired to a power meter, can
run the same command under the meter's capture and turn each benchmark into an
entry such as `{"name": "bloom/insert", "unit": "J", "value": 0.0021}`, the
custom format `github-action-benchmark` reads, so energy goes onto the same
dashboard as time.
