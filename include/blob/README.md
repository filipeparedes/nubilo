# blob

Stores file content. A file is split into fixed-size chunks, each chunk is saved on disk under its SHA-256 hash, and the database keeps 
the ordered list of chunks that make up each version of a file. Identical chunks are stored only once.

## Structure

| File | Role |
|---|---|
| `blobService.h` / `blobService.cpp` | The functions the routes call: `storeFile`, `readFile`, `listFiles`, `deleteFile`. They combine the disk store with the database. |
| `BlobStore.h` / `BlobStore.cpp` | Content-addressed storage on disk. Hashes content, writes it, reads it back by hash. |
| `chunker.h` / `chunker.cpp` | `chunkContent`: splits content into chunks of at most a given size. |
| `BlobError.h` | Error type returned by every call in this module (`BlobError{message}`). |
| `path.h` / `path.cpp` | `validatePath`: checks that a file or folder path is acceptable. `parentPaths`: lists the folders that contain a path. |

## How it works

**Layers.** Routes call `blobService`, which uses `BlobStore` for the bytes on disk and `Db` (see the `storage` module) for the records.

**On disk.** `BlobStore` keeps each blob in `root/<hash[0:2]>/<hash[2:4]>/<hash>`, so no single directory grows too large. The server uses the `blobs` directory next to where it runs.

**Storing a file.**
1. `chunkContent` splits the content into chunks of 4 MiB. The last chunk holds what remains and an empty file has no chunks.
2. `BlobStore::store` hashes each chunk and writes it. If a blob with that hash already exists, nothing is written. 
When a thread pool is given and the file has more than one chunk, the chunks are written concurrently.
3. In one database transaction, new chunks are added to `blobs`, the file is added to `files` if it does not exist yet, a new row is added to `file_versions` and the chunks of that 
version are added to `file_chunks` with their position.

**Concurrent chunk writes.** `storeFile` takes an optional `ThreadPool*` (see the `concurrency` module). With a pool, each chunk is written by a task and the results are collected in the original order, 
so the manifest keeps the right positions. `storeFile` always waits for every task to finish before returning, even if one fails, because the tasks read the file content. Without a pool, or for a file with a single chunk, 
the chunks are written one after another on the calling thread.

**Reading a file.** `readFile` finds a version of the file (the latest one unless a version number is given), reads its chunks from `file_chunks` in order, reads each one from the `BlobStore` and joins them.

**Versions.** A file is identified by its owner and path. Uploading to a path that already exists does not create a new file, it adds a new version to it: the file keeps its id and 
the version number goes up by one. Each version has its own size, content type and chunk list. The current version is the one with the highest number, and reads and listings always 
use it. Chunks that did not change between versions are the same blobs, so only the changed chunks take new space.

**Rollback.** `rollbackFile` restores a file to one of its earlier versions. It adds a new version with the size, content type and chunk list of the chosen one, 
so the history is kept and a rollback can itself be rolled back. No chunk is written, because the new version points to the blobs that already exist.

**Writes.** A blob is written to a temporary file with a unique name and then renamed to its final path. The rename is atomic, so a blob is never visible half-written, and two 
uploads of the same new content at the same time do not collide.

**Deduplication.** Chunks are shared by hash across files and users. Two files with a chunk in common reference the same blob.

**Failed uploads.** The database is only changed after every chunk has been written. If an upload fails, no file is created. The chunks already written stay on disk, since other files may share them.

## Notes

* **Deleting a file** removes the file, all its versions and their chunk lists, but not the chunks. Unreferenced chunks 
stay on disk until garbage collection is implemented.* **Chunk size** is the constant `DEFAULT_CHUNK_SIZE` in `blobService.cpp`. 
It is not configurable yet.
* **Memory.** A file is held in memory in full while it is uploaded and when it is read back.
* **Hashing** uses the PicoSHA2 library.
