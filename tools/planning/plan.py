#!/usr/bin/env python3
# SPDX-FileCopyrightText: 2026 Ryan Sherlock
# SPDX-License-Identifier: MIT
"""Planning tooling for the cxx-cmake family (decision D17).

Turns the issue-level backlog into GitHub state — repositories, settings, labels,
milestones, issues and their dependency links — idempotently, so that a person or an
agent can re-run it after editing the backlog and only the differences are applied.

Subcommands (run in this order the first time, or `all`):

  export      parse the maintainer's BACKLOG.md (--backlog PATH, default ./BACKLOG.md)
              into backlog.json, the committed data every other subcommand reads
  check       validate backlog.json: unknown references, cycles, counts (no GitHub calls)
  repos       create the layer repo if missing; visibility, merge settings, rulesets
  labels      create/update the shared label set in every repo
  milestones  create the phase milestones in every repo
  issues      create every backlog item as an issue (skips ones that already exist;
              --dry-run only reports what would be created)
  deps        wire native "blocked by" relationships from the After column
  close-ops   close the per-repo `ops: repository settings` issues with a summary
  project     add every issue to the user-level Project "cxx-cmake roadmap" (needs the
              `project` OAuth scope: `gh auth refresh -s project` — interactive)
  all         everything above except export/check/project (run those separately)

Only `export` touches BACKLOG.md; it is maintainer-private and not part of this repo.
State: issue-map.json (backlog ID -> issue URL) is a cache, not a source of truth. `issues`
rebuilds it from GitHub on every run by looking for the `<!-- backlog:ID -->` marker in
issue bodies; `deps`, `close-ops` and `project` trust the committed file (rebuilding only
if it is absent), so run `issues` first after any GitHub-side change.
"""
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import time
from pathlib import Path

HERE = Path(__file__).resolve().parent
BACKLOG_JSON = HERE / "backlog.json"
ISSUE_MAP = HERE / "issue-map.json"

OWNER = "SherlockInSpace"
REPOS = {  # backlog prefix -> repository name
    "C": "cxx-cmake-container",
    "Y": "meta-cxx-cmake",
    "L": "cxx-cmake",  # renamed to cxx-cmake-library at L-26; GitHub redirects
    "A": "cxx-cmake-app",
}
REPO_DESCRIPTIONS = {
    "C": "Versioned build-environment images for the cxx-cmake template family (GHCR, semver tags, never latest)",
    "Y": "Yocto layer for the cxx-cmake template family: recipes for the library and app templates, kas configs",
    "L": "C++ library template: CMake, CPM, GoogleTest, coverage, docs, CI — Yocto-friendly without Yocto lock-in",
    "A": "C++ application template consuming the cxx-cmake library template and third-party dependencies via CPM",
}
MILESTONES = {
    1: "Phase 1 — Foundations",
    2: "Phase 2 — Portability & CI cutover",
    3: "Phase 3 — Quality gates, docs, agents",
    4: "Phase 4 — Performance tier",
    5: "Phase 5 — App template & Yocto",
}
LABELS = {
    "area:cmake": ("1d76db", "Build system: CMake, presets, install/export contract"),
    "area:ci": ("0e8a16", "GitHub Actions workflows, runners, release automation"),
    "area:docker": ("fbca04", "Container images: Dockerfile, GHCR publishing"),
    "area:yocto": ("d93f0b", "Yocto layer: recipes, kas, BitBake proof"),
    "area:test": ("5319e7", "Test tiers, fixtures, test enforcement"),
    "area:docs": ("0075ca", "README, decision log, roadmap, Doxygen, AGENTS.md"),
    "area:quality": ("c5def5", "Formatting, naming, static analysis, sanitizers"),
    "area:perf": ("bfd4f2", "Benchmarks, profiling harnesses"),
    "area:license": ("e4e669", "Licensing, SPDX/REUSE, third-party notices"),
    "area:code": ("f9d0c4", "Library or application source changes"),
    "ops": ("ededed", "Settings/operations checklist — closed with a comment, no PR"),
    "mechanical": ("cccccc", "Pure-mechanical diff (format, rename, vendored file, deletion) — review differently"),
    "decision-needed": ("b60205", "Blocked on an owner decision"),
}
# Later-phase placeholders that live in prose in §2 rather than in a table.
PLACEHOLDERS = {
    "C-14": {"phase": 5, "after": ["C-10"]},
    "C-16": {"phase": 4, "after": ["C-10"]},
    "C-17": {"phase": 4, "after": ["C-10"]},
    "C-18": {"phase": 5, "after": ["C-10"]},
    "C-19": {"phase": 5, "after": ["C-10", "L-39"]},
    "C-20": {"phase": 5, "after": ["C-10", "Y-10"]},
}
Y_PHASES = {"Y-01": 1, "Y-02": 1, "Y-03": 1, "Y-04": 1, "Y-05": 1, "Y-06": 1,
            "Y-07": 2, "Y-08": 2, "Y-09": 3, "Y-10": 3, "Y-11": 5, "Y-12": 5}
CREATE_DELAY = 1.2  # seconds between content-creating calls (GitHub secondary limits)
EDIT_DELAY = 0.8


# ----------------------------------------------------------------------------- helpers
def gh(*args: str, check: bool = True, quiet: bool = False) -> str:
    cmd = ["gh", *args]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0 and check:
        raise RuntimeError(f"{' '.join(cmd)}\n{r.stderr.strip()}")
    if not quiet and r.stderr.strip() and r.returncode != 0:
        print(r.stderr.strip(), file=sys.stderr)
    return r.stdout.strip()


def gh_json(*args: str):
    out = gh(*args)
    return json.loads(out) if out else None


def repo_full(prefix: str) -> str:
    return f"{OWNER}/{REPOS[prefix]}"


def log(msg: str) -> None:
    print(msg, flush=True)


# ------------------------------------------------------------------------------ export
ID_RE = re.compile(r"\b([CYLA])-(\d\d)\b")


def split_row(line: str) -> list[str]:
    cells = re.split(r"(?<!\\)\|", line.strip())
    return [c.strip().replace("\\|", "|") for c in cells[1:-1]]


def expand_after(text: str) -> tuple[list[str], str]:
    """Return (ids, note) from an After cell. Ranges like L-01..L-25 are expanded;
    non-ID text (e.g. 'D15–D21 answered') is kept as a note."""
    text = text.replace("**", "")
    ids: list[str] = []
    for m in re.finditer(r"([CYLA])-(\d\d)\.\.\1-(\d\d)", text):
        p, a, b = m.group(1), int(m.group(2)), int(m.group(3))
        ids += [f"{p}-{i:02d}" for i in range(a, b + 1)]
    rest = re.sub(r"([CYLA])-(\d\d)\.\.\1-(\d\d)", "", text)
    ids += [f"{p}-{n}" for p, n in ID_RE.findall(rest)]
    note = re.sub(ID_RE, "", rest)
    note = re.sub(r"[,\s—–-]+", " ", note).strip()
    seen, out = set(), []
    for i in ids:
        if i not in seen:
            seen.add(i)
            out.append(i)
    return out, note


def export(backlog_md: Path) -> dict:
    if not backlog_md.is_file():
        raise SystemExit(f"export: {backlog_md} not found (pass --backlog PATH)")
    text = backlog_md.read_text(encoding="utf-8")
    items: dict[str, dict] = {}
    section = None
    l_phase = None
    for line in text.splitlines():
        if line.startswith("## 2."):
            section = "C"
        elif line.startswith("## 3."):
            section = "Y"
        elif line.startswith("## 4."):
            section = "L"
        elif line.startswith("## 5."):
            section = "A"
        elif line.startswith("## 6."):
            section = None
        m = re.match(r"### Phase (\d)", line)
        if m and section == "L":
            l_phase = int(m.group(1))
        if not (line.startswith("| ") and re.match(r"\| [CYLA]-\d\d \|", line)):
            continue
        cells = split_row(line)
        if len(cells) != 5:
            raise SystemExit(f"bad row (expected 5 cells): {line}")
        iid, title, size, after, done = cells
        prefix = iid[0]
        if prefix == "C":
            phase = 1
        elif prefix == "Y":
            phase = Y_PHASES[iid]
        elif prefix == "L":
            phase = l_phase
        else:
            phase = 2 if iid == "A-00" else 5
        after_ids, note = expand_after(after)
        items[iid] = {
            "id": iid, "repo": REPOS[prefix], "title": title, "size": size,
            "phase": phase, "after": after_ids, "after_note": note, "done_when": done,
            "ops": size == "ops", "placeholder": False,
        }
    # placeholders from the "Later container items" paragraph
    m = re.search(r"Later container items.*?(?=\n\n---)", text, re.S)
    if m:
        para = m.group(0)
        for pid, body in re.findall(r"\*\*(C-\d\d)\*\*(.*?)(?=\*\*C-\d\d\*\*|$)", para, re.S):
            if pid not in PLACEHOLDERS:
                continue  # dropped numbers
            title_m = re.search(r"`([^`]+)`", body)
            desc = re.sub(r"\s+", " ", body).strip(" ·")
            items[pid] = {
                "id": pid, "repo": REPOS["C"], "title": title_m.group(1) if title_m else pid,
                "size": "placeholder", "phase": PLACEHOLDERS[pid]["phase"],
                "after": PLACEHOLDERS[pid]["after"], "after_note": "",
                "done_when": desc, "ops": False, "placeholder": True,
            }
    data = {"owner": OWNER, "repos": REPOS, "milestones": MILESTONES, "items": items}
    BACKLOG_JSON.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    log(f"exported {len(items)} items to {BACKLOG_JSON.name}")
    return data


def load() -> dict:
    return json.loads(BACKLOG_JSON.read_text(encoding="utf-8"))


# ------------------------------------------------------------------------------- check
def topo_order(items: dict) -> list[str]:
    order, state = [], {}

    def visit(i: str, stack: list[str]):
        if state.get(i) == 2:
            return
        if state.get(i) == 1:
            raise SystemExit(f"dependency cycle: {' -> '.join(stack + [i])}")
        state[i] = 1
        for d in items[i]["after"]:
            if d not in items:
                raise SystemExit(f"{i} blocked by unknown {d}")
            visit(d, stack + [i])
        state[i] = 2
        order.append(i)

    for i in sorted(items):
        visit(i, [])
    return order


def check() -> None:
    data = load()
    items = data["items"]
    order = topo_order(items)
    by_repo: dict[str, list[str]] = {}
    for i in order:
        by_repo.setdefault(items[i]["repo"], []).append(i)
    log(f"{len(items)} items, no cycles, all references resolve")
    for repo, ids in by_repo.items():
        ops = sum(1 for i in ids if items[i]["ops"])
        ph = sum(1 for i in ids if items[i]["placeholder"])
        phases = sorted({items[i]["phase"] for i in ids})
        log(f"  {repo}: {len(ids)} issues ({ops} ops, {ph} placeholders), milestones {phases}")
    deps = sum(len(items[i]["after"]) for i in items)
    log(f"  {deps} dependency links")
    notes = {i: items[i]["after_note"] for i in items if items[i]["after_note"]}
    if notes:
        log(f"  non-issue blockers (informational): {notes}")


# ------------------------------------------------------------------------------- repos
RULESET_NAME = "protect integration branches"


def ruleset_body(branches: list[str]) -> dict:
    return {
        "name": RULESET_NAME,
        "target": "branch",
        "enforcement": "active",
        "conditions": {"ref_name": {"include": [f"refs/heads/{b}" for b in branches], "exclude": []}},
        "rules": [
            {"type": "deletion"},
            {"type": "non_fast_forward"},
            {"type": "required_linear_history"},
            {"type": "pull_request", "parameters": {
                "required_approving_review_count": 0,
                "dismiss_stale_reviews_on_push": false_(),
                "require_code_owner_review": false_(),
                "require_last_push_approval": false_(),
                "required_review_thread_resolution": false_(),
                "allowed_merge_methods": ["squash"],
            }},
        ],
        "bypass_actors": [],
    }


def false_():
    return False


def repos() -> None:
    # 1. the layer repo
    full = repo_full("Y")
    exists = subprocess.run(["gh", "repo", "view", full, "--json", "name"], capture_output=True).returncode == 0
    if not exists:
        log(f"creating {full}")
        gh("repo", "create", full, "--public", "--add-readme", "--description", REPO_DESCRIPTIONS["Y"])
        time.sleep(3)
    # 2. every repo: visibility, merge settings, description, ruleset
    for prefix, name in REPOS.items():
        full = repo_full(prefix)
        info = gh_json("repo", "view", full, "--json", "visibility,defaultBranchRef,description")
        if info["visibility"].upper() != "PUBLIC":
            log(f"{full}: making public")
            gh("repo", "edit", full, "--visibility", "public", "--accept-visibility-change-consequences")
        gh("repo", "edit", full,
           "--enable-squash-merge", "--enable-merge-commit=false", "--enable-rebase-merge=false",
           "--delete-branch-on-merge", "--allow-update-branch",
           "--description", REPO_DESCRIPTIONS[prefix])
        gh("api", "-X", "PATCH", f"repos/{full}",
           "-f", "squash_merge_commit_title=PR_TITLE", "-f", "squash_merge_commit_message=COMMIT_MESSAGES",
           quiet=True)
        branches = [info["defaultBranchRef"]["name"]]
        if prefix == "L":
            branches = sorted(set(branches + ["main", "library"]))
        existing = gh_json("api", f"repos/{full}/rulesets") or []
        if not any(r["name"] == RULESET_NAME for r in existing):
            log(f"{full}: creating ruleset for {branches}")
            body = json.dumps(ruleset_body(branches))
            for attempt in range(4):  # a fresh visibility flip needs a moment to propagate
                r = subprocess.run(["gh", "api", "-X", "POST", f"repos/{full}/rulesets", "--input", "-"],
                                   input=body, text=True, capture_output=True)
                if r.returncode == 0:
                    break
                log(f"{full}: ruleset attempt {attempt + 1} failed: {r.stderr.strip()[-200:]}; retrying")
                time.sleep(8)
            else:
                raise RuntimeError(f"{full}: could not create ruleset")
        log(f"{full}: public, squash-only, delete-branch-on-merge, ruleset on {branches}")


# ------------------------------------------------------------------------------ labels
def labels() -> None:
    for prefix in REPOS:
        full = repo_full(prefix)
        for name, (color, desc) in LABELS.items():
            gh("label", "create", name, "-R", full, "--color", color, "--description", desc, "--force", quiet=True)
        log(f"{full}: {len(LABELS)} labels ensured")


# -------------------------------------------------------------------------- milestones
def milestones() -> None:
    data = load()
    items = data["items"]
    for prefix, name in REPOS.items():
        full = repo_full(prefix)
        needed = sorted({it["phase"] for it in items.values() if it["repo"] == name})
        existing = {m["title"] for m in (gh_json("api", f"repos/{full}/milestones?state=all&per_page=100") or [])}
        for ph in needed:
            title = MILESTONES[ph]
            if title not in existing:
                gh("api", "-X", "POST", f"repos/{full}/milestones", "-f", f"title={title}",
                   "-f", f"description=Roadmap phase {ph} (docs/ROADMAP.md)", quiet=True)
                time.sleep(0.5)
        log(f"{full}: milestones {[MILESTONES[p] for p in needed]}")


# ------------------------------------------------------------------------------ issues
def labels_for(it: dict) -> list[str]:
    t = it["title"]
    head = t.split(":", 1)[0]
    out: list[str] = []
    if it["ops"]:
        return ["ops"]
    if head.startswith("ci") or head.startswith("chore(tools)"):
        out.append("area:ci")
    if head.startswith("build(docker)"):
        out.append("area:docker")
    if head.startswith("build(kas)") or head.startswith("feat(recipes)") or it["repo"] == REPOS["Y"]:
        out.append("area:yocto")
    if head.startswith(("build(cmake)", "chore(cmake)", "build(coverage)")):
        out.append("area:cmake")
    if head.startswith("test"):
        out.append("area:test")
    if head.startswith("docs") or "(prose)" in it["size"]:
        out.append("area:docs")
    if head.startswith(("style", "refactor")):
        out.append("area:quality")
    if head.startswith(("feat(bench)", "feat(perf)")):
        out.append("area:perf")
    if head.startswith("chore(license)") or "license" in t.lower():
        out.append("area:license")
    if "seed" in t.lower():
        out.append("area:code")
    if head.startswith(("fix(", "feat(bloom)", "refactor(", "feat: seed", "chore: seed", "fix:")) or head == "feat" and it["repo"] == REPOS["A"]:
        out.append("area:code")
    if head.startswith("chore") and not out:
        out.append("area:ci")
    if "mechanical" in it["size"] or "deletion" in it["size"]:
        out.append("mechanical")
    if not out:
        out.append("area:code")
    return sorted(set(out))


def bullets(text: str) -> list[str]:
    """Split a Done-when cell on '; ' but not inside (), [] or backticks."""
    out, depth, tick, cur = [], 0, False, ""
    i = 0
    while i < len(text):
        ch = text[i]
        if ch == "`":
            tick = not tick
        elif not tick and ch in "([":
            depth += 1
        elif not tick and ch in ")]":
            depth = max(0, depth - 1)
        if text.startswith("; ", i) and depth == 0 and not tick:
            out.append(cur.strip()); cur = ""; i += 2; continue
        cur += ch; i += 1
    if cur.strip():
        out.append(cur.strip())
    return out


def body_for(it: dict, items: dict, urls: dict[str, str]) -> str:
    lines = [f"<!-- backlog:{it['id']} -->",
             f"**Backlog ID** `{it['id']}` · **Phase** {it['phase']} · **Size** {it['size']}"]
    decisions = sorted(set(re.findall(r"\bD\d{1,2}(?:\.\d)?\b", it["done_when"] + " " + it["title"])))
    if decisions:
        lines.append(f"**Decisions** {', '.join(decisions)} — see `docs/DECISIONS.md` (lands with L-03).")
    lines += ["", "### Scope", it["title"] + ("" if it["title"].endswith(".") else "."), ""]
    if it["placeholder"]:
        lines += ["_Placeholder: created early so dependency links resolve; details are filled in when its phase opens._", ""]
    if it["ops"]:
        lines += ["### Checklist"] + [f"- [ ] {b}" for b in bullets(it["done_when"])] + [""]
    else:
        lines += ["### Done when"] + [f"- {b}" for b in bullets(it["done_when"])] + [""]
    if it["after"]:
        lines.append("### Blocked by")
        for d in it["after"]:
            ref = urls.get(d, "(not created yet)")
            lines.append(f"- {ref} — {d} {items[d]['title']}")
        lines.append("")
    if it["after_note"]:
        lines += [f"_Also gated on: {it['after_note']}._", ""]
    if it["ops"]:
        lines += ["### Conventions",
                  "Settings/operations item: applied with `gh` (see `tools/planning/plan.py`) and closed with a "
                  "comment listing what was done; no PR. Planning mechanics: `docs/DECISIONS.md` D17."]
        return "\n".join(lines) + "\n"
    lines += ["### Conventions",
              "One squash-merged PR closes this issue; the PR title is this issue's title (Conventional Commits, "
              "lint-enforced). Mechanical diffs stay in their own PR. Commits inside the PR are reviewable on their own. "
              "Planning mechanics: `docs/DECISIONS.md` D17; tooling: `tools/planning/` in the library repo."]
    return "\n".join(lines) + "\n"


def existing_issue_map(write: bool = True) -> dict[str, dict]:
    """backlog ID -> {'url','number','repo'} for issues that already carry the marker.

    Lists every issue in every repo; with `write` the result replaces issue-map.json."""
    found: dict[str, dict] = {}
    for prefix, name in REPOS.items():
        full = repo_full(prefix)
        rows = gh_json("issue", "list", "-R", full, "--state", "all", "--limit", "500",
                       "--json", "number,url,body") or []
        for r in rows:
            m = re.search(r"<!-- backlog:([CYLA]-\d\d) -->", r.get("body") or "")
            if m:
                found[m.group(1)] = {"url": r["url"], "number": r["number"], "repo": full}
    if write:
        ISSUE_MAP.write_text(json.dumps(found, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return found


def issues(dry_run: bool = False) -> None:
    data = load()
    items = data["items"]
    order = topo_order(items)
    found = existing_issue_map(write=not dry_run)
    urls = {k: v["url"] for k, v in found.items()}
    created = 0
    for iid in order:
        if iid in found:
            continue
        it = items[iid]
        full = f"{OWNER}/{it['repo']}"
        if dry_run:
            log(f"  {iid} would be created in {full}: {it['title']}")
            created += 1
            continue
        body = body_for(it, items, urls)
        tmp = HERE / ".body.md"
        tmp.write_text(body, encoding="utf-8")
        url = gh("issue", "create", "-R", full, "-t", it["title"], "-F", str(tmp),
                 "-l", ",".join(labels_for(it)), "-m", MILESTONES[it["phase"]])
        url = url.splitlines()[-1].strip()
        found[iid] = {"url": url, "number": int(url.rsplit("/", 1)[1]), "repo": full}
        urls[iid] = url
        created += 1
        log(f"  {iid} -> {url}")
        time.sleep(CREATE_DELAY)
    tmp = HERE / ".body.md"
    if tmp.exists():
        tmp.unlink()
    if dry_run:
        log(f"issues (dry run): {created} would be created, {len(found)} already exist")
        return
    ISSUE_MAP.write_text(json.dumps(found, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    log(f"issues: {created} created, {len(found) - created} already existed, {len(found)} total")


def deps() -> None:
    data = load()
    items = data["items"]
    found = json.loads(ISSUE_MAP.read_text(encoding="utf-8")) if ISSUE_MAP.exists() else existing_issue_map()
    missing = [i for i in items if i not in found]
    if missing:
        raise SystemExit(f"create these issues first: {missing}")
    linked = 0
    for iid, it in items.items():
        if not it["after"]:
            continue
        blockers = ",".join(found[d]["url"] for d in it["after"])
        r = subprocess.run(["gh", "issue", "edit", "-R", found[iid]["repo"], str(found[iid]["number"]),
                            "--add-blocked-by", blockers], capture_output=True, text=True)
        if r.returncode != 0:
            log(f"  {iid}: WARN {r.stderr.strip().splitlines()[-1] if r.stderr.strip() else 'unknown error'}")
        else:
            linked += len(it["after"])
        time.sleep(EDIT_DELAY)
    # refresh bodies whose "Blocked by" section was written before the blockers existed
    refreshed = 0
    for iid, it in items.items():
        if not it["after"]:
            continue
        body = body_for(it, items, {k: v["url"] for k, v in found.items()})
        tmp = HERE / ".body.md"
        tmp.write_text(body, encoding="utf-8")
        gh("issue", "edit", "-R", found[iid]["repo"], str(found[iid]["number"]), "-F", str(tmp), quiet=True)
        refreshed += 1
        time.sleep(EDIT_DELAY)
    if (HERE / ".body.md").exists():
        (HERE / ".body.md").unlink()
    log(f"deps: {linked} links added, {refreshed} bodies refreshed")


def close_ops(summary: str) -> None:
    data = load()
    items = data["items"]
    found = json.loads(ISSUE_MAP.read_text(encoding="utf-8"))
    for iid, it in items.items():
        if not it["ops"] or not it["title"].startswith("ops: repository settings") and not it["title"].startswith("ops: create repo") and not it["title"].startswith("ops: settings"):
            continue
        info = found[iid]
        state = gh_json("issue", "view", "-R", info["repo"], str(info["number"]), "--json", "state")["state"]
        if state.upper() == "CLOSED":
            continue
        gh("issue", "close", "-R", info["repo"], str(info["number"]), "-c", summary)
        log(f"  closed {iid} {info['url']}")
        time.sleep(EDIT_DELAY)


OPS_SUMMARY = f"""Done by `tools/planning/plan.py` on {time.strftime("%Y-%m-%d")}:

- [x] repository public
- [x] squash-merge only; merge commits and rebase merges disabled; squash commit title = PR title, message = the PR's commit messages (not the PR body)
- [x] delete branch on merge; "update branch" allowed
- [x] ruleset **protect integration branches** on the integration branch(es): PR required, squash only, linear history, no deletion, no force-push (required status checks are added once the PR gate exists)
- [x] shared label set (`area:*`, `ops`, `mechanical`, `decision-needed`)
- [x] phase milestones
- [x] every backlog item filed as an issue with native *blocked by* links

Left for the owner (needs interactive auth): `gh auth refresh -s project`, then `gh project create --owner @me --title "cxx-cmake roadmap"` and `python3 tools/planning/plan.py project` to add every issue.
"""


PROJECT_TITLE = "cxx-cmake roadmap"


def project() -> None:
    """Add every issue to the user-level Project (needs `gh auth refresh -s project`)."""
    found = json.loads(ISSUE_MAP.read_text(encoding="utf-8")) if ISSUE_MAP.exists() else existing_issue_map()
    projects = gh_json("project", "list", "--owner", "@me", "--format", "json") or {}
    match = [p for p in projects.get("projects", []) if p["title"] == PROJECT_TITLE]
    if match:
        number = match[0]["number"]
    else:
        created = gh_json("project", "create", "--owner", "@me", "--title", PROJECT_TITLE, "--format", "json")
        number = created["number"]
        log(f"created project #{number} {PROJECT_TITLE}")
    have = gh_json("project", "item-list", str(number), "--owner", "@me", "--limit", "500", "--format", "json") or {}
    present = {it.get("content", {}).get("url") for it in have.get("items", [])}
    added = 0
    for iid, info in sorted(found.items()):
        if info["url"] in present:
            continue
        gh("project", "item-add", str(number), "--owner", "@me", "--url", info["url"], quiet=True)
        added += 1
        log(f"project #{number} added ({added} / {len(found)})")
        time.sleep(EDIT_DELAY)
    log(f"project #{number}: {added} items added, {len(found)} total")


def main(argv: list[str]) -> None:
    ap = argparse.ArgumentParser(prog="plan.py", description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("command", nargs="?", default="check",
                    choices=["export", "check", "repos", "labels", "milestones", "issues", "deps",
                             "close-ops", "project", "all"])
    ap.add_argument("--backlog", metavar="PATH", default="BACKLOG.md",
                    help="export only: the maintainer's BACKLOG.md (default: ./BACKLOG.md)")
    ap.add_argument("--dry-run", action="store_true",
                    help="issues only: report what would be created without creating anything")
    args = ap.parse_args(argv[1:])
    cmd = args.command
    if cmd == "export":
        export(Path(args.backlog))
    elif cmd == "check":
        check()
    elif cmd == "repos":
        repos()
    elif cmd == "labels":
        labels()
    elif cmd == "milestones":
        milestones()
    elif cmd == "issues":
        issues(dry_run=args.dry_run)
    elif cmd == "deps":
        deps()
    elif cmd == "close-ops":
        close_ops(OPS_SUMMARY)
    elif cmd == "project":
        project()
    elif cmd == "all":
        repos(); labels(); milestones(); issues(); deps(); close_ops(OPS_SUMMARY)


if __name__ == "__main__":
    main(sys.argv)
