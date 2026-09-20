# Sprint 2 - Planning

**Dates:** 20–27 Sep

## Goal
Add authentication (session/token-based) and extend the schema
to support it and future sync state, on top of the API/persistence
foundation built in Sprint 1.

## Scope

- Password hashing integration (library choice + implementation)
- User registration endpoint
- Login endpoint (session/token issuance)
- Session/token validation middleware
- Logout / token invalidation
- Extended schema: sync-state table, file metadata table
- Per-endpoint authorization checks (users only access their own data)

~68h budget (~60h dev + ~8h testing/docs), per docs/roadmap.md.

## Success criteria

- A user can be registered via the API and the record persists in `users`
  (with a securely hashed password, never plaintext)
- A registered user can log in and receive a session/token
- A protected endpoint rejects requests without a valid session/token,
  and accepts them with one
- Logout invalidates the session/token, and a subsequent request with it
  is rejected
- All of the above covered by tests (unit and/or integration), consistent
  with the project's Definition of Done

## Risks / open questions

- Password hashing library not yet chosen, needs a decision early in
  the sprint, since registration/login depend on it
- Session vs token approach (and storage in-memory, or a new DB table)
  not yet decided
- Extending `users`/`files` tables via new migrations should be
  straightforward given Sprint 1's infrastructure, but this will be the
  first real test of adding migrations after the initial schema exists