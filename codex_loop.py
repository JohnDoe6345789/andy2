#!/usr/bin/env python3
"""
Codex-style GitHub loop runner.

- Polls GitHub Issues (label-filtered)
- Generates code changes from issue text via OpenAI Chat Completions
- Applies changes returned in <FILE path="...">...</FILE> blocks
- Creates a branch and opens a PR with "Closes #<issue>"
- Adds the 'codex' label to the PR
- Repeats forever

ENV:
  GITHUB_TOKEN   (required)  PAT with 'repo' scope (or a fine-grained token that can read/write issues & PRs)
  OPENAI_API_KEY (required)

ARGS:
  --repo owner/name           (optional; auto-derives from git remote 'origin' if omitted)
  --label codex               (issue label to process)
  --base  main                (base branch)
  --poll  30                  (seconds between polls)
  --paths libIOTCAPIsT.c,tests/**,Makefile   (comma-list of globs included as context)
"""

import argparse, os, re, sys, time, json, glob, subprocess, textwrap
from pathlib import Path
import requests

# ---------- Helpers ----------
def sh(cmd, cwd=None):
    p = subprocess.run(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if p.returncode != 0:
        raise RuntimeError(f"Command failed: {' '.join(cmd)}\n{p.stdout}")
    return p.stdout.strip()

def in_repo():
    try: return sh(["git", "rev-parse", "--is-inside-work-tree"]) == "true"
    except: return False

def derive_repo(arg_repo: str|None):
    if arg_repo: return arg_repo
    url = sh(["git", "remote", "get-url", "origin"])
    m = re.search(r'github\.com[:/]+([^/]+)/([^/]+?)(?:\.git)?$', url)
    if not m: raise RuntimeError("Could not derive owner/name from origin; pass --repo")
    return f"{m.group(1)}/{m.group(2)}"

def gh(method, path, token, **kwargs):
    url = f"https://api.github.com{path}"
    headers = kwargs.pop("headers", {})
    headers.update({"Authorization": f"token {token}", "Accept": "application/vnd.github+json"})
    r = requests.request(method, url, headers=headers, **kwargs)
    if r.status_code >= 300:
        raise RuntimeError(f"GitHub {method} {path} failed {r.status_code}:\n{r.text}")
    return r.json()

def list_open_issues(repo, token, label):
    out = gh("GET", f"/repos/{repo}/issues?state=open&labels={label}&per_page=100", token)
    # Only real issues (PRs also appear in this endpoint)
    return [i for i in out if "pull_request" not in i]

def pr_exists_for_issue(repo, token, issue_num):
    pulls = gh("GET", f"/repos/{repo}/pulls?state=open&per_page=100", token)
    needle = f"#{issue_num}"
    for pr in pulls:
        if pr.get("body") and needle in pr["body"]:
            return True
    return False

def ensure_clean():
    if sh(["git", "status", "--porcelain"]):
        raise RuntimeError("Working tree not clean. Commit or stash your changes first.")

def current_base_sha(base):
    sh(["git", "fetch", "origin", base])
    return sh(["git", "rev-parse", f"origin/{base}"])

def make_branch_from(base, name):
    sh(["git", "checkout", "-B", name, f"origin/{base}"])

def open_pr(repo, token, head, base, title, body):
    pr = gh("POST", f"/repos/{repo}/pulls", token, json={
        "title": title, "head": head, "base": base, "body": body, "maintainer_can_modify": True
    })
    return pr["number"], pr["html_url"]

def add_pr_labels(repo, token, pr_number, labels):
    gh("POST", f"/repos/{repo}/issues/{pr_number}/labels", token, json={"labels": labels})

def ensure_label(repo, token, name, color="0E8A16"):
    try:
        gh("GET", f"/repos/{repo}/labels/{name}", token)
    except:
        try: gh("POST", f"/repos/{repo}/labels", token, json={"name": name, "color": color})
        except: pass

# ---------- OpenAI ----------
def openai_chat(api_key, messages, model="gpt-4.1", temperature=0.2):
    r = requests.post(
        "https://api.openai.com/v1/chat/completions",
        headers={"Authorization": f"Bearer {api_key}"},
        json={"model": model, "messages": messages, "temperature": temperature}
    )
    if r.status_code >= 300:
        raise RuntimeError(f"OpenAI error {r.status_code}:\n{r.text}")
    return r.json()["choices"][0]["message"]["content"]

FILE_BLOCK_RE = re.compile(
    r'<FILE\s+path="([^"]+)">\s*([\s\S]*?)\s*</FILE>',
    re.IGNORECASE
)

def parse_file_blocks(text: str):
    """
    Expect multiple:
      <FILE path="relative/path.c">
      ...contents...
      </FILE>
    """
    matches = FILE_BLOCK_RE.findall(text)
    return [(Path(p).as_posix(), content) for p, content in matches]

# ---------- Context ----------
def collect_context(globs):
    files = []
    for g in globs:
        for p in glob.glob(g, recursive=True):
            pth = Path(p)
            if pth.is_file():
                try:
                    content = pth.read_text(encoding="utf-8", errors="replace")
                except Exception as e:
                    content = f"<<UNREADABLE: {e}>>"
                files.append((pth.as_posix(), content))
    # Limit extremely large files
    MAX = 120_000
    limited = []
    for path, content in files:
        if len(content) > MAX:
            head = content[:60_000]
            tail = content[-60_000:]
            content = head + "\n/* ...SNIP... */\n" + tail
        limited.append((path, content))
    return limited

def build_prompt(issue, context_files):
    issue_title = issue["title"]
    issue_body  = issue.get("body") or ""
    ctx_snips = []
    for path, content in context_files:
        ctx_snips.append(f"<CONTEXT path=\"{path}\">\n{content}\n</CONTEXT>")
    context_blob = "\n\n".join(ctx_snips) if ctx_snips else "(no extra context)"
    system = "You are an expert C/C++ systems developer. Return only file blocks. No prose."
    user = textwrap.dedent(f"""
    Task (from GitHub Issue #{issue['number']}):
    Title: {issue_title}
    Body:
    {issue_body}

    Repository context (read-only):
    {context_blob}

    Requirements:
    - Implement the task safely and incrementally.
    - Return ONLY changed/new files using this exact format for each file you touch:
        <FILE path="relative/path/from/repo/root">
        ...entire file contents...
        </FILE>
    - Do not include any text outside <FILE> blocks.
    - Ensure code compiles; keep existing functionality unless the issue says otherwise.
    """).strip()
    return system, user

# ---------- Apply changes ----------
def write_files(file_blocks):
    for relpath, content in file_blocks:
        p = Path(relpath)
        p.parent.mkdir(parents=True, exist_ok=True)
        p.write_text(content, encoding="utf-8")

# ---------- Main loop ----------
def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--repo", default=None, help="owner/name (derived from origin by default)")
    ap.add_argument("--label", default="codex", help="issue label to process")
    ap.add_argument("--base",  default="main", help="base branch")
    ap.add_argument("--poll",  type=int, default=30, help="poll seconds")
    ap.add_argument("--paths", default="libIOTCAPIsT.c,tests/**,include/**,src/**,Makefile,CMakeLists.txt",
                    help="comma-separated globs to pass as context")
    ap.add_argument("--model", default="gpt-4.1", help="OpenAI model to use")
    args = ap.parse_args()

    gh_token  = os.environ.get("GITHUB_TOKEN")
    oa_token  = os.environ.get("OPENAI_API_KEY")
    if not gh_token or not oa_token:
        print("Set GITHUB_TOKEN and OPENAI_API_KEY in your environment.", file=sys.stderr)
        sys.exit(1)
    if not in_repo():
        print("Run this from the repo root (so we can commit/push).", file=sys.stderr)
        sys.exit(1)

    repo = derive_repo(args.repo)
    owner, name = repo.split("/", 1)
    ensure_label(repo, gh_token, args.label)

    print(f"🚀 Codex loop running for {repo} (label='{args.label}', base='{args.base}')")
    while True:
        try:
            issues = list_open_issues(repo, gh_token, args.label)
            if not issues:
                print("… no open labeled issues.")
            for issue in issues:
                num = issue["number"]
                if pr_exists_for_issue(repo, gh_token, num):
                    print(f"• Issue #{num} already has an open PR; skipping.")
                    continue

                print(f"→ Processing issue #{num}: {issue['title']!r}")
                ensure_clean()
                base_sha = current_base_sha(args.base)

                # Gather context & call the model
                globs = [g.strip() for g in args.paths.split(",") if g.strip()]
                context_files = collect_context(globs)
                system, user = build_prompt(issue, context_files)
                print("  Calling OpenAI…")
                raw = openai_chat(oa_token, [{"role":"system","content":system},
                                              {"role":"user","content":user}],
                                              model=args.model, temperature=0.2)

                blocks = parse_file_blocks(raw)
                if not blocks:
                    print("  ⚠️ Model did not return any <FILE> blocks; skipping this issue for now.")
                    continue

                # Create branch, write files, commit, push, PR
                ts = time.strftime("%Y%m%d-%H%M%S")
                branch = f"codex/issue-{num}-{ts}"
                print(f"  Creating branch {branch} from origin/{args.base}…")
                make_branch_from(args.base, branch)

                print(f"  Writing {len(blocks)} file(s)…")
                write_files(blocks)
                sh(["git", "add", "--all"])
                sh(["git", "commit", "-m", f"Codex: implement #{num} — {issue['title']}"])
                sh(["git", "push", "-u", "origin", branch])

                pr_title = f"[Codex] {issue['title']}"
                pr_body  = f"Closes #{num}\n\nGenerated by codex_loop.py."
                pr_number, pr_url = open_pr(repo, gh_token, branch, args.base, pr_title, pr_body)
                add_pr_labels(repo, gh_token, pr_number, [args.label])
                print(f"  ✅ Opened PR #{pr_number}: {pr_url}")

            time.sleep(args.poll)
        except KeyboardInterrupt:
            print("\nStopping.")
            break
        except Exception as e:
            print(f"ERROR: {e}", file=sys.stderr)
            time.sleep(max(5, args.poll))
            continue

if __name__ == "__main__":
    main()
