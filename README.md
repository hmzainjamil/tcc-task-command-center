# TCC: Task Command Center

TCC is a local Python CLI that stores task records as JSON, routes tasks by keyword, and can execute selected local or provider-backed handlers in parallel. The repository also includes a dashboard and a script that seeds task templates.

## Repository map

| File | Purpose |
|---|---|
| `tcc` | Task queue, routing, execution, retry, cancellation, purge, and watch commands |
| `tcc-dashboard` | Displays local provider-key presence, Paperclip reachability, queue, workflows, registry, launch agents, disk, RAM, and cache size |
| `tcc-init` | Queries local Paperclip endpoints and adds eight preset tasks to the local queue |
| `README.md` | Setup scope, commands, behavior, and limitations |
| `SECURITY.md` | Data handling and operational risks |

## Requirements and local state

The main CLI uses Python's standard library. It also calls external programs and services depending on the selected route, including `~/.claude/bin/llm-burst`, `claude`, `~/.claude/bin/workflow-dag`, Ollama, and a local Paperclip API at `127.0.0.1:3100`. Those tools and their configuration are not included in this repository. No dependency manifest or automated tests are present. The repository scripts `tcc-dashboard` and `tcc-init` also call `~/.claude/bin/tcc`; running them from this checkout still requires that installed path and may use a different TCC copy than `./tcc`.

On startup, the CLI creates `~/.claude/tasks` and `~/.claude/tcc-logs`. Task records are JSON files in the task directory; execution logs are written to the log directory. The first route lookup writes a default route table to `~/.claude/tcc-routes/routes.json`. Override this table only after reviewing its routing behavior.

## Command reference

Run the executable from the repository root. Examples below document source behavior; they have not been run during this review.

```bash
./tcc routes
./tcc add "draft a project brief" --priority med
./tcc list --status pending
./tcc status TASK_ID
./tcc fire TASK_ID --dry-run
./tcc fire all
./tcc blast "task one" "task two" --dry-run
./tcc retry failed
./tcc cancel TASK_ID
./tcc purge --status failed
./tcc watch
./tcc-dashboard
./tcc-init
```

Important command semantics:

- `add` stores a pending task. `add --fire` also executes it.
- `fire all` runs all pending tasks in parallel. A task may call a provider, external CLI, workflow runner, or local Paperclip API based on its route.
- `fire --dry-run` does not execute the task handler, but it marks the task `done` with a `[dry-run]` result. It is a state-changing simulation.
- `blast` creates tasks and fires them immediately. Its `--dry-run` option still changes task records to done.
- `cancel` changes the recorded status to `cancelled`; it does not stop an already running subprocess.
- `retry` resets failed or cancelled tasks to pending. `retry --fire` also runs them.
- `purge` deletes task JSON files matching a status. It defaults to failed tasks.
- `tcc-init` queries Paperclip and adds eight preset tasks. It does not fire them automatically.
- `tcc-dashboard` reads local configuration/state and probes Ollama, Paperclip, launchd, and system resources.

## Routing and execution

The default route table matches keywords in task descriptions and selects handlers such as `tier0-blast`, `paperclip`, `workflow-dag`, `claude-code-subagent`, or `apify`. Routing labels are not proof that every referenced tool, skill, API, or model is installed or available.

The Apify handler currently returns a message describing a possible actor invocation; it does not run an actor. Other handlers can make network requests, invoke local commands, or create workflow files. The TCC route table is stored outside the repository and can override defaults.

## Data and safety

Task descriptions are stored locally and may be passed to external providers or subprocesses. The default LLM handler invokes `llm-burst`; other handlers can call `claude`, `workflow-dag`, or Paperclip. See [`SECURITY.md`](SECURITY.md) before processing private or untrusted task text.

## Validation and release status

The README documents the checked-in source, not a verified deployment. No scripts, providers, queues, workflows, dashboard, or Paperclip services were run for this review. The repository has no test suite, dependency manifest, or license file; GitHub metadata reports no declared license. Do not infer permission to reuse or redistribute its contents.

## Maintainer

[hmzainjamil](https://github.com/hmzainjamil)
