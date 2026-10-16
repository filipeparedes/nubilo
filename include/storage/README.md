# storage

SQLite access for the whole server: connection, transactions and schema migrations. Other modules use it through the `Db` class and never talk to `sqlite3` directly.

## Structure

| File | Role |
|---|---|
| `Db.h` / `Db.cpp` | RAII wrapper around a SQLite connection. Runs SQL (`exec`, `query`) and exposes `lastInsertId`. |
| `DbError.h` | Error type returned by every storage call (`DbError{msg}`). |
| `DbTransaction.h` / `DbTransaction.cpp` | RAII transaction. Rolls back on its own if `commit()` is not called. |
| `DbMigrator.h` / `DbMigrator.cpp` | Applies the schema migrations that have not run yet. |
| `DbMigrations.h` | The list of migrations, in version order. |

## How it works

**Queries.** `exec` runs statements with no result rows and `query` returns the rows as a JSON array, one object per row. Both take positional `?` parameters as strings, so values are never concatenated into the SQL. Failures come back as `std::expected` with a `DbError`.

**Thread safety.** The HTTP server handles requests on several threads that share one `Db`. Each call to `exec`, `query` and `lastInsertId` locks a recursive mutex for its duration. When several calls must run as one unit, for example an `INSERT` followed by `lastInsertId`, hold `db.lock()` around them. The mutex is recursive, so the calls inside can still lock it.

**Transactions.** `DbTransaction::begin(db)` starts a transaction. Call `commit()` to keep the changes. Leaving the scope without committing, including on an early `return` after an error, rolls everything back.

**Migrations.** At startup `DbMigrator` creates the `schema_migrations` table if needed, compares it with the list in `DbMigrations.h` and applies the missing versions in order. Each migration runs in its own transaction together with the insert that records its version, so a failed migration leaves no trace.

## Schema

| Table | Purpose |
|---|---|
| `users` | Accounts: email and password hash. |
| `sessions` | Login tokens, with an expiration date. |
| `files` | One row per file: owner, path and timestamps. Unique per owner and path. |
| `file_versions` | One row per version of a file: version number, size, content type and creation date. |
| `blobs` | One row per stored chunk: hash and size. |
| `file_chunks` | The ordered list of chunks that make up each version. |
| `sync_state` | Per-file sync version. |
| `schema_migrations` | Which migrations have been applied. |

## Notes

* **Adding a migration:** append a new entry with the next version number to `DbMigrations.h`. Never edit or remove an entry that has already run in any environment.
* **Foreign keys** are declared in the schema but SQLite does not enforce them unless `PRAGMA foreign_keys` is on, and the project does not turn it on. 
Code that deletes rows must also delete the rows that depend on them. Deleting a file removes its `file_chunks`, then its `file_versions`, then the `files` row, in that order.
* `Db::handle()` returns the raw `sqlite3*` and is not protected by the mutex.
* **Migration 14** makes `files` unique per owner and path, so it fails on a database that already has two files with the same owner and path. 
In a development database, delete it and start again.