# 0001 - Separate repositories for backend and frontend

## Context

Nubilo needs both a backend (the actual synchronization system) and a
frontend (a client to interact with it). The initial instinct was to keep
both in a single monorepo, since this is a solo project and a monorepo
usually reduces coordination overhead.

However, the backend is the actual subject of this project: the frontend is
an optional, secondary demonstration client. Keeping them in one repository
risked creating an implicit coupling between the two (shared folders,
temptation to import across boundaries, README and issue tracker mixing
concerns from both sides), which would blur that distinction.

## Decision

Use two separate repositories:

- `nubilo` - the backend API, the core deliverable of the project.
- `nubilo-client` - an optional frontend client that consumes the API over
  HTTP.

## Reasoning

- The backend can be developed, tested, and evaluated in complete isolation,
  with no dependency on frontend code or tooling.
- The frontend is clearly communicated as optional - a demonstration client, not a required part of the
  software.
- Physical separation forces the only allowed interaction between the two to
  be the HTTP API itself, rather than shared code or folders.
- Each repository has its own clean toolchain (CMake/C++ vs npm/TypeScript)
  without one bleeding into the other's setup instructions.

## Trade-offs accepted

- No single commit can span both backend and frontend changes; keeping the
  two in sync (e.g. after an API change) requires a bit more manual
  coordination than a monorepo would.
- Two repositories to set up and maintain instead of one — acceptable given
  there is no CI/deployment pipeline that would need coordinating across them.
