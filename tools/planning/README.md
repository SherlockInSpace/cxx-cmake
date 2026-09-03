# Planning tooling (decision D17)

`plan.py` turns the family backlog into GitHub state — repositories, settings, labels,
milestones, issues, dependency links, the roadmap Project — and keeps it in sync. Any
agent or person can re-run it: on an up-to-date repo every subcommand is a no-op.

Requirements: Python 3.11+, the `gh` CLI logged in as the repository owner (`gh auth
status`). Run from the repository root:

```sh
python3 tools/planning/plan.py check
```

## Files

| File | Role |
| --- | --- |
| `plan.py` | the tool; one subcommand per step |
| `backlog.json` | **committed source of truth** for every subcommand except `export`: one entry per backlog item (id, repo, title, size, phase, blockers, done-when) |
| `issue-map.json` | cache: backlog ID → issue URL. Rebuilt from GitHub by `issues` (or by any subcommand when the file is absent); `deps`, `close-ops` and `project` trust the committed copy |
| `.gitignore` | keeps `.body.md` (scratch issue body), `*.log` and `__pycache__/` out of the repo |

`BACKLOG.md` — the hand-edited, issue-level backlog — is **maintainer-private and not in
this repository**. Only `export` reads it; the maintainer runs `export` from wherever the
backlog lives and commits the resulting `backlog.json`. `check`, `issues`, `deps` and the
rest consume `backlog.json` only, so they work from a plain clone.

## Subcommands, in order

| Step | Subcommand | What it does | Idempotency |
| --- | --- | --- | --- |
| 0 | `export --backlog PATH` | parse `BACKLOG.md` into `backlog.json` (default `PATH` = `./BACKLOG.md`); maintainer only | pure function of the input |
| 1 | `check` | validate `backlog.json`: unknown references, cycles, per-repo counts; no GitHub calls | read-only |
| 2 | `repos` | create the Yocto layer repo if missing; set every repo public, squash-only merges, delete-branch-on-merge, description, the "protect integration branches" ruleset | settings are re-applied (same values); the ruleset is created only if absent |
| 3 | `labels` | ensure the shared label set in every repo | `gh label create --force` |
| 4 | `milestones` | ensure the phase milestones each repo needs | created only if the title is absent |
| 5 | `issues` | file every backlog item as an issue with labels and milestone | marker-based, see below; `--dry-run` reports what would be created |
| 6 | `deps` | add native *blocked by* links from the backlog's After column and rewrite the **whole body** of every issue that has blockers from `backlog.json` | adding an existing link is a no-op; bodies are regenerated to the same text — hand edits to those bodies are overwritten |
| 7 | `close-ops` | close the per-repo `ops: repository settings` issues with a summary comment | already-closed issues are skipped |
| 8 | `project` | add every issue to the user-level Project **cxx-cmake roadmap** | items already in the Project are skipped |
| — | `all` | steps 2–7 in order | as above |

`export`, `check` and `project` are never part of `all`: `export` needs the private file,
`check` is a gate you run and read, and `project` needs an extra OAuth scope (below).

### Marker-based idempotency

Every issue body starts with `<!-- backlog:ID -->`. `issues` begins by listing all issues
(open and closed) in every repo and reading that marker, which rebuilds `issue-map.json`
(`--dry-run` lists but leaves the file untouched); an item whose marker is found anywhere
is never created again. `deps`, `close-ops` and `project` do **not** list GitHub: they trust
the committed map and rebuild it only when the file is absent. So GitHub is the source of
truth and the map is a cache, but a stale cache is only refreshed by `issues` — after
renumbering, closing or moving an issue, run `issues` before `deps`, `close-ops` or
`project`. A partial run can be resumed by running the same subcommand again. Do not
remove the marker when editing an issue by hand.

### `project` and the OAuth scope

`gh` tokens do not carry the `project` scope by default. Once, interactively:

```sh
gh auth refresh -s project
python3 tools/planning/plan.py project
```

The Project is user-level (`--owner @me`); it is created if the title is not found.

## Conventions the issues encode

- One squash-merged PR closes one issue; the PR title is the issue title (Conventional
  Commits, lint-enforced). Mechanical diffs stay in their own PR.
- `repos` sets the squash-merge commit format: **title = PR title, message = the PR's
  commit messages** (`squash_merge_commit_title=PR_TITLE`,
  `squash_merge_commit_message=COMMIT_MESSAGES`) — so the PR body is never the commit
  message, and commits inside a PR should be reviewable on their own.
- Native *blocked by* links come from the backlog's After column; milestones are the
  roadmap phases; labels are the shared `area:*` set plus `ops`, `mechanical`,
  `decision-needed`.
- `ops` items are settings checklists applied by this tool and closed with a comment; no PR.

## Editing the backlog

1. Edit `BACKLOG.md` (maintainer).
2. `python3 tools/planning/plan.py export --backlog /path/to/BACKLOG.md`
3. `python3 tools/planning/plan.py check`
4. `python3 tools/planning/plan.py issues --dry-run`, then `issues`, `deps`, `project`.
5. Commit `backlog.json` (and the refreshed `issue-map.json`).

`issues` never touches an existing issue: titles, bodies, labels and milestones are set at
creation only. `deps`, however, regenerates the **entire body** of every issue that has
blockers (the "Blocked by" section needs the blockers' URLs, which do not exist until all
issues are filed) and applies it with `gh issue edit`, so a body edited by hand on GitHub
is overwritten on the next `deps` run. Body changes therefore go in `BACKLOG.md` and are
re-exported; titles, labels and milestones of existing issues are edited on GitHub by hand.
