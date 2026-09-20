# Sprint 1 - Retrospective

**Dates:** 12–20 Sep (sprint cadence adjusted from Saturday-to-Saturday to
Sunday-to-Sunday during this sprint; going forward, all sprints run
Sunday to Sunday)

**Goal:** Get a minimal REST API skeleton running, backed by SQLite, with
the C++20 modules toolchain validated.

## Done

- Validated C++20 modules toolchain support, found unstable in
  combination with non-modularized third-party libraries (cpp-httplib)
  under sanitizer builds; migrated the whole codebase to traditional
  headers instead (see below)
- HTTP server setup (cpp-httplib), with a `/health` endpoint
- Routing structure: centralized `Router` decoupling modules from
  `httplib::Server` directly (ADR 0004)
- JSON serialization integration (nlohmann/json), applied to `/health`
- SQLite connection (`SqliteDb`) with RAII lifecycle and `std::expected`
  based error handling
- Migration mechanism (`DbMigrator`), transactional (`DbTransaction`) so
  a failed migration leaves the schema untouched rather than half-applied
- Basic schema: `users` and `files` tables (skeleton, no auth/content
  columns yet)
- Consistent JSON error response format (`writeErrorResponse`), ready for
  endpoints that need it starting Sprint 2
- Test framework (GoogleTest) set up and used throughout, every piece
  above has unit and/or integration test coverage
- `.clang-format` added and corrected (`AccessModifierOffset`)

## What changed vs the plan

- **C++20 modules reverted to traditional headers mid-sprint**, the
  single biggest deviation. Modules combined with cpp-httplib caused
  toolchain-level failures (ABI tag redeclaration errors under
  AddressSanitizer/UBSan, `<regex>` redefinition errors) that persisted
  across LLVM version changes. Documented in ADR 0005; adopted a mirrored `include/` +
  `src/` folder structure as part of the same change.
- **Router introduced as a deliberate abstraction over cpp-httplib**
  (ADR 0004), not originally planned as its own decision point, but
  emerged naturally once route registration needed a central place to
  live, decoupled from the HTTP library.
- **`SqliteDb`/`DbMigrator` error handling switched from exceptions to
  `std::expected`** partway through, to align with the project's own
  coding standards (exceptions reserved for truly unrecoverable cases).
  This also introduced a general "fallible construction" pattern (private
  constructor + static factory returning `std::expected`, non-copyable
  but movable) now documented for reuse in future modules.
- **Transactions (`DbTransaction`) added** beyond the original scope of
  "set up SQLite connection and migrations" — recognized during
  implementation that applying a migration's SQL and recording it as
  applied were two separate writes that could be left inconsistent by a
  crash between them.
- **Generic `SqliteDb::query()` returning JSON** was built instead of a
  narrower int-only query helper, anticipating near-term needs (Auth,
  Sync, Storage will all need to read rows of varying shapes) rather than
  building a single-purpose method now and a generic one later.

## Blockers / learnings

- Significant time lost to toolchain issues unrelated to project logic:
  LLVM version conflicts (Homebrew's `llvm` vs keg-only `llvm@18`),
  linker failures, and the modules/sanitizer incompatibility that
  ultimately drove the headers migration. This is the clearest evidence
  so far that environment/tooling risk can consume real sprint time
  independent of feature complexity.
- RAII + `std::expected` as a combined pattern (rather than exceptions)
  required more upfront design (move semantics, private constructors,
  factory methods) but paid off in clarity once established, reused
  cleanly across `SqliteDb`, `DbTransaction`, and will extend to future
  modules.


## Carried over

- None. All planned Sprint 1 issues were completed.