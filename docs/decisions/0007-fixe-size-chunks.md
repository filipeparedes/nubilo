# 0007 - Fixed-size chunks stored as blobs, reassembled through a manifest

## Context

Until Sprint 4, an uploaded file was stored as a single blob, keyed by the
SHA-256 of its whole content. Two files that differ by one byte were stored
twice in full, and a large file had to be written and read in one piece.

Sprint 4 splits files into chunks so identical chunks are stored only once
and large files can be handled in pieces. Three questions had to be settled:

- How to split a file into chunks.
- Where the hash of each chunk is computed, since issue #24 (block-level
  SHA-256 hashing) was originally planned as a dedicated module.
- How chunks are recorded in the database so a file can be reassembled.

## Decision

- **Splitting:** fixed-size chunks (4 MiB by default, passed as a parameter)
  produced by `chunkContent`, which returns `std::string_view` slices of the
  original content. Nothing is copied at this stage.
- **Hashing:** there is no dedicated hash module. `BlobStore::store` already
  hashes its input and skips the write when the blob exists, so each chunk is
  passed straight to `store`, which now takes a `std::string_view`. Issue #24
  is closed by this change.
- **Storage:** every chunk is a regular blob (a row in `blobs` plus a file on
  disk). A new table `file_chunks (file_id, position, chunk_hash)`, with
  primary key `(file_id, position)` and `chunk_hash` referencing `blobs`, is
  the manifest of a file. Reading a file means reading its chunks in order.
- **Upload flow:** `chunkContent`, then `BlobStore::store` for each chunk,
  then the ordered hashes are written to the manifest. Registering the file
  and its manifest rows must be atomic.
- **Concurrency:** storing chunks in parallel through a generic thread pool is
  a separate issue, planned for the same sprint, and is not part of this
  decision.

## Reasoning

- Reusing `store` keeps one place responsible for hashing and deduplication.
  A separate hash module would only wrap one call to `picosha2`.
- Chunks as ordinary blobs means deduplication across files, users and
  versions comes from the existing content-addressable storage, with no new
  storage mechanism.
- The manifest also gives a direct way to count references to a chunk (the
  number of `file_chunks` rows pointing to it), which orphan blob garbage
  collection will need.
- Fixed-size splitting is simple, predictable and cheap: the chunker cost is
  negligible next to hashing and disk writes.

## Trade-offs accepted

- Inserting or removing bytes near the start of a file shifts every later
  boundary, so the chunks after the edit no longer match the previous
  version. Content-defined chunking would avoid this, at the cost of a more
  complex algorithm. It can replace `chunkContent` later without changing the
  storage layer.
- Many small rows in `blobs` and `file_chunks` for large files, and one disk
  write per new chunk. The database writes for a file should be batched in a
  single transaction.
- `files.content_hash` (currently the hash of the whole file) and the way
  `readFile` and `listFiles` use the manifest are decided in issue #25, not
  here.
