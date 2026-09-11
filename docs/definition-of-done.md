# Definition of Done

This document defines the minimum bar a piece of work must clear before it
can be considered complete and merged into `main`. It applies uniformly
across the whole project. If this document ever needs to change, that change is
itself recorded as an ADR, not a silent edit.

A task is **done** when all of the following are true.

## 1. Correctness

- [ ] The feature/fix behaves as described in its issue, including edge
      cases identified during implementation (not just the happy path).
- [ ] No known regression was introduced in previously working
      functionality - verified by re-running existing tests.
- [ ] Error conditions are handled explicitly (no silent failures, no
      unchecked return values from I/O, file, or DB operations).

## 2. Code quality

- [ ] Compiles cleanly, without any warnings or errors
- [ ] No obvious memory-safety issues: run through AddressSanitizer /
      UndefinedBehaviorSanitizer at least once for any code touching raw
      pointers, manual memory management, or concurrency.
- [ ] The code should be Doxygen documented and commented.
- [ ] No dead code, commented-out blocks, or leftover debug prints left in
      the final commit.
- [ ] Follows the module boundaries defined in `architecture.md` - logic
      lives in the module it conceptually belongs to (e.g. hashing logic
      doesn't leak into the API layer).

## 3. Testing

- [ ] Unit tests exist for new non-trivial logic (parsing, hashing,
      diffing, conflict resolution, etc.) and pass locally.
- [ ] If the change affects how modules interact (e.g. API ↔ Sync engine ↔
      Storage), at least one integration-level test or manual end-to-end
      check confirms the interaction works.
- [ ] Tests are deterministic and should have no reliance on timing, external network
      state, or execution order to pass.

## 4. Documentation

- [ ] If the change affects architecture, introduces a new significant
      decision, or changes an existing one — an ADR is written or an
      existing one superseded, not just mentioned in a commit message.
- [ ] If the change adds or modifies an API endpoint — `docs/api.md` is
      updated in the same PR (not "later").
- [ ] If the change affects the database schema — `docs/data-model.md` is
      updated in the same PR.
- [ ] The relevant sprint's `sprint-planning.md` / progress notes reflect
      that the task is done (or explicitly carried over, with a reason).

## 5. Git & review hygiene

- [ ] Work happened on a branch named per
      [ADR 0003](./decisions/0003-git-workflow.md)'s convention, off `main`.
- [ ] Commits follow Conventional Commits and are reasonably atomic — a
      commit does one describable thing.
- [ ] Merged via PR into `main`, with a description explaining *what* and
      *why* (not just *what*), referencing its issue (`Closes #N`).
- [ ] `main` still builds and runs after the merge

---

**Not required**, to keep this realistic under the project's timeline:
100% test coverage, exhaustive documentation of trivial/internal helper
functions, or performance benchmarking unless a phase's goal specifically
concerns performance (e.g. deduplication efficiency in Phase 4).