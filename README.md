# Nubilo

A personal file synchronization system, built from scratch in C++. This repository holds the server: a REST API with authentication, content-addressed file storage, chunk-level deduplication and file versioning.

> A Dropbox-like sync engine, built as a final year project. No subscriptions, no bloat, just the essentials for keeping files in sync across devices.

---

## Status

| Feature | Status |
|---|---|
| REST API with routing and a consistent error format | Done |
| Authentication with sessions and tokens (Argon2 password hashing) | Done |
| File upload, download, list and delete, with per-user access control | Done |
| Content-addressed storage with SHA-256 | Done |
| File chunking and deduplication across files and users | Done |
| File versioning with rollback | Done |
| Concurrent chunk storage with a thread pool | Done |
| Folder support | Planned |
| Version listing and comparison | Planned |
| Conflict handling | Planned |
| `nubilo-client`: local sync client and CLI | Planned |
| `nubilo-web`: web frontend | Planned |
| Deployment, OpenAPI specification, orphan blob cleanup, benchmarks | Planned |
| Real-time notifications and file sharing between users | Stretch goals |

See the [Roadmap](docs/roadmap.md) for the plan, sprint by sprint.

---

## How it works

Files are split into 4 MiB chunks. Each chunk is stored on disk under its SHA-256 hash, so identical content is stored only once, even across files and users. The database keeps the ordered list of chunks that make up each file. Uploading to a path that already exists adds a new version of the file, and only the chunks that changed take new space. A file can be restored to any earlier version, and the history is kept.

The server is a modular monolith: a single process organised into the modules `api`, `auth`, `blob`, `storage` and `concurrency`. See [Architecture](docs/architecture.md).

---

## Repositories

| Repository | Role |
|---|---|
| `nubilo` (this one) | The server: the synchronization API. |
| `nubilo-client` | Local executable with the sync client and a CLI. |
| `nubilo-web` | Optional web frontend that talks to the server over HTTP. |

---

## Requirements

- A C++23 compiler (developed with Clang/LLVM)
- CMake 3.28+
- OpenSSL, zlib and Brotli (used by cpp-httplib)

cpp-httplib, nlohmann/json, SQLite, Argon2, PicoSHA2 and GoogleTest are downloaded by CMake during configuration.

```bash
brew install cmake openssl brotli                          # macOS
sudo apt install cmake libssl-dev zlib1g-dev libbrotli-dev  # Linux (Debian/Ubuntu)
```

---

## Build, run and test

```bash
cmake -S . -B build
cmake --build build

./build/nubilo                                    # listens on port 8080

cmake --build build --target nubilo_tests
ctest --test-dir build
```

See [`tests/README.md`](tests/README.md) for how the tests are organised and how to run a single one.

The server creates `nubilo.db` and a `blobs/` directory in the folder where it is started.

---

## Quick tour

```bash
# register and log in (the response contains the token)
curl -X POST localhost:8080/auth/register -H "Content-Type: application/json" \
  -d '{"email": "user@example.com", "password": "a-strong-password"}'
curl -X POST localhost:8080/auth/login -H "Content-Type: application/json" \
  -d '{"email": "user@example.com", "password": "a-strong-password"}'

TOKEN="<token from the login response>"

# upload a file (uploading to the same path again adds a new version)
curl -X POST localhost:8080/files -H "Authorization: Bearer $TOKEN" \
  -F "file=@notes.txt" -F "path=notes.txt"

# list files, download one and restore version 1 of it
curl localhost:8080/files -H "Authorization: Bearer $TOKEN"
curl localhost:8080/files/1 -H "Authorization: Bearer $TOKEN" -o notes.txt
curl -X POST localhost:8080/files/1/versions/1/restore -H "Authorization: Bearer $TOKEN"
```

All endpoints are listed in [`include/api/README.md`](include/api/README.md).

---

## Documentation

| Where | What |
|---|---|
| [`docs/architecture.md`](docs/architecture.md) | Overview of the system and its modules. |
| [`docs/roadmap.md`](docs/roadmap.md) | Phases and sprint plan. |
| [`docs/decisions/`](docs/decisions) | Architecture decision records (ADRs). |
| [`docs/progress/`](docs/progress) | Sprint planning and retrospectives. |
| [`docs/coding-standards.md`](docs/coding-standards.md) | Code conventions. |
| [`docs/definition-of-done.md`](docs/definition-of-done.md) | What a finished task means. |
| [`docs/ai-usage.md`](docs/ai-usage.md) | Declaration of how AI was used in the project. |
| [`docs/bibliography.md`](docs/bibliography.md) | References. |
| `include/<module>/README.md` | What each module does and how it works: `api`, `auth`, `blob`, `storage`, `concurrency`. |