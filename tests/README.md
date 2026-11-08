# Tests

There are two kinds of tests, in two languages:

- **Unit tests** (`ut_`) are C++ files, built into a single GoogleTest executable,
  `nubilo_tests`, and discovered by CTest. They are grouped by feature in the folders of
  this directory.
- **Integration tests** (`it_`) are Python files, run with pytest, in
  [`integration/`](integration). They start the real server and talk to it over HTTP, as
  any client would.

## Running

```bash
scripts/test.sh                                   # unit and integration tests
scripts/test.sh unit                              # unit tests only
scripts/test.sh integration                       # integration tests only
```

The scripts build what is needed first. The integration tests need Python 3 and the
packages of `integration/requirements.txt`:

```bash
pip install -r tests/integration/requirements.txt
```

The same thing without the scripts:

```bash
cmake --build build --target nubilo_tests
ctest --test-dir build                                  # all the unit tests

./build/tests/nubilo_tests                              # all the unit tests, with gtest output
./build/tests/nubilo_tests --gtest_filter='UtFilesStore.*'          # one unit test file
ctest --test-dir build -R UtFolders                     # one feature through CTest

cmake --build build --target nubilo
python3 -m pytest tests/integration                     # all the integration tests
python3 -m pytest tests/integration/auth                # one feature
python3 -m pytest tests/integration -k cors             # the tests whose name has "cors"
```

## Layout

```
tests/
├── CMakeLists.txt        creates nubilo_tests and adds every folder below
├── common/               helpers shared by several folders (headers only)
│   ├── testUtils.h       temp paths, TEST_CHUNK_SIZE, makeMigratedDb()
│   └── BlobFixture.h     migrated temp database + blob store, for blobService tests
├── api/  auth/  files/  folders/  versioning/  path/  threads/  storage/
│                         unit tests, one folder per feature
├── blob/
│   ├── ut_chunker.cpp    a module with a single file goes straight into blob/
│   └── blobStore/        a module with several files gets its own folder (and fixture)
└── integration/          Python tests that start the server and use HTTP
    ├── pytest.ini        pytest settings: the test files are named it_*.py
    ├── requirements.txt  the Python packages the tests need
    ├── conftest.py       the `server` and `client` fixtures
    ├── helpers.py        the HTTP client and the calls shared by the tests
    └── api/  auth/  files/  folders/  versioning/
```

Every unit test folder has its own `CMakeLists.txt` that lists its files with
`target_sources(nubilo_tests PRIVATE ...)`. The integration tests are not built by CMake.

## Unit or integration?

- **Unit (`ut_`)**: calls a function or class directly (`storeFile`, `Db`,
  `ThreadPool`, ...). No server, no HTTP. Written in C++.
- **Integration (`it_`)**: starts the server and sends HTTP requests to it. Written in
  Python, in `integration/`.

If the test starts a server or sends an HTTP request, it is `it_`. Otherwise it is `ut_`.

## Naming

| What | Convention | Example |
|---|---|---|
| Unit file | `ut_<feature>_<part>.cpp` | `ut_files_store.cpp` |
| Unit suite | `Ut<Feature><Part>` | `UtFilesStore` |
| Unit test | what the code does, in PascalCase | `RejectsEmptyPath` |
| Integration file | `it_<feature>_<part>.py` | `it_auth_login.py` |
| Integration test | `test_` and what the server does, in snake_case | `test_rejects_wrong_password` |

One file covers one part of a feature (store, read, list, delete, ...), so a suite stays
small and a single file can be run on its own.

## Writing a unit test

Tests that need a database and a blob store use `BlobFixture`. It provides `db_`
(migrated, temporary) and `blobStore_`, and cleans everything up afterwards, so
tests need no manual cleanup.

```cpp
#include "../common/BlobFixture.h"

class UtFilesStore : public BlobFixture {};

TEST_F(UtFilesStore, StoresNewFile) {
    nubilo::File file{"notes.txt", "hello", "text/plain"};

    auto id = nubilo::storeFile(*db_, *blobStore_, 1, file);

    ASSERT_TRUE(id.has_value());
}
```

Other helpers: `storeText(ownerId, path, content)` and `paths(contents)` in
`BlobFixture`; `nubilo::test::TEST_CHUNK_SIZE` for files with several chunks (it must
match `DEFAULT_CHUNK_SIZE` in `blobService.cpp`).

Tests that need no fixture use a plain `TEST(UtThreadsPool, ...)`. A module with
its own setup keeps its fixture in its own folder (`blob/blobStore/BSFixture.h`,
`storage/DbFixture.h`).

## Writing an integration test

Each test gets its own server, started from the binary `build/nubilo` (another one can be
given with the `NUBILO_BIN` environment variable). The server has its own port, database
and blob directory, in a temporary folder, so tests never share state. Build the server
before running them (`scripts/build.sh`).

A test asks for the fixtures it needs by naming them as arguments:

- `client`: an HTTP client for that server, with `get`, `post`, `delete` and `options`.
  Pass the session token with `token=`; it is sent as the `Authorization` header.
- `server`: the running server. `server.query(sql)` and `server.execute(sql)` read and
  change its SQLite database directly, to check what the API did or to set up a state the
  API cannot, such as an expired session.

```python
from helpers import register_and_login, upload


def test_returns_uploaded_files(client):
    token = register_and_login(client)
    upload(client, token)

    response = client.get("/files", token=token)

    assert response.status_code == 200
    assert len(response.json()) == 1
```

`register_and_login(client, email)` registers a user and logs in, giving the token.
`upload(client, token, content, filename, content_type, path)` uploads a file. Both are in
`helpers.py`, with the `Client` class.

## Adding a test

1. **New test in an existing file:** add a function to it.
2. **New file in an existing feature:** create `it_<feature>_<part>.py` (or
   `ut_<feature>_<part>.cpp`, and add it to the `CMakeLists.txt` of the folder).
3. **New feature:** create the folder. For unit tests, add a `CMakeLists.txt` and an
   `add_subdirectory(<folder>)` to `tests/CMakeLists.txt`. For integration tests, a
   folder in `integration/` is enough.

## Rules

- Tests are deterministic: no dependency on timing, order or the network.
- Every test uses its own temporary database and directory (the helpers and fixtures
  already do this).
- Test the behaviour, not the implementation, and keep one scenario per test.
