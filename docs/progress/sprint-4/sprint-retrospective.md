# Sprint 4 - Retrospective

**Dates:** 4 - 11 Oct

**Goal:** Break stored files into blocks and store them by hash, with
deduplication on write (the technical core of the project), and close two
loose ends from the earlier sprints: session expiration and thread safety
of the shared database connection.

**Time:** ~60h against a ~70h budget. The chunking work took less than
expected: the chunker is small, and block hashing turned out not to need a
module of its own (see below).

## Done

- File chunking algorithm (`chunkContent`): splits content into fixed-size
  chunks of 4 MiB, returned as `std::string_view` slices of the original
  content, so nothing is copied. Empty content produces no chunks
- Block-level SHA-256 hashing: done by `BlobStore::store`, which now takes
  a `std::string_view` so chunks can be stored without copying
- Chunk manifest: new `file_chunks` table (file id, position, chunk hash)
  that lists the chunks of each file in order (migration 9). Existing files
  were converted to a single chunk at position 0 by the same migration
- Chunk storage layer: `storeFile`, `readFile`, `listFiles` and `deleteFile`
  now work through the manifest. A file is split into chunks, each chunk is
  written to the blob store, and the file and its manifest are registered in
  a single transaction. `readFile` rebuilds the file by reading the chunks
  in order, `listFiles` computes the size from the chunks and `deleteFile`
  removes the manifest together with the file
- Deduplication check on chunk write: identical chunks are stored once,
  across files and users. The write is skipped on disk when the blob already
  exists and in the database with `INSERT OR IGNORE`
- Schema: `content_type` moved from `blobs` to `files`, since a chunk has no
  content type of its own. Migration 10 rebuilds `files` without
  `content_hash` and drops `blobs.content_type`, keeping the ids and content
  types of existing files
- Session expiration: sessions expire 7 days after login (`expires_at`,
  migration 8). An expired session is rejected by `requireAuth` and removed
  on that request
- Thread safety of `Db`: `exec`, `query` and `lastInsertId` lock a recursive
  mutex, and `Db::lock()` lets callers hold the lock across a sequence of
  calls. Applied to `registerUser` and `storeFile`. `SQLITE_THREADSAFE=1` is
  now set explicitly for the sqlite3 target
- Unit tests for the chunker, the known SHA-256 values of `BlobStore`,
  session expiration and each chunked `blobService` case (multiple chunks,
  chunk sharing between files, empty files, manifest cleanup on delete)
- Outside the original scope: bug fix in `BlobStore` (see below), ADR 0007,
  a README for each module, a bibliography, and a set of quality issues
  (security hardening review, orphan blob garbage collection, OpenAPI spec,
  benchmarks, deployment guide, and others) created for after Sprint 7 and
  not assigned to any sprint

## What changed vs the plan

- **Block hashing did not need a module.** `BlobStore::store` already hashes
  its input and skips blobs that exist, so passing each chunk to it covers
  the hashing and the deduplication check. The issue was closed by changing
  the parameter to `std::string_view`, instead of adding a module that would
  only wrap one library call.
- **The open questions in the planning were answered.** Chunking is
  fixed-size, with 4 MiB blocks, and is recorded in ADR 0007. Content-defined
  chunking can replace `chunkContent` later without touching the storage
  layer. The endpoints moved to chunked storage in this sprint, with each
  chunk stored as a row in the existing `blobs` table. The session lifetime
  is 7 days.
- **The thread safety audit found real problems.** Individual SQLite calls
  are safe as compiled, but sequences of calls were not: `lastInsertId` is
  per connection, `storeFile` checked and then inserted, and transactions
  on a shared connection could interleave, the risk noted in the planning.
  Fixed with the recursive mutex.
- **An unplanned bug was fixed.** Two concurrent uploads of the same new
  content wrote to the same temporary file in `BlobStore`. Each call now uses
  a unique temporary file name, and a test covers it.
- **No leftover columns.** When the model changed, the columns that stopped
  making sense (`files.content_hash`, `blobs.content_type`) were dropped in
  the same sprint, even though that meant rebuilding the `files` table.
- **Features are not issues.** No new features are added until the existing
  ones are tested, retested, hardened and documented. Quality issues can
  still be added to the backlog, as was done this sprint.

## Blockers / learnings

- Designing the manifest before coding showed that `content_type` belonged
  to the file, not to the blob. It was a leftover from the whole-file model
  of Sprint 3. When a model changes, every column built on the old one
  needs to be checked.
- Concurrency problems were found by reading and auditing the code, not by
  tests. Only the `BlobStore` concurrent test was added, since stress tests
  for the rest were not judged worth their cost.
- Foreign keys are declared but not enforced, because `PRAGMA foreign_keys`
  is never turned on. Code that deletes rows has to remove the dependent rows
  explicitly, as `deleteFile` now does with the manifest.
- Session tokens are generated with `std::mt19937_64`, which is not a
  cryptographically secure generator. It is documented in the `auth` README
  and left for the security hardening review.

## Carried over

- None.