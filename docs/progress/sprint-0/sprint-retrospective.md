# Sprint 0 - Retrospective

## What was done
- Repository structure and initial scaffold set up (`nubilo`, standalone backend)
- `docs/architecture.md` written: system overview, module breakdown, tech stack
- ADRs written: 0001 (separate repos for backend/frontend), 0002 (modular monolith vs microservices), 0003 (git workflow)
- `docs/coding-standards.md` written: naming, C++20 modules conventions, memory management (RAII), formatting (K&R, 4 spaces, 100 cols), const-correctness, error handling (`std::expected`)
- `docs/definition-of-done.md` written: correctness, code quality, testing, documentation, git hygiene
- `docs/roadmap.md` written: phase breakdown mapped to proposal hours, sprint-by-sprint issue-level breakdown
- Initial `README.md` written
- GitHub setup: 8 milestones (Sprint 0–7) with due dates, all sprint issues created (43 total)
- `progress/sprint-1/sprint-planning.md` written

## What changed vs the plan
- Originally considered a single `progress.md` log; switched to a `progress/sprint-N/` folder structure with separate planning/retrospective files per sprint
- Originally considered per-sprint Definition of Done; decided on a single global DoD instead, changeable only via ADR
- Originally considered tracking sprints via GitHub labels; settled on Milestones instead, to avoid redundancy
- Decided to use C++20 modules instead of traditional header/source files


## Blockers / learnings
- None yet - this sprint was documentation-only, no implementation attempted
- Open technical risk carried into Sprint 1: C++20 modules + `import std` support on the target toolchain is unverified

## Carried over
- Nothing carried over from planned scope , all Sprint 0 documentation goals were completed