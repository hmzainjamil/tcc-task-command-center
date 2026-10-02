# Security Notes

TCC stores task descriptions and execution results in local JSON and log files under `~/.claude/tasks` and `~/.claude/tcc-logs`. Depending on the route, task text may also be sent to configured providers, passed to local subprocesses, or posted to Paperclip on `127.0.0.1:3100`.

## Verified risk boundaries

- The default LLM handler interpolates task text into a shell command and invokes it with `shell=True`. The Claude handler also builds a shell command from task text. Do not expose these handlers to untrusted task descriptions; review and harden command construction before doing so.
- Provider-backed routes can transmit task content outside the machine. Inspect `~/.claude/bin/llm-burst` and its provider configuration before submitting private data.
- The workflow handler writes JSON under `~/.claude/workflows` and launches a local workflow runner.
- The Paperclip handler sends task descriptions to a local HTTP service. A loopback address does not establish that the service is trusted or access-controlled.
- `fire all`, `blast`, `retry --fire`, and `tcc-init` can start work or mutate queue state. `purge` deletes task files. `cancel` changes a status field but does not terminate running work.
- Dry-run updates a task to `done` with a dry-run result; it does not preserve pending status.
- The dashboard sources `~/.claude/tier0.env`, checks for key presence, and probes local services and machine state.

## Before using with sensitive work

1. Review the route table and identify the executor for each task.
2. Confirm whether the handler sends text to a provider, subprocess, workflow runner, or Paperclip.
3. Limit file permissions for task, log, route, and credential files.
4. Avoid placing credentials, personal information, or confidential content in task descriptions unless the full route and retention behavior are understood.
5. Review task state before retrying, purging, or firing the queue.

This file is a source-based risk note, not a security audit or guarantee of safety. No security contact or vulnerability disclosure process is declared in the repository. GitHub metadata reports no license for this repository.
