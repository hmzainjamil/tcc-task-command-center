# tcc-task-command-center
Parallel task queue for Claude Code — fire multiple agents, workflows, automations simultaneously

![Python](https://img.shields.io/badge/Python-3.11+-3776AB?style=flat&labelColor=555&logo=python)
![Claude](https://img.shields.io/badge/Claude-Code-cc785c?style=flat&labelColor=555)
![Agents](https://img.shields.io/badge/Agents-Parallel-purple?style=flat&labelColor=555)
![Status](https://img.shields.io/badge/Status-Active-brightgreen?style=flat&labelColor=555)

[Concepts](#-concepts) · [How It Works](#️-how-it-works) · [Install](#-install) · [Commands](#-commands) · [Tips](#-tips-and-tricks-8) · [Startups](#️-startups--businesses)

---

## 🧠 CONCEPTS

| Feature | Location | Description |
|---------|----------|-------------|
| [**tcc**](tcc) | `tcc` | 570-line task command center — add, fire, list, cancel tasks |
| [**tcc-dashboard**](tcc-dashboard) | `tcc-dashboard` | Live terminal dashboard — active tasks, status, cost, ETA |
| [**tcc-init**](tcc-init) | `tcc-init` | Bootstrap TCC environment, create queue dirs, validate dependencies |
| [**Task Queue**](~/.claude/tcc-logs/) | `~/.claude/tcc-queue.json` | Persistent JSON queue — survives session restarts |
| [**Priority Levels**](tcc) | `--priority high/med/low` | High fires immediately, med queues, low batches overnight |
| [**Agent Types**](tcc) | `--agent TYPE` | Route to specific agent: ads, seo, dev, research, writing |

### 🔥 Hot

| Feature | Location | Description |
|---------|----------|-------------|
| [**tcc blast**](tcc) | `tcc blast "t1" "t2" "t3"` | Fire 3+ tasks simultaneously in parallel — fastest throughput |
| [**tcc fire all**](tcc) | `tcc fire all` | Drain entire queue at once — all pending tasks execute in parallel |
| [**Live Dashboard**](tcc-dashboard) | `tcc-dashboard` | Real-time task progress, token usage, cost per task |

---

## ⚙️ HOW IT WORKS

```
tcc add "write ad copy for HVAC client" --priority high --agent ads-creative
tcc add "audit Google Ads account" --priority high --agent paid-media-auditor
tcc add "build landing page" --priority med --agent senior-developer
         ↓
tcc fire all
         ↓
All 3 tasks execute in parallel via Claude sub-agents
         ↓
Results saved to ~/.claude/tcc-logs/YYYY-MM-DD/task-[ID].json
         ↓
tcc-dashboard shows live progress
```

**Queue persistence:** Tasks survive Claude restarts — queue written to `~/.claude/tcc-queue.json`

---

## 🚀 INSTALL

```bash
git clone https://github.com/hmzainjamil/tcc-task-command-center
cd tcc-task-command-center
cp tcc tcc-* ~/.claude/bin/
chmod +x ~/.claude/bin/tcc ~/.claude/bin/tcc-*
~/.claude/bin/tcc-init
```

---

## 📟 COMMANDS

| Command | Description |
|---------|-------------|
| `tcc add "task" [--priority] [--agent]` | Add task to queue |
| `tcc blast "t1" "t2" "t3"` | Add + fire multiple tasks immediately |
| `tcc fire [ID\|all]` | Execute specific task or drain all |
| `tcc list [--status pending\|active\|done\|failed]` | View queue |
| `tcc status [ID]` | Detailed task status |
| `tcc cancel ID` | Cancel pending task |
| `tcc-dashboard` | Open live terminal dashboard |

---

## 💡 TIPS AND TRICKS (8)

[queue](#tips-queue) · [parallel](#tips-parallel) · [agents](#tips-agents) · [debug](#tips-debug)

<a id="tips-queue"></a>■ **Queue Management (2)**

| Tip | Source |
|-----|--------|
| `tcc list --status pending` before `fire all` — review queue before mass execution | [HMZ](https://github.com/hmzainjamil) |
| Queue persists in `~/.claude/tcc-queue.json` — edit directly to bulk-import tasks | [HMZ](https://github.com/hmzainjamil) |

<a id="tips-parallel"></a>■ **Parallel Execution (2)**

| Tip | Source |
|-----|--------|
| `tcc blast` is faster than `tcc add` + `tcc fire` — single command, no intermediate state | [DigiMinds](https://github.com/hmzainjamil) |
| Max 5 parallel tasks recommended on M1 Pro — higher causes context thrashing | [HMZ](https://github.com/hmzainjamil) |

<a id="tips-agents"></a>■ **Agent Routing (2)**

| Tip | Source |
|-----|--------|
| `--agent ads-strategy` routes to the ads specialist — gets better output than generic | [HMZ](https://github.com/hmzainjamil) |
| Omit `--agent` for auto-routing — TCC picks agent from task description keywords | [DigiMinds](https://github.com/hmzainjamil) |

<a id="tips-debug"></a>■ **Debug (2)**

| Tip | Source |
|-----|--------|
| `tcc status TASK_ID` shows full model trace + token count | [HMZ](https://github.com/hmzainjamil) |
| Failed tasks auto-retry once with `--retry` flag — check logs for error reason | [HMZ](https://github.com/hmzainjamil) |

---

## ☠️ STARTUPS / BUSINESSES

| This Repo / Feature | Replaced |
|-|-|
| **tcc blast (parallel agents)** | [Ray](https://ray.io), [Celery](https://celeryq.dev), [Prefect](https://prefect.io) — no infra needed |
| **tcc queue** | [Linear](https://linear.app), [Jira](https://atlassian.com/jira), [Asana](https://asana.com) — CLI-native |
| **tcc-dashboard** | [Datadog](https://datadoghq.com), [Grafana](https://grafana.com) — zero setup |
| **Agent routing** | [AgentOps](https://agentops.ai), [Langfuse](https://langfuse.com), [Weights & Biases](https://wandb.ai) |

---

## Star History

[![Star History Chart](https://api.star-history.com/svg?repos=hmzainjamil/tcc-task-command-center&type=Date)](https://star-history.com/#hmzainjamil/tcc-task-command-center&Date)

---

<div align="center">
Built by <a href="https://github.com/hmzainjamil">HMZ</a> · Part of the <a href="https://github.com/hmzainjamil/claude-ai-system">HMZ Claude AI System</a>
</div>
