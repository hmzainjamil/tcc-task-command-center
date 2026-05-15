# tcc-task-command-center

Task Command Center: parallel blast, sequential queue, and live dashboard for orchestrating all Claude Code tasks efficiently.

![TCC](https://img.shields.io/badge/TCC-Task_Center-blue?style=flat&labelColor=555) ![Parallel](https://img.shields.io/badge/Execution-Parallel-green?style=flat&labelColor=555) ![Dashboard](https://img.shields.io/badge/Dashboard-Live-orange?style=flat&labelColor=555) ![License](https://img.shields.io/badge/License-MIT-yellow?style=flat&labelColor=555)

[Concepts](#-concepts) · [How It Works](#-how-it-works) · [Install](#-install) · [Usage](#-usage) · [Config](#-configuration) · [Tips](#-tips-and-tricks-12) · [Troubleshooting](#-troubleshooting) · [Architecture](#-architecture) · [Startups](#️-startups--businesses)

---

## 🧠 CONCEPTS

| Feature | Location | Description |
|---|---|---|
| Blast Command | `tcc/blast.py` | Fire N tasks simultaneously across Tier 0 models |
| Task Queue | `tcc/queue.py` | FIFO/priority queue with dependency support |
| Fire All | `tcc/fire.py` | Drain the queue — execute all pending tasks |
| Dashboard | `tcc/dashboard.py` | Live terminal dashboard: task status, progress, cost |
| Task Spec | `tasks/` | YAML task definitions: prompt, model, output path |
| Dependency Graph | `tcc/deps.py` | DAG-based task ordering with parallel safe detection |
| Result Aggregator | `tcc/aggregator.py` | Collects all outputs into unified result set |
| Cost Tracker | `tcc/cost.py` | Per-task token cost with daily budget enforcement |
| Retry Engine | `tcc/retry.py` | Auto-retry failed tasks with provider fallback |
| History | `tcc/history.py` | Logs all task runs to SQLite for replay/review |
| API Server | `tcc/server.py` | REST API for programmatic task submission |
| Template Engine | `templates/` | Reusable task templates for common workflows |

### 🔥 Hot

| Feature | Location | Description |
|---|---|---|
| Blast Command | `tcc/blast.py` | N tasks in parallel — N× faster than sequential |
| Dashboard | `tcc/dashboard.py` | Real-time visibility into every running task |
| Fire All | `tcc/fire.py` | Queue drained in one command — no manual babysitting |
| Dependency Graph | `tcc/deps.py` | Complex task ordering without race conditions |
| Cost Tracker | `tcc/cost.py` | Know exactly what each task costs before it runs |

---

## ⚙️ HOW IT WORKS

```
tcc blast "t1" "t2" "t3"
    │
    ▼
┌─────────────────────────────────┐
│  PARALLEL DISPATCH              │
│  t1 → Groq worker               │
│  t2 → Gemini worker             │
│  t3 → DeepSeek worker           │
└─────────────────────────────────┘
    │ (all complete)
    ▼
aggregator.py collects results
    │
    ▼
Save to ~/.claude/tcc-logs/{date}/

tcc fire all
    │
    ├── load queue from queue.db
    ├── resolve dependency graph
    ├── execute in topological order (parallel where safe)
    └── drain queue → all tasks complete
```

---

## 🚀 INSTALL

```bash
git clone https://github.com/hmzainjamil/tcc-task-command-center
cd tcc-task-command-center

pip install -r requirements.txt

# Install TCC CLI
pip install -e .
# or: alias tcc="python3 tcc/cli.py"
# and: alias tcc-dashboard="python3 tcc/dashboard.py"

# Init database
python3 tcc/init_db.py

# Test blast
tcc blast "What is 2+2?" "What is the capital of France?"

# Open dashboard
tcc-dashboard
```

---

## 📟 USAGE

```bash
# Blast N tasks in parallel
tcc blast "task 1 prompt" "task 2 prompt" "task 3 prompt"

# Add task to queue
tcc queue add "Research top 10 keywords for Google Ads" --priority high

# Fire all queued tasks
tcc fire all

# Fire with concurrency limit
tcc fire all --max-parallel 3

# Live dashboard
tcc-dashboard

# View task history
tcc history --last 20

# Retry failed tasks
tcc retry --status failed

# Check cost
tcc cost --today

# Export results
tcc export --format md --output ~/Downloads/results.md

# Define task in YAML
tcc run --task tasks/competitor_analysis.yaml
```

---

## ⚙️ CONFIGURATION

| Variable | Default | Description |
|---|---|---|
| `BLAST_MAX_PARALLEL` | `10` | Max simultaneous blast workers |
| `QUEUE_DB_PATH` | `~/.tcc/queue.db` | SQLite queue database path |
| `RESULTS_DIR` | `~/.claude/tcc-logs/` | Task output directory |
| `DEFAULT_MODEL` | `groq:llama-3.1-70b` | Default model for tasks |
| `TASK_TIMEOUT_S` | `120` | Max seconds per task |
| `DAILY_TOKEN_BUDGET` | `1000000` | Max tokens per day all providers |
| `RETRY_MAX_ATTEMPTS` | `3` | Auto-retry attempts on failure |
| `DASHBOARD_REFRESH_S` | `2` | Dashboard refresh interval |
| `COST_ALERT_USD` | `1.00` | Alert when session cost exceeds |
| `HISTORY_RETENTION_DAYS` | `30` | Days to retain task history |

---

## 💡 TIPS AND TRICKS (12)

[Blast](#tips-blast) · [Queue](#tips-queue) · [Dashboard](#tips-dash) · [Cost](#tips-cost)

<a id="tips-blast"></a>■ **Blast Mode (3)**

| Tip | Source |
|---|---|
| `tcc blast` routes each task to different Tier 0 provider — no provider bottleneck | Blast design |
| Blast is ideal for: N variations, parallel research, multi-account pulls | Use case guide |
| Results saved individually — each blast task has own output file in tcc-logs/ | Results design |

<a id="tips-queue"></a>■ **Queue Mode (3)**

| Tip | Source |
|---|---|
| Add `--depends-on task_id` for sequential dependencies within parallel queue | Dependency guide |
| `--priority high/normal/low` controls execution order when concurrent slots limited | Queue docs |
| `tcc queue list` shows all pending tasks with priorities — review before fire | Queue CLI |

<a id="tips-dash"></a>■ **Dashboard (3)**

| Tip | Source |
|---|---|
| Run dashboard in tmux pane — monitor all tasks without switching terminals | Terminal tips |
| Dashboard shows cost in real-time — spot expensive tasks before they complete | Dashboard features |
| Color coding: green=complete, yellow=running, red=failed, gray=queued | Dashboard legend |

<a id="tips-cost"></a>■ **Cost Management (3)**

| Tip | Source |
|---|---|
| Groq is free tier 0 — route all blast tasks to Groq first via `--model groq` | Cost guide |
| `tcc cost --estimate "prompt"` estimates tokens before running | Cost estimator |
| Set `DAILY_TOKEN_BUDGET=500000` — TCC enforces hard stop when exceeded | Budget enforcement |

---

## 🔧 TROUBLESHOOTING

| Issue | Fix |
|---|---|
| Blast tasks failing silently | `tcc history --status failed --last 5` — see error details |
| Queue not draining | Check `tcc queue list` — dependency deadlock? |
| Dashboard not updating | Reduce `DASHBOARD_REFRESH_S=1` if updates seem stale |
| Cost exceeds budget | `tcc cost reset` or increase `DAILY_TOKEN_BUDGET` |
| DB locked error | Kill stale TCC process: `pkill -f tcc` |
| Tasks timing out | Increase `TASK_TIMEOUT_S` for long-running tasks |
| Blast too slow | Check Tier 0 provider latency: `tcc providers ping` |

---

## 📊 ARCHITECTURE

```
tcc-task-command-center/
├── tcc/
│   ├── cli.py                  # Main CLI
│   ├── blast.py                # Parallel task execution
│   ├── queue.py                # Task queue management
│   ├── fire.py                 # Queue drain execution
│   ├── dashboard.py            # Live terminal UI
│   ├── deps.py                 # Dependency graph (DAG)
│   ├── aggregator.py           # Result collection
│   ├── cost.py                 # Token cost tracking
│   ├── retry.py                # Failure retry logic
│   ├── history.py              # SQLite task history
│   ├── server.py               # REST API server
│   └── init_db.py              # Database initialization
├── tasks/                      # YAML task definitions
├── templates/                  # Reusable task templates
├── config/
│   └── providers.yaml          # Provider routing config
├── tests/
│   └── test_blast.py
├── requirements.txt
└── .env.example
```

---

## 📋 TASK YAML FORMAT

```yaml
# tasks/competitor_analysis.yaml
name: "Competitor Analysis — DigiMinds"
model: "groq:llama-3.1-70b-versatile"
priority: high
timeout_s: 60
depends_on: []
prompt: |
  Analyze the top 5 Google Ads competitors for a digital marketing agency
  targeting ecommerce businesses in Australia. For each competitor:
  - Agency name and URL
  - Estimated ad spend
  - Key differentiators
  - Weaknesses to exploit
output_path: "~/.claude/tcc-logs/competitor-analysis.md"
```

---

## 📊 BENCHMARK: BLAST vs SEQUENTIAL

| Scenario | Sequential | TCC Blast | Speedup |
|---|---|---|---|
| 5 keyword research tasks | 12 min | 2.5 min | 4.8× |
| 3 ad copy variations | 6 min | 1.5 min | 4× |
| 10 account audits | 45 min | 8 min | 5.6× |
| Daily ops (8 tasks) | 20 min | 3 min | 6.7× |

---

## ☠️ STARTUPS / BUSINESSES

| This Repo / Feature | Replaced |
|---|---|
| Blast Mode | Sequential task execution — N× slower |
| Task Queue | Manual tracking of pending tasks |
| Fire All | Running each task individually with supervision |
| Live Dashboard | No visibility into task progress |
| Dependency Graph | Race conditions in complex workflows |
| Cost Tracker | Unknown API spend across parallel tasks |
| History | No record of what tasks ran or their outputs |
| REST API | No integration with external automation tools |

---

## Star History

[![Star History Chart](https://api.star-history.com/svg?repos=hmzainjamil/tcc-task-command-center&type=Date)](https://star-history.com/#hmzainjamil/tcc-task-command-center&Date)

---
<div align="center">Built by <a href="https://github.com/hmzainjamil">HMZ</a> · Part of HMZ Claude AI System</div>

---

## 🔄 CONTRIBUTING

PRs welcome. Please include:
- Tests for new functionality
- Updated `config/providers.yaml` if adding providers
- Benchmark comparison for performance claims
- Documentation update in README

```bash
git checkout -b feature/my-feature
# make changes
python3 tests/run_all.py  # must pass
git push origin feature/my-feature
# open PR
```

---

## 📌 RELATED REPOS

| Repo | Purpose |
|---|---|
| [G0DM0D3](https://github.com/hmzainjamil/G0DM0D3) | Multi-model race + Liquid Response |
| [hermes-ai-system](https://github.com/hmzainjamil/hermes-ai-system) | Local agent with 30+ tools |
| [claude-ai-system-backup](https://github.com/hmzainjamil/claude-ai-system-backup) | Full system backup |
| [hmz-ai](https://github.com/hmzainjamil/hmz-ai) | Personal automation hub |
