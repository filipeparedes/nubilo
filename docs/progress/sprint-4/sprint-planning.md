# Sprint 4 - Planning

**Dates:** 4 - 11 Oct

**Goal:** Break stored files into blocks and store them by hash, with
deduplication on write (the technical core of the project), and close two
loose ends from the earlier sprints: session expiration and thread safety
of the shared database connection.

## Scope

- File chunking algorithm (splitting files into blocks)
- Block-level SHA-256 hashing
- Chunk manifest structure (per-file list of chunk hashes)
- Chunk storage layer (content-addressed storage on disk)
- Deduplication check on chunk write (skip if hash already stored)
- Add session expiration: `expires_at` column on `sessions`, set at login
  and enforced in `requireAuth`. An expired session is treated as invalid
  and removed from the table on next use (lazy cleanup)
- Audit `Db` thread safety under concurrent requests (issue title still
  says `SqliteDb`, renamed in Sprint 3): cpp-httplib serves requests from a
  thread pool and all route handlers share the same instance by reference.
  Verify whether SQLite's threading mode (as compiled via the amalgamation)
  already serializes access safely, and if not, protect `exec()`/`query()`
  with a `std::mutex` and `std::lock_guard`

~70h budget, in line with the time Sprint 3 actually took (roadmap baseline
is ~68h). The last two items were not in the roadmap and were added after
Sprint 3 planning.

## Success criteria

- A file is split into blocks, each block is hashed, and its manifest lists
  the block hashes in order
- Blocks are stored on disk by hash, and writing a block whose hash is
  already stored does not write it again
- A request with an expired session token is rejected, and the expired
  session is removed from `sessions` on that request
- The thread safety question has a documented answer (safe as compiled, or
  made safe with a mutex), not an assumption
- Covered by tests consistent with the project's Definition of Done

## Risks / open questions

- This is the first sprint on the core algorithm of the project, so it
  carries more uncertainty than the endpoint work of Sprint 3. Chunking
  strategy (fixed-size vs content-defined blocks) and the block size are
  still to be decided
- The upload and download endpoints currently store whole files. How they
  move to the chunked storage, and what happens to the existing `blobs`
  table, is still open
- Session lifetime (how long a session lasts) is not decided yet
- Even if SQLite serializes individual calls, transactions
  (`beginTx`/`commitTx`) on a single shared connection could interleave
  between threads, which a per-call mutex would not prevent. To be
  verified during the audit