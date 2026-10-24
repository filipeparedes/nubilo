# Roadmap

High-level phases for the project. Two categories from the original proposal "REST API & Authentication" (60h) and "Data & Metadata Persistence" (40h) are split across two phases below, because authentication cannot be built
without some persistence in place first.

The hours in the development phases also account for any research/learning that might have to be done.

---

## Core development phases

| # | Phase | Description | Hours 
|---|-------|-------------|------|
| 1 | REST API & Basic Persistence | HTTP server skeleton, routing, and the minimal SQLite schema needed to support it. | ~50h 
| 2 | Authentication & Extended Persistence | Session/token-based auth, user table, and the rest of the metadata/sync-state schema. | ~50h 
| 3 | File Endpoints & Storage | Upload, download, list, and delete endpoints; whole-file SHA-256 hashing; content-addressed storage on disk. Lives in nubilo. | ~60h 
| 4 | File Chunking, Hashing & Deduplication | Splitting files into blocks, block-level hashing, and deduplication. The technical core of the project. | ~90h 
| 5 | File Versioning | Tracking previous versions of a file, enabling rollback/history. | ~50h 
| 6 | Folders, CI & Versioning Completion | Folder support in the API, the CI pipeline, the version listing endpoint and CORS, plus the start of nubilo-web. Lives in nubilo and nubilo-web. | ~60h 
| 7 | nubilo-web | Hosted frontend (nubilo-web) and end-to-end testing. | ~50h 
| 8 | Configuration, Polish & Delivery | Configuration through environment variables, deployment and deployment guide, OpenAPI specification, orphan blob garbage collection, benchmarks, documentation consolidation and automated deployment. Real-time notifications and file sharing remain optional extras. | ~40h

**Subtotal:** ~450h

## Cross-cutting activities

| Activity | Description | Hours | Distribution |
|----------|-------------|------:|---------------|
| Integration & Unit Testing & System Validation | Unit tests per module as it's built, integration tests as modules connect, end-to-end validation once the system is complete. | ~40h | Spread across Phases 1-7, heavier towards Phases 4-5 and a final validation pass after Phase 7. |
| Documentation | ADRs, architecture notes, sprint planning/retrospectives, API/data-model docs as they emerge, final report and defense prep. | ~30h | Continuous from Sprint 0 onward; the last portion concentrated on final report consolidation. |

**Subtotal:** ~70h

## Total: ~520h

## Notes on sequencing decisions

- **Phase 1/2 split:** authentication needs a place to store users and
  sessions, so a minimal persistence layer is built in Phase 1 alongside
  the API skeleton, ahead of the bulk of the "Persistência de dados"
  category, which continues in Phase 2 alongside auth.
- **Phase 3/4 relationship:** the sync client (Phase 3) needs *some* way to
  detect that a file changed, which conceptually belongs to Phase 4's
  hashing work. Phase 3 uses a simple whole-file SHA-256 hash as a
  placeholder for this, which Phase 4 then replaces/extends with proper
  block-level chunking, hashing, and deduplication.
- **Folders before the frontend:** folder support changes the API contract (paths, listing, operations on folders),
  so it is done first in Sprint 6, before the frontend is built on top of it, instead of changing the contract afterwards.
- **Frontend before the client:** the sync client needs the sync state (file-state cache, change detection, conflict handling),
  which was barely started. The project prefers a tangible result, so `nubilo-web` comes right after the backend and
  `nubilo-client` moves to after v1.0. It remains the long-term priority for how a user interacts with their files, through its CLI.
- **CI pipeline early:** the pipeline (build and tests on every push and pull request) starts in Sprint 6, so that the
rest of the project, including the client and the frontend, is built with automatic checks.
- **Phase 8:** a final phase dedicated to configuration, polish and delivery. Configuration through environment variables
  comes first, since deployment depends on it. Then the deployment of the system, the deployment guide, the OpenAPI specification,
  the cleaning of orphan blobs and the measurements (deduplication savings, version savings and upload time with and without the thread pool).
  Automated deployment with GitHub Actions is the last item and moves to a future release if time runs out.
  Real-time notifications and file sharing remain optional extras.

---

# Sprint Roadmap

Each bullet below is a candidate GitHub Issue, kept at a granular enough
level to be tracked and closed individually, while still open-ended enough
to adjust scope during implementation.

## v0.01 (Sprint 0) (~15h)
- Project planning documentation: architecture, ADRs, coding standards, definition of done, roadmap

## v0.1 (Sprint 1) (~68h)
- Validate C++20 modules + `import std` on the target toolchain (CMake + Ninja + clang, hello-world modularized)
- HTTP server setup (basic listener, health-check endpoint)
- Routing structure (route registration, request/response handling)
- JSON serialization/deserialization integration (nlohmann/json)
- SQLite connection setup and migration mechanism
- Basic schema: users table (skeleton), files table (skeleton)
- Consistent API error response format
- Basic request logging

## v0.2 (Sprint 2) (~68h)
- Password hashing integration (library choice + implementation)
- User registration endpoint
- Login endpoint (session/token issuance)
- Session/token validation middleware
- Logout / token invalidation
- Extended schema: sync-state table, file metadata table
- Per-endpoint authorization checks (users only access their own data)

## v0.3 (Sprint 3) (~68h)
- File upload endpoint
- File download endpoint
- List files endpoint
- Delete file endpoint
- Whole-file content storage on disk

## v0.4 (Sprint 4) (~68h)
- File chunking algorithm (splitting files into blocks)
- Block-level SHA-256 hashing
- Chunk manifest structure (per-file list of chunk hashes)
- Chunk storage layer (content-addressed storage on disk)
- Deduplication check on chunk write (skip if hash already stored)

## v0.5 (Sprint 5) (~68h)
- Deduplication across files/users (shared chunk store)
- File reassembly from chunks
- Version metadata table (file version history)
- Version diffing (store only changed chunks per version)
- Rollback mechanism (restore a previous version)

## v0.6 (Sprint 6) (nubilo + nubilo-web) (~68h)
- Folder support (hierarchical paths, folder listing and operations), done first because it changes the API contract
- CI pipeline (build and tests on every push and pull request)
- Versioning completion: version listing endpoint
- CORS support, so the browser frontend can call the API
- Frontend project setup (Vite + React + TS, in `nubilo-web`)

## v0.7 (Sprint 7) (nubilo-web + nubilo) (~67h)
- Frontend: API integration layer (auth, fetching files)
- Frontend: file browser / navigation UI
- Frontend: upload/download UI
- Frontend: file history view, with version comparison if the view needs it
- End-to-end integration testing pass

## v1.0 (Sprint 8) (nubilo + nubilo-web) (~40h)
- Configuration through environment variables (port, database path, blob directory), first
- Deployment of the server and the frontend
- Deployment guide
- OpenAPI specification of the API
- Orphan blob garbage collection
- Benchmarks (deduplication and version savings, upload time with and without the thread pool)
- Documentation consolidation (final report, etc..)
- Final polish and fixes
- Automated deployment with GitHub Actions, last (moves to a future release if there is no time)

## Extras (if there is extra time available)
- Real-time notifications (mechanism TBD: websockets vs polling) 
- File sharing between users (permissions model, share endpoint)
- File/folder moving & renaming

## Unplanned future releases
- Multi-server replication and high availability
- Email verification (send confirmation link/code)
- Centralized logging to daily rotated files
- CLI interface for Nubilo
- `nubilo-client`: the sync client (API client, diff computation, change detection loop, local file-state cache) and the CLI
- Conflict detection for uploads based on an older version
- Trash for deleted files and folders, with a retention period
