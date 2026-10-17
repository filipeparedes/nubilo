# 0008 - File versions as rows of a file, with the manifest per version

## Context

Until Sprint 5, every upload created a new file, even when it had the same path as an earlier one, and a file's chunk list belonged directly to the file. There was no history, so a file could not be inspected or restored to an earlier state.

Sprint 5 adds versioning (issues #30, #31 and #32). Three questions had to be settled:

- What an upload to an existing path means.
- Where the size, content type and chunk list of a file live.
- How a rollback works if old versions must still be available.

## Decision

- **Identity:** a file is identified by its owner and path (`UNIQUE (owner_id, path)`). Uploading to an existing path adds a new version to the same file. The file keeps its id and the upload still answers 201.
- **Versions:** a new table `file_versions (id, file_id, version, size, content_type, created_at)` with `UNIQUE (file_id, version)`. Each version has its own size and content type.
- **Manifest:** `file_chunks` now belongs to a version: `(version_id, position, chunk_hash)`, with primary key `(version_id, position)`.
- **Files table:** `files` keeps only the identity: id, owner, path and timestamps. `content_type` moved to the versions.
- **Current version:** the highest version number of a file. There is no pointer to it.
- **Rollback:** restoring version K creates version N+1 with a copy of the chunk list of version K. History is append-only and no version is ever changed or removed by a rollback.
- **Deleting:** deleting a file removes the file, all its versions and their manifests, in one transaction. The chunks are left to the orphan blob garbage collection.
- **Migrations:** four migrations, one per step (11 to 14): create `file_versions`, turn every existing file into version 1, move the manifest to the versions, and rebuild `files` without `content_type` and with the unique constraint. Tables are rebuilt (create, copy, drop, rename), because SQLite cannot drop a column that has a `REFERENCES` clause.

## Reasoning

- Chunks are already content-addressed blobs shared by hash, so a new version only adds the chunks that changed and reuses the rest. Versioning needs no new storage mechanism.
- A manifest per version keeps every version complete and independent: reading any version is the same operation as reading the latest one.
- Storing size and content type per version keeps them correct when a new version changes them, and lets the listing read them without summing chunks.
- Taking the highest number as the current version avoids a second source of truth that could disagree with the history.
- A rollback that appends a version keeps the history honest: the restore itself is recorded, and a restore can be undone by another restore.
- Unused columns are dropped properly instead of left in the schema.

## Trade-offs accepted

- A rollback copies the chunk list of the old version. That is one row per chunk, which is small next to the chunk data, and it gives the new version a manifest of its own.
- Old versions keep their chunks alive, so storage grows with every upload until there is a way to remove versions and garbage collect the chunks that nothing references.
- Migration 14 fails on a database that already has two files with the same owner and path. This only affects development databases created before the change.
- Two uploads to the same path used to create two files and now create two versions of one. A client that relied on getting a new id for every upload must use the path instead.
- There is no way yet to delete a single version, only the whole file.