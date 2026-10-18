# Sprint 5 - Retrospective

**Dates:** 11 - 18 Oct

**Goal:** Add file versioning on top of the chunk storage built in Sprint 4:
keep the history of each file as a list of versions, store only the chunks
that changed between versions, and allow restoring a previous version.
Also verify the deduplication and reassembly work that Sprint 4 already
covers, and document the project's use of AI.

**Time:** ~50h against a ~68h budget. Most of the effort went into the
versioning model, as expected. Deduplication and reassembly needed only
verification, which left room for unplanned work (see below).

## Done

- Deduplication across files and users (#28): verified against the roadmap.
  The shared chunk store from Sprint 4 already stores identical chunks once,
  and this is covered by tests across files and across owners
- File reassembly from chunks (#29): verified. `readFile` rebuilds a file
  through its manifest, and large multi-chunk files are covered by tests
- Version metadata table (#30): new `file_versions` table (file id, version
  number, size, content type, creation time). The chunk manifest
  (`file_chunks`) now belongs to a version instead of a file, and a file is
  identified by its owner and path (`UNIQUE (owner_id, path)`). Migrations 11
  to 14 apply the change, and existing files become version 1. Recorded in
  ADR 0008
- Version diffing (#31): uploading to a path that already exists adds a new
  version of the same file instead of creating a new one. Chunks that did not
  change are the same blobs, so only the changed chunks take new space. Closed
  together with #30, in the same pull request
- Rollback mechanism (#32): `rollbackFile` and the endpoint
  `POST /files/{id}/versions/{version}/restore`. Restoring adds a new version
  that copies the size, content type and chunk list of the chosen one, so the
  history is kept and no chunk is written
- `readFile`, `listFiles` and `deleteFile` work on versions: reads and
  listings use the latest version, and deleting a file removes all its
  versions and manifests
- Document AI usage in the project (#78): `docs/ai-usage.md`, following the
  UNIPS guidelines for generative AI
- Update documentation (#89): root README rewritten, roadmap and
  architecture updated, and the module READMEs, API endpoint table, storage
  schema and bibliography brought up to date with versions and rollback
- Tests for each versioning case: new versions, shared chunks between
  versions, latest version on read and list, deletion of all versions,
  migration of existing files, and rollback (content, history, size and
  content type, chunk list without new blobs, rolling back a rollback, and
  errors for a missing file or version). Route tests cover the restore
  endpoint
- Outside the original scope: a thread pool for concurrent chunk storage
  (see below), the issues for `nubilo-web` for Sprint 7, and the
  restructuring of the roadmap (see below)

## What changed vs the plan

- **The open questions in the planning were answered.** The manifest lives
  in the version, and a file points to its current version by having the
  highest version number. An upload to a path that the owner already has adds
  a version, and any other upload creates a file. Chunks are still referenced
  from `file_chunks`, so references to a chunk can be counted by the future
  orphan blob garbage collection.
- **#30 and #31 were done as one piece of work.** With the manifest moved to
  versions and chunks shared by hash, storing only the changed chunks follows
  from the model, so a separate implementation would have meant changing the
  same code twice.
- **Rollback includes the endpoint.** #32 covers the function and the route
  that exposes it. Version listing and comparison stay in Sprint 6.
- **Columns that stopped making sense were dropped.** `files` no longer holds
  size or content type, since they belong to each version. As in Sprint 4,
  the tables were rebuilt in the same sprint instead of leaving leftover
  columns, because SQLite cannot drop a column that has a `REFERENCES`
  clause. Migration 14 fails if the database already has two files with the
  same owner and path, which is documented.
- **A thread pool was added as unplanned work.** `ThreadPool` (new
  `concurrency` module) lets `storeFile` write the chunks of a file at the
  same time. It is optional: without a pool, the chunks are written one after
  another, so the server still works if the pool cannot be created. It
  reports failures with `std::expected`, like the rest of the project, and has
  its own tests.
- **The roadmap was restructured.** A Sprint 8 for polish and delivery was
  added (deployment and deployment guide, OpenAPI specification, orphan blob
  garbage collection and benchmarks). Folder support moved to Sprint 6, before
  `nubilo-client`, because it changes the API contract, and the CI pipeline
  starts in Sprint 6. File sharing and real-time notifications stay as
  optional extras and left Sprint 7.
- **Features are not issues.** As in Sprint 4, the focus stays on testing,
  hardening and documenting what exists before adding features. Pulling
  planned work forward is the only kind of unplanned work taken on.

## Blockers / learnings


- `storeFile` hands the chunks to the workers as `std::string_view` slices of
  the file content. It must wait for every task to finish before returning,
  even when one of them fails, otherwise the workers would read memory that
  is already gone. The AddressSanitizer build is what protects this.
- Foreign keys are still declared but not enforced, so deleting a file now
  removes rows in a fixed order: chunk lists, versions, then the file. The
  versions added one more level to that order.
- `storeFile` grew to around 120 lines, mixing the writing of chunks to disk
  with the database registration. The two phases are already separated by the
  transaction, so splitting it is a natural refactor.
- The tests for the thread pool path of `storeFile` do not cover the route
  level: `main` creating the pool and the routes passing it on is checked by
  running the server, not by an automated test.

## Carried over

- None.