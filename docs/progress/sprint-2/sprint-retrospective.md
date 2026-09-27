# Sprint 2 - Retrospective

**Dates:** 20 - 27 Sep

**Goal:** Add basic authentication (session/token-based) and extend the schema
to support it and future sync state, on top of the API/persistence
foundation built in Sprint 1.

## Done

- Password hashing (Argon2id via libargon2), with random salt generation
  and a general "fallible construction" reuse across the codebase
- SqliteDb extended significantly: parameterized `exec()` and `query()`
  (fixing a SQL injection vector present in the initial registration
  implementation), `lastInsertId()`, and transaction support
  (`beginTx`/`commitTx`/`rollbackTx`, wrapped by a new `DbTransaction`
  RAII type)
- User registration endpoint (`POST /auth/register`), with unique-email
  detection and basic email format validation
- Login endpoint (`POST /auth/login`), issuing a random 32-byte session
  token; deliberately returns the same generic error for "email not
  found" and "wrong password" to avoid leaking account existence
- Session/token validation middleware (`requireAuth`), wrapping route
  handlers so protected routes receive a verified token without
  repeating validation logic
- Logout endpoint (`POST /auth/logout`), the first route to use
  `requireAuth` in practice, invalidating only the specific session tied
  to the token used, not all of a user's sessions
- Extended schema: `sessions`, `file_metadata`, `sync_state` tables
- Per-endpoint authorization principle documented in
  `architecture.md` (no code yet — no user-owned-by-id resources exist
  until Sync/Storage routes are built)

## What changed vs the plan

- **`AuthRouteCallback`'s signature changed mid-sprint**, originally
  designed to pass the authenticated `userId` to protected handlers, but
  switched to passing the session `token` instead once the logout
  endpoint made clear that `userId` alone can't identify *which* session
  to invalidate (a user can have multiple active sessions across
  devices). `userId`, when needed, is always derivable from the token.
- **`RegisterError` was unified into a shared `AuthError`** once the
  login endpoint needed its own error type. Avoided two near-identical
  error structs in the same module.
- **Per-endpoint authorization checks (issue) closed as documentation
  only**.  No file/resource routes exist yet to apply the pattern to;
  the principle is written down in `architecture.md` for when Sync/
  Storage routes are built.
- **`requireAuth` shipped without dedicated tests**, deliberately,
  the following logout issue exercises the same validation path with
  real routes, which was judged sufficient rather than writing tests
  against a synthetic route.

## Blockers / learnings

- No toolchain-level blockers this sprint (a welcome contrast to Sprint
  1's LLVM/modules saga), the friction was entirely conceptual, working
  through how `requireAuth` composes as a function that returns a
  function, and what "sync state" actually represents as a concept
  before it could be turned into a schema.
- The session/sync-state work surfaced a reusable design idea: "wrapper
  function that adapts one callback signature into another" (as used in
  `requireAuth`) is now a pattern to reach for again wherever
  cross-cutting behavior (future: request logging) needs to apply to
  many routes without duplicating logic in each handler.
- Confirmed value of the "fallible construction" pattern from Sprint 1
  (private constructor + static factory returning `std::expected`),
  extended naturally to `DbTransaction` and reused conceptually for
  session/auth error handling.

## Carried over

- None. All planned Sprint 2 issues were completed.