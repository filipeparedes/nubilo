# 0003 - Git workflow

## Context

A workflow was needed for branching, commits, and task tracking that fits a
solo developer working within sprints, while still producing a clean,
traceable history — useful both during development and later when writing
the final report and preparing for the defense.

Git Flow (with a permanent `main` + `dev` branch split) was considered, but
a permanent `dev` branch mainly pays off when multiple people need a buffer
between integration and release. For a solo project, it tends to become a
mirror of `main` with an extra sync step and no real protective benefit.

A permanent documentation branch was also considered, to keep `docs/` work
separate from code. This was rejected for the same reason: documentation
describes the current state of the code, so it makes more sense to live
alongside it on `main`, rather than on a branch that can drift out of sync.

## Decision

- **Branching:** GitHub Flow style. `main` is the only permanent branch and
  is expected to always be in a functional state. All work happens on
  short-lived branches created from `main`.
- **Branch naming:** `<type>/<issue-number>-<short-description>`, e.g.
  `feature/12-auth-module`, `docs/1-project-planning`. The type prefix
  matches the issue's label (`feature`, `docs`, `fix`, `chore`).
- **Issues:** each unit of work (feature, bug, documentation task) is
  tracked as a GitHub Issue. Issues are kept intentionally high-level and
  open-ended rather than rigid checklists, to leave room to adjust scope
  while working.
- **Milestones:** issues are grouped into milestones representing sprints
  (e.g. "Sprint 1"), giving a visual progress indicator per sprint.
- **Pull requests:** even solo, work is merged via PRs rather than direct
  pushes to `main`. The PR description doubles as lightweight documentation
  of what changed and why. PR descriptions reference their issue (e.g.
  `Closes #12`) so the issue closes automatically on merge.
- **Commit messages:** follow the Conventional Commits format:
  `feat: ...`, `fix: ...`, `docs: ...`, `refactor: ...`, `chore: ...`.
- **Tags:** at the end of each sprint, once `main` is in a presentable
  state, it gets tagged (e.g. `v0.1`, `v0.2`) as a checkpoint.

## Reasoning

- Keeps `main` simple to reason about: it's always the current, working
  state of the project — no need to remember which of two branches is
  "ahead."
- Issue/milestone/branch naming ties every piece of work back to a
  traceable unit, useful when reconstructing the project's history for the
  final report.
- PR descriptions and Conventional Commits produce documentation as a
  side-effect of normal development, at close to no extra cost.
- Tags give clear, citable checkpoints per sprint without needing a
  dedicated release branch.

## Trade-offs accepted

- Slightly more ceremony than pushing straight to `main` (opening a PR even
  when working alone), in exchange for a cleaner, more reviewable history.
