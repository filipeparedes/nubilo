# Sprint 1 — Planning

## Goal
Get a minimal REST API skeleton running, backed by SQLite, with the C++20
modules toolchain validated. By the end of this sprint, the server should
start, respond to a health-check, and have a basic (not yet user-specific)
database schema in place.

## Scope
- Validate C++20 modules + `import std` on the target toolchain (CMake + Ninja + clang, hello-world modularized)
- HTTP server setup (basic listener, health-check endpoint)
- Routing structure (route registration, request/response handling)
- JSON serialization/deserialization integration (nlohmann/json)
- SQLite connection setup and migration mechanism
- Basic schema: users table (skeleton), files table (skeleton)
- Consistent API error response format
- Basic request logging

~68h budget (~60h dev + ~8h testing/docs).

## Success criteria
- `curl localhost:<port>/health` returns a 200 with a valid JSON body
- A row can be inserted and queried in both `users` and `files` tables via the running server (even if only through a temporary/manual test endpoint)
- `./tests` runs at least one passing GoogleTest case
- Server builds and runs cleanly on the target toolchain with zero warnings

## Risks / open questions
- Migration mechanism for SQLite schema changes isn't designed yet - deciding between a hand-rolled versioned migration table vs. a lightweight existing library.
- Cross-platform requirement (Windows + Linux + macOS) means avoiding any OS-specific APIs from the start, even though development happens on macOS.