# Nubilo

Trying to build a personal file synchronization system API from scratch in C++.

> A Dropbox-like sync engine, built as a final year project -> no subscriptions, no bloat, just the essentials for keeping files in sync across devices.

---

## Planned Features

- **REST API** — HTTP interface for clients to interact with the sync system
- **Authentication** — session/token-based auth to identify users
- **Change detection** — SHA-256 hashing to detect modified files
- **File chunking & deduplication** — files split into blocks, avoiding redundant storage/transfer of identical content
- **File versioning** — history of previous file versions, with rollback support
- **Conflict handling** — resolving the same file being changed in two places
- **Real-time notifications** *(stretch goal)*
- **File sharing between users** *(stretch goal)*

> None of the above is implemented yet — this project is currently in the
> planning phase. See [Roadmap](#roadmap) below.

---

## Requirements

- C++23 or later, with C++20 modules support
- CMake 3.28+
- SQLite
- [cpp-httplib](https://github.com/yhirose/cpp-httplib)
- [nlohmann/json](https://github.com/nlohmann/json)
- GoogleTest (for tests only)

```bash
brew install cmake sqlite googletest        # macOS
sudo apt install cmake libsqlite3-dev libgtest-dev  # Linux (Debian/Ubuntu)
vcpkg install sqlite3 gtest                 # Windows (via vcpkg)
```

