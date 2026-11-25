# WORKFLOW

Working rules for this fork. All GitHub work targets `limapedro/pytorch-vulkan` only.
**Never** open PRs, issues or comments against `pytorch/pytorch`.

## Planning

- One milestone per phase; milestones 2-9 already exist (Phase 0-7).
- Issue -> branch -> PR; one focused unit of work per PR, small PRs.
- Branch prefixes: `feat/`, `perf/`, `docs/`, `exp/`, `fix/`.
- Base branch for every PR: `stable/v2.15.0` (the fork default). Never force-push it.
- Docs are updated in the same PR as the code they describe. A doc that disagrees with
  code is a bug.

## PR body checklist

Every PR body contains:

1. Summary.
2. Linked issue.
3. Linked `HYPOTHESIS.md` / `FINDINGS.md` when the PR is an experiment.
4. Parity test result (or explicit "not applicable").
5. Before/after benchmark table when performance is touched (no numbers => no claim).
6. Docs updated list.

## Merge gates

- Build green (cheap PR checks: lint, docs checks, shader compile, op tests on Mesa
  lavapipe or the CPU Vulkan implementation where feasible; full build nightly/manual).
- Parity tests pass; no merge on a parity failure.
- No unexplained performance regression above 3% vs the last recorded number.
- Self-review checklist completed. Squash-merge only after gates pass.
- Close stale/superseded PRs with a written reason. Tag milestones when completed.

## AI authorship and the inherited policy conflict

The upstream `CLAUDE.md` in this fork says an agent may never act autonomously on
GitHub and must mark all AI-generated text in issues/PRs. The project mission explicitly
grants autonomous GitHub permission and requires issue/PR workflows. Resolved by
decision D004 in `ARCHITECTURE_LOG.md`: follow the mission, but wrap AI-authored
issue/PR bodies in a quote block, disclose AI authorship, and keep every change
reviewable in the PR diff before squash merge. If the user revokes this authorization,
revert to the repo policy immediately.

## Commits

Do not commit unless the user explicitly asks. When committing, include a Test Plan
with the exact commands run and disclose AI authorship, per `CLAUDE.md`.
