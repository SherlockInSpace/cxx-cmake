window.BENCHMARK_DATA = {
  "lastUpdate": 1795454433365,
  "repoUrl": "https://github.com/SherlockInSpace/cxx-cmake-library",
  "entries": {
    "Benchmark": [
      {
        "commit": {
          "author": {
            "name": "Ryan Sherlock",
            "username": "SherlockInSpace",
            "email": "ryan.m.sherlock@gmail.com"
          },
          "committer": {
            "name": "Ryan Sherlock",
            "username": "SherlockInSpace",
            "email": "ryan.m.sherlock@gmail.com"
          },
          "id": "d14ee15d627409ff16467f784931040550bcc639",
          "message": "fix(ci): switch back explicitly after starting gh-pages\n\nAn orphan switch leaves no previous branch, so \"git switch -\" failed\nafter the dashboard job created gh-pages. The step records the branch it\nstarted on and checks it out again.\n\nCloses #131\n\nCo-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>\nClaude-Session: https://claude.ai/code/session_01D87VFpn27AWtTUboTpY7Zq",
          "timestamp": "2026-11-23T17:17:10Z",
          "url": "https://github.com/SherlockInSpace/cxx-cmake-library/commit/d14ee15d627409ff16467f784931040550bcc639"
        },
        "date": 1795454432625,
        "tool": "googlecpp",
        "benches": [
          {
            "name": "bloom/insert",
            "value": 245.4551053703182,
            "unit": "ns/iter",
            "extra": "iterations: 2846532\ncpu: 245.39775066642494 ns\nthreads: 1"
          },
          {
            "name": "bloom/contains/present:1",
            "value": 246.01298102667414,
            "unit": "ns/iter",
            "extra": "iterations: 2844151\ncpu: 245.98441222002634 ns\nthreads: 1"
          },
          {
            "name": "bloom/contains/present:0",
            "value": 241.964986885391,
            "unit": "ns/iter",
            "extra": "iterations: 2889146\ncpu: 241.92281629242686 ns\nthreads: 1"
          },
          {
            "name": "expected_vs_throw/expected_success",
            "value": 0.5195899332289559,
            "unit": "ns/iter",
            "extra": "iterations: 1348391528\ncpu: 0.5195217549601809 ns\nthreads: 1"
          },
          {
            "name": "expected_vs_throw/expected_failure",
            "value": 0.526029619321192,
            "unit": "ns/iter",
            "extra": "iterations: 1312041278\ncpu: 0.5259926181987089 ns\nthreads: 1"
          },
          {
            "name": "expected_vs_throw/throw_success",
            "value": 0.38897648959490116,
            "unit": "ns/iter",
            "extra": "iterations: 1797476001\ncpu: 0.38897030759299706 ns\nthreads: 1"
          },
          {
            "name": "expected_vs_throw/throw_failure",
            "value": 786.239921264263,
            "unit": "ns/iter",
            "extra": "iterations: 888033\ncpu: 786.1838028541728 ns\nthreads: 1"
          },
          {
            "name": "expected_vs_throw/bloom_create_refused",
            "value": 38.09688937373864,
            "unit": "ns/iter",
            "extra": "iterations: 18366173\ncpu: 38.09617643261881 ns\nthreads: 1"
          },
          {
            "name": "thread_pool/throughput/threads:1/real_time",
            "value": 9757367.044642251,
            "unit": "ns/iter",
            "extra": "iterations: 112\ncpu: 8889751.276785715 ns\nthreads: 1"
          },
          {
            "name": "thread_pool/throughput/threads:2/real_time",
            "value": 9122962.86666636,
            "unit": "ns/iter",
            "extra": "iterations: 75\ncpu: 8389325.786666669 ns\nthreads: 1"
          },
          {
            "name": "thread_pool/throughput/threads:4/real_time",
            "value": 13211040.735847985,
            "unit": "ns/iter",
            "extra": "iterations: 53\ncpu: 10781879.56603774 ns\nthreads: 1"
          },
          {
            "name": "thread_pool/throughput/threads:8/real_time",
            "value": 14400734.795917835,
            "unit": "ns/iter",
            "extra": "iterations: 49\ncpu: 10481818.122448979 ns\nthreads: 1"
          },
          {
            "name": "thread_pool/uneven_load/unit:1048576/real_time",
            "value": 10.095430157142411,
            "unit": "ms/iter",
            "extra": "iterations: 70\ncpu: 0.025199199999998662 ms\nthreads: 1"
          }
        ]
      }
    ]
  }
}