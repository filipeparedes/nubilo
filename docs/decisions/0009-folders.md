# 0009 - Folders as rows, with the full path kept on files

## Context

Until now a path was only a string on a file. A path such as `docs/2026/notes.txt` had slashes in it, but nothing in the system knew that `docs` was a folder. Issue #92 asked for folder support and suggested starting with the path-prefix model (no new table) and only adding a table if empty folders or atomic moves became necessary.

Three questions had to be settled:

- Whether a folder is something that exists on its own or only the shared prefix of some file paths.
- What creating, listing and deleting folders mean, and what happens when a file and a folder want the same path.
- Where the integrity of the folder tree is guaranteed.

## Decision

- **Folders table:** a new table `folders (id, owner_id, path, created_at)` with `UNIQUE (owner_id, path)`. There is one row per folder. The root has no row.
- **Paths:** `files.path` stays the full path, for example `docs/2026/notes.txt`. A folder row stores the full path of the folder too (`docs/2026`). Paths are stored without a trailing slash.
- **Invariant:** every parent folder of every file and every folder has a row. The code keeps this true: `storeFile` and `createFolder` call `ensureParentFolders`, which creates the missing parents (like `mkdir -p`). Migration 16 backfills the folders of the files that already existed, with a recursive query over the positions of the slashes.
- **Creating:** `POST /folders` creates the folder and its missing parents, and answers 201 with the id. Creating a folder that already exists returns its id, so it is idempotent.
- **Listing:** `GET /folders` lists the root and `GET /folders/{id}` lists a folder. Both return the direct subfolders and the direct files, never the deeper levels. Children are found by comparing the path prefix with `substr`, not with `LIKE`, because `LIKE` treats `%` and `_` as wildcards and is case-insensitive.
- **Same path:** a file and a folder of the same owner cannot have the same path, and a file cannot be the parent of anything. Both cases are answered with 409 `PATH_CONFLICT`, from `POST /files` and from `POST /folders`. The error is a `BlobError` of type `Conflict`, so the routes can tell it from an internal failure.
- **Deleting a file:** it does not touch folders. A folder stays even when its last file is deleted.
- **Deleting a folder:** `DELETE /folders/{id}` removes the folder and everything inside it, at any depth: subfolders, files, their versions and their chunk lists, in one transaction. It is permanent. Its parent folders stay.
- **Validation:** `validatePath` rejects empty paths, paths longer than 1024 bytes, leading or trailing slashes, backslashes, control characters, empty segments and `.` or `..` segments. Spaces and non-ASCII characters are allowed.
- **Out of scope:** moving and renaming folders.

## Reasoning

- Empty folders and folder metadata (the creation date, and later anything else) need somewhere to live. With only prefixes a folder that holds no file does not exist, and the user cannot create one before uploading into it.
- Keeping the full path on both tables means a file keeps its identity, `UNIQUE (owner_id, path)` still holds, and no existing query or version history had to change. Versioning (ADR 0008) is untouched.
- Listing by prefix on a path column needs no joins and no recursion, and a folder tree stored as `parent_id` would have required a recursive query for every path lookup.
- Making the code responsible for the invariant keeps the schema simple. The database has no foreign keys enforced anyway (see the storage README), so integrity already lives in the code.
- Rejecting a file and a folder at the same path avoids a state that cannot be shown in any file system: one name that is both.
- Deleting a folder with its content matches what a user expects from a delete in a file manager, and avoids a client having to walk the tree and delete it one file at a time.

## Trade-offs accepted

- The tree is stored as paths, so renaming or moving a folder would have to rewrite the path of every file and folder under it. This is why it is out of scope for now, and why it was the reason to consider a different model if moves become important.
- The invariant (every parent has a row) is not enforced by the database. A bug in the code, or a row written by hand, can break it.
- Deleting a folder is permanent and there is no way to undo it. The chunks stay on disk until the orphan blob garbage collection exists, but the metadata is gone. A trash with a retention period was considered and left for a future release, because it touches every query (a `deleted_at` filter), conflicts with the unique path, and needs restore and purge endpoints.
- Deleting a folder is a single transaction over every file below it, so a very large folder holds the database lock for as long as it takes.
- The path of a folder is checked against files with one extra query per level on every upload and folder creation.
- Migration 16 does not check for existing clashes, so a database that already has a file and a folder-to-be with the same path keeps both. This only affects development databases.