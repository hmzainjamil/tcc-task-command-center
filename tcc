#!/usr/bin/env python3
"""
TCC — Task Command Center
Deploy multiple agents, workflows, automations in parallel.
Usage:
  tcc add "task description" [--type TYPE] [--priority high|med|low] [--agent AGENT]
  tcc fire [TASK_ID|all]        — execute task(s) now
  tcc list [--status pending|active|done|failed]
  tcc status [TASK_ID]
  tcc cancel TASK_ID
  tcc queue                     — show full queue
  tcc routes                    — show routing table
  tcc blast "task1" "task2" ... — add + fire multiple tasks at once
  tcc watch                     — live status monitor
"""

import sys, os, json, uuid, subprocess, threading, time, argparse
from datetime import datetime
from pathlib import Path

TASKS_DIR = Path.home() / ".claude/tasks"
LOGS_DIR  = Path.home() / ".claude/tcc-logs"
ROUTES_FILE = Path.home() / ".claude/tcc-routes/routes.json"
TASKS_DIR.mkdir(parents=True, exist_ok=True)
LOGS_DIR.mkdir(parents=True, exist_ok=True)

# ─── ROUTING TABLE ───────────────────────────────────────────────────────────
DEFAULT_ROUTES = {
    "keywords": {
        # ads / marketing
        "ads|ppc|meta|facebook|google ads|campaign|roas|cpc|ctr": {
            "agent": "tier0-blast",
            "model": "groq",
            "skills": ["ads-strategy","ads-copy","ads-creative"]
        },
        "seo|geo|ranking|schema|crawl|backlink|content brief": {
            "agent": "tier0-blast",
            "model": "gemini",
            "skills": ["searchfit-seo:seo-audit","searchfit-seo:content-strategy"]
        },
        # scraping / research
        "scrape|extract|actor|apify|web data|crawl site": {
            "agent": "apify",
            "actor": "apify/web-scraper",
            "model": "none"
        },
        "research|analyze|report|summary|brief|intel|competitor": {
            "agent": "tier0-blast",
            "model": "gemini",
            "skills": []
        },
        # code
        "code|build|fix|debug|implement|script|function|class|api": {
            "agent": "claude-code-subagent",
            "model": "deepseek",
            "skills": ["engineering:debug","engineering:system-design"]
        },
        # outreach / leads
        "lead|outreach|email|linkedin|prospect|apollo|contact": {
            "agent": "tier0-blast",
            "model": "groq",
            "skills": ["apollo:prospect","sales:draft-outreach"]
        },
        # content
        "write|draft|copy|blog|post|article|social": {
            "agent": "tier0-blast",
            "model": "gpt4o-mini",
            "skills": ["marketing:content-creation","marketing:draft-content"]
        },
        # pdf / doc
        "pdf|document|report|audit|presentation|pptx|docx": {
            "agent": "claude-code-subagent",
            "model": "gemini",
            "skills": ["anthropic-skills:pdf","anthropic-skills:pptx"]
        },
        # workflow / automation
        "workflow|automate|automation|pipeline|cron|schedule|n8n": {
            "agent": "workflow-dag",
            "model": "none",
            "skills": []
        },
        # paperclip / agency ops
        "agency|digiminds|paperclip|client|proposal|kpi|goal": {
            "agent": "paperclip",
            "model": "none",
            "skills": []
        },
        # hermes / nous research agent
        "hermes|nous|self-evolve|evolve skill|hermes agent|function calling hermes": {
            "agent": "hermes_local",
            "model": "openrouter/deepseek",
            "skills": ["~/.claude/agents/hermes-nous-agent.md"]
        }
    },
    "default": {
        "agent": "tier0-blast",
        "model": "groq",
        "skills": []
    }
}

# ─── HELPERS ─────────────────────────────────────────────────────────────────
def load_routes():
    if ROUTES_FILE.exists():
        return json.loads(ROUTES_FILE.read_text())
    ROUTES_FILE.parent.mkdir(parents=True, exist_ok=True)
    ROUTES_FILE.write_text(json.dumps(DEFAULT_ROUTES, indent=2))
    return DEFAULT_ROUTES

def route_task(description: str, routes: dict) -> dict:
    desc_lower = description.lower()
    for pattern, config in routes["keywords"].items():
        for keyword in pattern.split("|"):
            if keyword.strip() in desc_lower:
                return config
    return routes["default"]

def save_task(task: dict):
    path = TASKS_DIR / f"{task['id']}.json"
    path.write_text(json.dumps(task, indent=2))

def load_task(task_id: str) -> dict:
    path = TASKS_DIR / f"{task_id}.json"
    if not path.exists():
        # try partial match
        matches = list(TASKS_DIR.glob(f"{task_id}*.json"))
        if matches:
            path = matches[0]
        else:
            return None
    return json.loads(path.read_text())

def load_all_tasks(status_filter=None) -> list:
    tasks = []
    for f in sorted(TASKS_DIR.glob("*.json"), key=lambda x: x.stat().st_mtime, reverse=True):
        try:
            t = json.loads(f.read_text())
            if status_filter is None or t.get("status") == status_filter:
                tasks.append(t)
        except:
            pass
    return tasks

def new_task(description, task_type=None, priority="med", agent_override=None, meta=None) -> dict:
    routes = load_routes()
    route = route_task(description, routes)
    if agent_override:
        route = dict(route)
        route["agent"] = agent_override
    return {
        "id": str(uuid.uuid4())[:8],
        "description": description,
        "type": task_type or "auto",
        "priority": priority,
        "status": "pending",
        "route": route,
        "created": datetime.now().isoformat(),
        "started": None,
        "completed": None,
        "result": None,
        "log": None,
        "meta": meta or {}
    }

COLORS = {
    "pending": "\033[33m",   # yellow
    "active":  "\033[34m",   # blue
    "done":    "\033[32m",   # green
    "failed":  "\033[31m",   # red
    "reset":   "\033[0m"
}

def colorize(text, color):
    return f"{COLORS.get(color,'')}{text}{COLORS['reset']}"

def fmt_task(t):
    c = t["status"]
    badge = colorize(f"[{t['status'].upper():8}]", c)
    prio = {"high": "🔴", "med": "🟡", "low": "🟢"}.get(t["priority"], "⚪")
    agent = t["route"].get("agent", "?")
    model = t["route"].get("model", "")
    model_str = f" via {model}" if model and model != "none" else ""
    ts = t["created"][:16].replace("T", " ")
    return f"{badge} {prio} [{t['id']}] {t['description'][:60]} → {agent}{model_str}  {ts}"

# ─── EXECUTORS ───────────────────────────────────────────────────────────────

def execute_tier0_blast(task: dict) -> str:
    prompt = task["description"]
    # llm-burst is the proven working executor — routes to Groq/Gemini/OpenRouter/Ollama
    safe_prompt = prompt.replace("'", "\\'")
    cmd = f"source ~/.claude/tier0.env 2>/dev/null && ~/.claude/bin/llm-burst '{safe_prompt[:400]}' 2>/dev/null"
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=120)
    output = result.stdout.strip() or result.stderr.strip() or "(no output)"
    return output[:2000]

def execute_paperclip(task: dict) -> str:
    """Route agency/ops tasks: try Paperclip CEO layer, fall back to llm-burst."""
    import urllib.request
    api = "http://127.0.0.1:3100"
    data = json.dumps({
        "title": task["description"][:100],
        "description": task["description"],
        "priority": task["priority"],
        "source": "tcc"
    }).encode()

    # Try known Paperclip endpoints
    for endpoint in ["/api/projects", "/api/tasks", "/api/goals", "/projects", "/tasks"]:
        try:
            req = urllib.request.Request(
                f"{api}{endpoint}",
                data=data,
                headers={"Content-Type": "application/json", "User-Agent": "TCC/1.0"},
                method="POST"
            )
            with urllib.request.urlopen(req, timeout=5) as resp:
                body = resp.read().decode()
                if "error" not in body.lower():
                    return f"Paperclip {endpoint}: {body[:150]}"
        except Exception:
            continue

    # Paperclip API not available for this route — execute via llm-burst instead
    return execute_tier0_blast(task)

def execute_workflow_dag(task: dict) -> str:
    # Create a mini workflow JSON and run it
    wf_id = task["id"]
    wf = {
        "name": task["description"][:50],
        "description": task["description"],
        "steps": [
            {
                "id": "main",
                "command": "echo 'Workflow task: {}' && date".format(task["description"][:100].replace("'", "")),
                "depends_on": []
            }
        ]
    }
    wf_path = Path.home() / f".claude/workflows/tcc-{wf_id}.json"
    wf_path.write_text(json.dumps(wf, indent=2))

    cmd = f"~/.claude/bin/workflow-dag ~/.claude/workflows/tcc-{wf_id}.json"
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=60)
    return result.stdout or result.stderr or "(no output)"

def execute_claude_code_subagent(task: dict) -> str:
    # Launch claude -p for the task (non-interactive)
    cmd = f'claude -p "{task["description"][:300]}" --output-format json 2>&1'
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True, timeout=300)
    out = result.stdout[:1000] if result.stdout else result.stderr[:500]
    return out

def execute_apify(task: dict) -> str:
    actor = task["route"].get("actor", "apify/web-scraper")
    return f"[Apify] Would run actor {actor} for: {task['description']}\nUse: tcc fire {task['id']} --mcp to trigger via MCP"

def execute_task(task: dict) -> str:
    agent = task["route"].get("agent", "tier0-blast")

    dispatch = {
        "tier0-blast":          execute_tier0_blast,
        "paperclip":            execute_paperclip,
        "workflow-dag":         execute_workflow_dag,
        "claude-code-subagent": execute_claude_code_subagent,
        "apify":                execute_apify,
    }

    fn = dispatch.get(agent, execute_tier0_blast)
    return fn(task)

def fire_task(task_id: str, dry_run=False):
    task = load_task(task_id)
    if not task:
        print(f"Task {task_id} not found")
        return

    task["status"] = "active"
    task["started"] = datetime.now().isoformat()
    save_task(task)

    print(f"  → Firing [{task['id']}] via {task['route'].get('agent')} | {task['description'][:60]}")

    if dry_run:
        task["status"] = "done"
        task["result"] = "[dry-run]"
        task["completed"] = datetime.now().isoformat()
        save_task(task)
        return

    try:
        result = execute_task(task)
        task["status"] = "done"
        task["result"] = result[:500]
    except Exception as e:
        task["status"] = "failed"
        task["result"] = str(e)[:300]

    task["completed"] = datetime.now().isoformat()

    # Write log
    log_path = LOGS_DIR / f"{task['id']}.log"
    log_path.write_text(f"Task: {task['description']}\nAgent: {task['route']}\nResult:\n{task.get('result','')}")
    task["log"] = str(log_path)

    save_task(task)

    status_sym = "✓" if task["status"] == "done" else "✗"
    print(f"  {status_sym} [{task['id']}] {task['status']} — {str(task.get('result',''))[:80]}")

def fire_parallel(task_ids: list, dry_run=False):
    threads = []
    for tid in task_ids:
        t = threading.Thread(target=fire_task, args=(tid, dry_run))
        threads.append(t)
        t.start()
        time.sleep(0.05)  # slight stagger to avoid race on log files
    for t in threads:
        t.join()

# ─── CLI ─────────────────────────────────────────────────────────────────────

def cmd_add(args):
    description = " ".join(args.description)
    task = new_task(description, args.type, args.priority, args.agent)
    save_task(task)
    print(fmt_task(task))
    print(f"\n  Added task [{task['id']}] → {task['route']['agent']} ({task['route'].get('model','')})")
    if args.fire:
        print(f"\n  Auto-firing...")
        fire_task(task["id"])

def cmd_fire(args):
    if args.task_id == "all" or args.task_id == "pending":
        tasks = load_all_tasks("pending")
        if not tasks:
            print("No pending tasks.")
            return
        print(f"\n⚡ Firing {len(tasks)} pending tasks in PARALLEL\n")
        fire_parallel([t["id"] for t in tasks], dry_run=args.dry_run)
    else:
        fire_task(args.task_id, dry_run=args.dry_run)

def cmd_list(args):
    tasks = load_all_tasks(args.status)
    if not tasks:
        print(f"No tasks{' with status '+args.status if args.status else ''}.")
        return
    print(f"\n{'─'*80}")
    print(f"  TCC TASK QUEUE  ({len(tasks)} tasks{' — '+args.status if args.status else ''})")
    print(f"{'─'*80}")
    for t in tasks:
        print(fmt_task(t))

    counts = {}
    for t in load_all_tasks():
        counts[t["status"]] = counts.get(t["status"], 0) + 1
    print(f"\n  Total: {sum(counts.values())} | " + " | ".join(f"{k}: {v}" for k,v in counts.items()))

def cmd_status(args):
    task = load_task(args.task_id)
    if not task:
        print(f"Task {args.task_id} not found")
        return
    print(f"\n{'─'*60}")
    print(f"  ID:          {task['id']}")
    print(f"  Description: {task['description']}")
    print(f"  Status:      {colorize(task['status'], task['status'])}")
    print(f"  Priority:    {task['priority']}")
    print(f"  Agent:       {task['route'].get('agent')}")
    print(f"  Model:       {task['route'].get('model','—')}")
    print(f"  Created:     {task['created'][:19]}")
    print(f"  Started:     {task.get('started','—')}")
    print(f"  Completed:   {task.get('completed','—')}")
    if task.get("result"):
        print(f"\n  Result:\n  {task['result'][:400]}")
    if task.get("log"):
        print(f"\n  Full log: {task['log']}")

def cmd_cancel(args):
    task = load_task(args.task_id)
    if not task:
        print(f"Task {args.task_id} not found")
        return
    task["status"] = "cancelled"
    save_task(task)
    print(f"Cancelled [{task['id']}] {task['description'][:60]}")

def cmd_retry(args):
    """Reset failed/cancelled tasks back to pending and optionally fire them."""
    if args.task_id in ("all", "failed"):
        tasks = load_all_tasks("failed") + load_all_tasks("cancelled")
    else:
        t = load_task(args.task_id)
        tasks = [t] if t else []
    if not tasks:
        print("No tasks to retry.")
        return
    for task in tasks:
        task["status"] = "pending"
        task["result"] = None
        task["started"] = None
        task["completed"] = None
        save_task(task)
        print(f"  ↺ Reset [{task['id']}] {task['description'][:60]}")
    if args.fire:
        print(f"\n⚡ Firing {len(tasks)} retried tasks in parallel...")
        fire_parallel([t["id"] for t in tasks])

def cmd_purge(args):
    """Delete task files by status."""
    status = args.status or "failed"
    tasks = load_all_tasks(status)
    if not tasks:
        print(f"No {status} tasks to purge.")
        return
    for task in tasks:
        path = TASKS_DIR / f"{task['id']}.json"
        path.unlink(missing_ok=True)
    print(f"Purged {len(tasks)} {status} task(s).")

def cmd_queue(args):
    pending = load_all_tasks("pending")
    active  = load_all_tasks("active")
    done    = load_all_tasks("done")
    failed  = load_all_tasks("failed")

    print(f"""
╔══════════════════════════════════════════════════════════════════╗
║              TCC — TASK COMMAND CENTER                          ║
╚══════════════════════════════════════════════════════════════════╝
  Pending: {len(pending):3}  |  Active: {len(active):3}  |  Done: {len(done):3}  |  Failed: {len(failed):3}
  Total tasks: {len(pending)+len(active)+len(done)+len(failed)}
""")
    if pending:
        print("  ── PENDING ──")
        for t in pending[:10]:
            print(f"  {fmt_task(t)}")
    if active:
        print("\n  ── ACTIVE ──")
        for t in active:
            print(f"  {fmt_task(t)}")
    if failed:
        print("\n  ── FAILED ──")
        for t in failed[:5]:
            print(f"  {fmt_task(t)}")

def cmd_blast(args):
    """Add + fire multiple tasks at once in parallel"""
    tasks = []
    for desc in args.descriptions:
        task = new_task(desc, priority=args.priority)
        save_task(task)
        tasks.append(task)
        print(fmt_task(task))

    print(f"\n⚡ BLAST: firing {len(tasks)} tasks in parallel...\n")
    fire_parallel([t["id"] for t in tasks], dry_run=args.dry_run)

def cmd_routes(args):
    routes = load_routes()
    print("\n  TCC ROUTING TABLE\n  " + "─"*60)
    for pattern, config in routes["keywords"].items():
        keywords = " | ".join(pattern.split("|")[:4])
        agent = config["agent"]
        model = config.get("model", "—")
        skills = ", ".join(config.get("skills", [])[:2])
        print(f"  [{keywords:40}] → {agent:20} {model}")
    print(f"\n  [default] → {routes['default']['agent']}")

def cmd_watch(args):
    """Simple live status watcher"""
    import curses
    try:
        while True:
            os.system("clear")
            pending = load_all_tasks("pending")
            active  = load_all_tasks("active")
            done    = load_all_tasks("done")
            failed  = load_all_tasks("failed")
            print(f"TCC Watch — {datetime.now().strftime('%H:%M:%S')}   [Ctrl+C to exit]")
            print(f"Pending: {len(pending)}  Active: {len(active)}  Done: {len(done)}  Failed: {len(failed)}")
            print("─"*70)
            for t in (active + pending)[:15]:
                print(fmt_task(t))
            time.sleep(2)
    except KeyboardInterrupt:
        print("\nExiting watch mode.")

# ─── MAIN ─────────────────────────────────────────────────────────────────────
def main():
    parser = argparse.ArgumentParser(
        prog="tcc",
        description="Task Command Center — deploy agents/workflows/automations in parallel"
    )
    sub = parser.add_subparsers(dest="command")

    # add
    p_add = sub.add_parser("add", help="Add a task")
    p_add.add_argument("description", nargs="+")
    p_add.add_argument("--type", "-t", default=None)
    p_add.add_argument("--priority", "-p", default="med", choices=["high","med","low"])
    p_add.add_argument("--agent", "-a", default=None)
    p_add.add_argument("--fire", "-f", action="store_true", help="fire immediately after adding")

    # fire
    p_fire = sub.add_parser("fire", help="Execute task(s)")
    p_fire.add_argument("task_id", nargs="?", default="all")
    p_fire.add_argument("--dry-run", action="store_true")

    # list
    p_list = sub.add_parser("list", help="List tasks")
    p_list.add_argument("--status", "-s", default=None)

    # status
    p_status = sub.add_parser("status", help="Show task details")
    p_status.add_argument("task_id")

    # cancel
    p_cancel = sub.add_parser("cancel", help="Cancel a task")
    p_cancel.add_argument("task_id")

    # queue
    sub.add_parser("queue", help="Show full queue overview")

    # blast
    p_blast = sub.add_parser("blast", help="Add + fire multiple tasks in parallel")
    p_blast.add_argument("descriptions", nargs="+")
    p_blast.add_argument("--priority", "-p", default="high")
    p_blast.add_argument("--dry-run", action="store_true")

    # routes
    sub.add_parser("routes", help="Show routing table")

    # watch
    sub.add_parser("watch", help="Live status monitor")

    # retry
    p_retry = sub.add_parser("retry", help="Reset failed tasks back to pending")
    p_retry.add_argument("task_id", nargs="?", default="failed")
    p_retry.add_argument("--fire", "-f", action="store_true", help="fire immediately after reset")

    # purge
    p_purge = sub.add_parser("purge", help="Delete task files by status")
    p_purge.add_argument("--status", "-s", default="failed")

    args = parser.parse_args()

    dispatch = {
        "add":    cmd_add,
        "fire":   cmd_fire,
        "list":   cmd_list,
        "status": cmd_status,
        "cancel": cmd_cancel,
        "queue":  cmd_queue,
        "blast":  cmd_blast,
        "routes": cmd_routes,
        "watch":  cmd_watch,
        "retry":  cmd_retry,
        "purge":  cmd_purge,
    }

    if args.command in dispatch:
        dispatch[args.command](args)
    else:
        cmd_queue(args)

if __name__ == "__main__":
    main()
