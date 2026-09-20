# Architecture

## Overview

Nubilo is a personal file synchronization system, built from scratch in C++,
inspired by tools like Dropbox but intentionally scoped down: no
subscriptions, no unnecessary features — just the essentials for keeping
files in sync across devices.

This document describes the high-level architecture and the reasoning behind
it. Individual decisions with more detailed trade-off analysis live in
[`decisions/`](./decisions) as ADRs (Architecture Decision Records).

## Repository split

The project is split across two repositories:

- **`nubilo`** (this repository) — the core: a standalone synchronization
  API written in C++. This is the actual subject of the project.
- **`nubilo-client`** — an optional frontend (React + TypeScript) that
  consumes the API. It exists mainly as a demonstration client, not as a
  core deliverable.

See [ADR 0001](./decisions/0001-separate-repos-backend-frontend.md) for the
reasoning behind this split.

## High-level structure

The backend is a **modular monolith**: a single C++ process, internally
organized into clearly separated modules, rather than a distributed set of
microservices. See [ADR 0002](./decisions/0002-modular-monolith-vs-microservices.md)
for why this was chosen over a microservices approach.

```
           
          Client (external, any HTTP consumer)
                      |
                      v
+-------------------------------------------------------+
|                     Server (C++)                      |
|                                                       |
|   +----------------+                                  |
|   |   API layer    |  HTTP routing (cpp-httplib)      |
|   +----------------+                                  |
|          |                                            |
|   +------------+  +------------+  +----------------+  |
|   |    Auth    |  |    Sync    |  |    Storage     |  |
|   | sessions,  |  | SHA-256    |  | SQLite + files |  |
|   | tokens     |  | diffing    |  |                |  |
|   +------------+  +------------+  +----------------+  |
+-------------------------------------------------------+
```

### Modules

- **API layer** — receives HTTP requests, routes them to the appropriate
  module, and formats responses (JSON via nlohmann/json).
- **Auth** — handles sessions/tokens and authorization checks.
- **Sync engine** — detects file changes (via SHA-256 hashing), computes
  diffs, and decides what needs to be transferred between client and server.
- **Storage** — persists file metadata in SQLite and manages the actual file
  contents on disk.

Each module is expected to expose a narrow internal interface, so that if the
project ever needed to extract a module into its own service (e.g. the sync
engine as a background daemon), the boundary is already there.

## Storage

The storage module is responsible for everything the system needs to
persist: user accounts, file metadata, sync state, and, starting in
Phase 4, the file content itself. It's split into two distinct pieces,
each backed by a different mechanism.

### SQLite for metadata

The `SqliteDb` class wraps a single SQLite connection with RAII: opened
once at startup, closed automatically on shutdown. Schema changes are
applied through `DbMigrator`, which tracks which migrations have already
run in a `schema_migrations` table and applies any new ones (defined in
`Migrations.h`) in order, each wrapped in a transaction (`DbTransaction`)
so a failure partway through a migration leaves the schema untouched
rather than half-updated.

Reads and writes go through `SqliteDb::exec()` for statements with no
result set, and `SqliteDb::query()` for `SELECT`s, which returns rows as
JSON, one object per row, keyed by column name, rather than a bespoke
result-set type, since `nlohmann::json` already handles values of
different types across rows and columns.

### Metadata vs content separation

The database (SQLite) stores only metadata, file paths, sizes, hashes,
ownership, sync state, and (starting in Phase 4) chunk manifests. It never
stores file content directly as a BLOB.

Actual file bytes live on the filesystem, managed by the application
itself as content-addressed storage (files named by their own hash) — the
same pattern Git and Dropbox use internally. This keeps the database small
and fast regardless of how much data is synced, and is a prerequisite for
chunk-level deduplication (Phase 4): a chunk can be shared across files and
users by reference (its hash) without duplicating it in the database.

## Tech stack

- **Language:** C++23
- **HTTP layer:** cpp-httplib / Crow
- **Serialization:** nlohmann/json
- **Database:** SQLite
- **Change detection:** SHA-256
- **Build system:** CMake
- **Development platform:** macOS

## Workflow

Development follows a GitHub Flow style workflow (`main` + short-lived
feature/docs branches), tracked via GitHub Issues and Milestones. See
[ADR 0003](./decisions/0003-git-workflow.md) for details.
