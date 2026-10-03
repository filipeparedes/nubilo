# Architecture

## Overview

Nubilo is a personal file synchronization system, built from scratch in C++,
inspired by tools like Dropbox but intentionally scoped down: no
subscriptions, no unnecessary features — just the essentials for keeping
files in sync across devices.

This document describes the high-level architecture and the reasoning behind
it. Individual decisions with more detailed trade-off analysis live in
[`decisions/`](./decisions) as ADRs (Architecture Decision Records).

---

## Repository split

The project is split across three repositories:

- **`nubilo`** (this repository) - the core: a standalone synchronization
  API written in C++. This is the actual subject of the project.
- **`nubilo-client`** - a local executable, run on the end user's own
  machine: a CLI plus the real sync client (file-state cache, change
  detection, diffing, conflict groundwork). This is the primary way a
  user interacts with their synced files, closer in spirit to git/gh
  than a typical desktop app.
- **`nubilo-web`** - an optional, hosted React frontend, for interacting
  with files through a browser without installing anything locally. It
  talks to `nubilo` directly over HTTP and shares no code with
  `nubilo-client`.

See [ADR 0006](./decisions/0006-three-repo-split.md) for the reasoning
behind this split, and [ADR 0001](./decisions/0001-separate-repos-backend-frontend.md)
(now superseded) for the original two-repo decision.

## High-level structure

The backend is a **modular monolith**: a single C++ process, internally
organized into clearly separated modules, rather than a distributed set of
microservices. See [ADR 0002](./decisions/0002-modular-monolith-vs-microservices.md)
for why this was chosen over a microservices approach.

```
           
          Client (external, any HTTP consumer)
                      |
                      v
+------------------------------------------------------------+
|                     Server (C++)                           |
|                                                            |
|   +----------------+                                       |
|   |   API layer    |  HTTP routing (cpp-httplib)           |
|   +----------------+                                       |
|          |                                                 |
|   +------------+  +-----------------+  +----------------+  |
|   |    Auth    |  |    Blob         |  |    Storage     |  |
|   | sessions,  |  | files, content- |  |    SQLite      |  |
|   | tokens     |  |  adressed       |  |                |  |
|   +------------+  +-----------------+  +----------------+  |
+------------------------------------------------------------+
```

### Modules

- **API layer** — receives HTTP requests, routes them to the appropriate
  module, and formats responses (JSON via nlohmann/json).
- **Auth** — handles sessions/tokens and authorization checks.
- **Storage** — persists all application metadata in SQLite: user accounts, sessions, sync-state and file metadata.
- **Blob** - persists the actual file content on disk, as content-addressed storage.

Each module is expected to expose a narrow internal interface, so that if the
project ever needed to extract a module into its own service (e.g. the sync
engine as a background daemon), the boundary is already there.

---

## Storage

The `Db` class persists all application metadata: user accounts,
sessions, and file metadata (paths, sizes, hashes, ownership, sync
state, and, starting in Phase 4, chunk manifests). It wraps a single
SQLite connection with RAII: opened once at startup, closed
automatically on shutdown. Schema changes are applied through
`DbMigrator`, which tracks which migrations have already run in a
`schema_migrations` table and applies any new ones (defined in
`Migrations.h`) in order, each wrapped in a transaction
(`DbTransaction`) so a failure partway through a migration leaves the
schema untouched rather than half-updated.

Reads and writes go through `Db::exec()` for statements with no
result set, and `Db::query()` for `SELECT`s, which returns rows as
JSON, one object per row, keyed by column name, rather than a bespoke
result-set type, since `nlohmann::json` already handles values of
different types across rows and columns.
--- 

## Blob Storage

The `BlobStore` class (`blob/`) persists file content on disk, as
content-addressed storage (files named by their own SHA-256 hash) - the
same pattern Git and Dropbox use internally.

The database never stores file content directly as a BLOB, it only
holds a reference to it (the hash). This keeps the database small and
fast regardless of how much data is synced, and is a prerequisite for
chunk-level deduplication (Phase 4): a chunk can be shared across files
and users by reference (its hash) without duplicating it in the
database.

---

## Authorization pattern

Authentication (requireAuth) confirms *who* is making a request.
Authorization confirms *what* they're allowed to access, these are
different checks and both are required wherever a route operates on a
specific resource (a file, a session, etc.) identified by an id.

Any route that fetches or modifies a resource by id must verify that the
resource belongs to the authenticated user (typically: the resource's
owner_id/user_id column matches the id resolved from the request's
token) before acting on it. Never trust an id from the request path or
body alone. A resource not found and a resource that exists but belongs
to someone else should both return 404, not 403, returning 403
confirms to an attacker that the resource exists.

---

## Tech stack

- **Language:** C++23
- **HTTP layer:** cpp-httplib / Crow
- **Serialization:** nlohmann/json
- **Database:** SQLite
- **Change detection:** SHA-256
- **File hashing:** SHA-256
- **Password hashing:** Argon2
- **Build system:** CMake
- **Development platform:** macOS

## Workflow

Development follows a GitHub Flow style workflow (`main` + short-lived
feature/docs branches), tracked via GitHub Issues and Milestones. See
[ADR 0003](./decisions/0003-git-workflow.md) for details.
