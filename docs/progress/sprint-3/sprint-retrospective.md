# Sprint 3 - Retrospective

**Dates:** 27 Sep - 4 Oct

**Goal (as replanned):** Build the file endpoints (upload, download, list,
delete) and whole-file content storage on disk, on top of the
authentication and persistence layers built in Sprints 1 and 2.

**Time:** ~70h against a ~68h budget. The overrun comes from the mid-sprint
replanning described below.

## Done

- Content-addressed blob storage on disk (`BlobStore`): files stored under
  their own SHA-256 hash (via picosha2), sharded into subdirectories,
  written atomically (temp file + rename). Storing identical content twice
  is a no-op
- `blobService` layer (free functions: `storeFile`, `readFile`, `listFiles`,
  `deleteFile`) sitting between the API and `BlobStore` + `Db`, so routes
  only validate and `BlobStore` knows nothing about the database
- Upload endpoint (`POST /files`, multipart with a `file` and an optional
  `path`)
- Download endpoint (`GET /files/{id}`), with `Content-Type` and
  `Content-Disposition` headers
- List files endpoint (`GET /files`), returning metadata only, without
  reading any content from disk
- Delete file endpoint (`DELETE /files/{id}`)
- First real use of the per-endpoint authorization principle documented in
  Sprint 2: download and delete return 404 both for missing files and for
  files owned by someone else, so ids can't be probed
- Schema: new `blobs` table, and `files` extended with `content_hash`,
  `created_at` and `updated_at` (migration 7)
- Unit tests for each `blobService` function and integration tests for each
  endpoint
- Outside the original scope: session token generation hardened with
  timestamp mixing (#75), `SqliteDb` renamed to `Db` (#76), `.clang-tidy`
  added, ADR 0006, and updated architecture and roadmap docs

## What changed vs the plan

- **The sprint was replanned mid-way.** The original Sprint 3 was the sync
  client (file-state cache, change detection, API client, diffing). While
  working on it we realised the roadmap had completely left out the file
  endpoints and server-side storage the client would talk to, so there was
  nothing for it to synchronise against. The scope was replaced by the file
  endpoints and storage, and the sync client moved to Sprint 6.
- **Frontend and client were merged by mistake.** The plan treated them as
  one `nubilo-client`, ignoring that the frontend is hosted on a server
  while the client had to be installed on the user's own machine to be able
  to sync. This led to ADR 0006 (three repositories: `nubilo`,
  `nubilo-client`, `nubilo-web`), superseding ADR 0001. The roadmap changed
  accordingly: `nubilo-client` in Sprint 6, `nubilo-web` in Sprint 7, and
  notifications and file sharing moved to extras.
- **`file_metadata` was replaced by `blobs`.** The Sprint 2 table was 1:1
  with `files`, so two users uploading the same content could not share
  it, and its timestamps would have been shared too. It was split into
  `blobs` (size and content type, one row per unique content) and `files`
  (owner, path, hash and timestamps, one row per owner and path). This was
  caught by discussing the design before building on top of it.
- **A `blobService` layer was introduced.** `BlobStore` only touches the
  disk, and putting database logic in the API layer would have broken the
  pattern used by `auth`. Ownership validation stays in the routes, not in
  the service.
- **Delete only removes the `files` row.** The blob stays on disk and in
  `blobs`, since other users may reference the same content. Cleaning up
  orphaned blobs is deliberately left for a future garbage collection.

## Blockers / learnings

- The replanning came from validating the roadmap at phase level only:
  nobody checked what the sync client would actually talk to. Each sprint's
  scope should be checked against what it depends on.
- Settling the data model (metadata vs content) before writing the
  endpoints avoided reworking them later.

## Carried over

- None.