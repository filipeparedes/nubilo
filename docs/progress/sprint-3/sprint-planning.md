# Sprint 3 - Planning

**Dates:** 27 Sep - 4 Oct

**Goal:** Build the sync client's core: detect local file changes and
communicate them to the server, using a simple whole-file hash for now
(full chunking arrives in Phase 4).

## Scope

- Local file-state cache (tracked files + last known hash)
- Whole-file SHA-256 hashing utility
- Change detection loop (compare local state vs cache)
- API client for server communication (upload/download changed files)
- Diff computation between local and remote state
- Groundwork for conflict detection (flagging, not resolving yet)

~68h budget (~60h dev + ~8h testing/docs), per docs/roadmap.md.

## Success criteria

- Given a tracked local file that changes, the sync client detects the
  change without re-scanning/re-hashing files that haven't changed
- The client can report a changed file's new hash to the server via the
  API and receive confirmation
- A file changed on both sides between syncs is flagged as a conflict,
  not silently overwritten in either direction
- Covered by tests consistent with the project's Definition of Done

## Risks / open questions

- This introduces the project's first genuinely new subsystem (sync/)
  with no prior scaffolding, unlike Sprint 2, which mostly extended
  existing modules (storage/, api/)
- File I/O and hashing on real files (not just SQLite/HTTP as before)
  may surface new cross-platform considerations (path handling, file
  locking) given the Windows/Linux/macOS requirement
- Conflict detection "groundwork" is deliberately scoped to flagging
  only, full resolution strategy is an open design question for a
  later phase, not this sprint