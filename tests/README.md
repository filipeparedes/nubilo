# Tests

All tests are built into a single GoogleTest executable, `nubilo_tests`, and are
discovered by CTest. Tests are grouped by feature, and each file is either a unit
test (`ut_`) or an integration test (`it_`).

## Running

```bash
cmake --build build --target nubilo_tests
ctest --test-dir build                                  # everything

./build/tests/nubilo_tests                              # everything, with gtest output
./build/tests/nubilo_tests --gtest_filter='UtFilesStore.*'          # one file
./build/tests/nubilo_tests --gtest_filter='Ut*'                     # unit tests only
./build/tests/nubilo_tests --gtest_filter='It*'                     # integration tests only
ctest --test-dir build -R UtFolders                     # one feature through CTest
```

## Layout

```
tests/
├── CMakeLists.txt        creates nubilo_tests and adds every folder below
├── common/               helpers shared by several folders (headers only)
│   ├── testUtils.h       temp paths, TEST_CHUNK_SIZE, makeMigratedDb()
│   ├── BlobFixture.h     migrated temp database + blob store, for blobService tests
│   └── RoutesEnv.h       server with all routes on a temp database, for integration tests
├── api/  auth/  files/  folders/  versioning/  path/  threads/  storage/
│                         unit tests, one folder per feature
├── blob/
│   ├── ut_chunker.cpp    a module with a single file goes straight into blob/
│   └── blobStore/        a module with several files gets its own folder (and fixture)
└── integration/          tests that start the server and use HTTP
    └── api/  auth/  files/  folders/  versioning/
```

Every folder has its own `CMakeLists.txt` that lists its files with
`target_sources(nubilo_tests PRIVATE ...)`.

## Unit or integration?

- **Unit (`ut_`)**: calls a function or class directly (`storeFile`, `Db`,
  `ThreadPool`, ...). No server, no HTTP.
- **Integration (`it_`)**: starts the server (`RoutesEnv`) and talks to it through an
  HTTP client.

If the test starts a server or sends an HTTP request, it is `it_`. Otherwise it is `ut_`.
The integration tests are meant to move to Python (pytest) later, which is why they
live apart in `integration/`.

## Naming

| What | Convention | Example |
|---|---|---|
| File | `ut_<feature>_<part>.cpp` / `it_<feature>_<part>.cpp` | `ut_files_store.cpp` |
| Suite | `Ut<Feature><Part>` / `It<Feature><Part>` | `UtFilesStore`, `ItAuthLogin` |
| Test | what the code does, in PascalCase | `RejectsEmptyPath` |

One file covers one part of a feature (store, read, list, delete, ...), so a suite
stays small and `--gtest_filter='UtFilesStore.*'` runs exactly that file.

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

Each test starts its own server on its own port, so tests never share state.

```cpp
#include "../../common/RoutesEnv.h"

TEST(ItFilesList, ListsUploadedFile) {
    RoutesEnv env(8110);
    auto client = env.newClient();
    // register, log in (registerAndLogin), then call the endpoint with the token
}
```

Pick a port that no other test uses. To see the ones in use:

```bash
grep -rhoE "RoutesEnv env\([0-9]+" tests/integration | sort
```

## Adding a test

1. **New test in an existing file:** add a `TEST_F` / `TEST` to it.
2. **New file in an existing feature:** create `ut_<feature>_<part>.cpp` in the
   feature folder and add it to that folder's `CMakeLists.txt`.
3. **New feature:** create the folder with a `CMakeLists.txt`, then add
   `add_subdirectory(<folder>)` to `tests/CMakeLists.txt` (or to
   `integration/CMakeLists.txt` for integration tests).

## Rules

- Tests are deterministic: no dependency on timing, order or the network.
- Every test uses its own temporary database and directory (the helpers in
  `testUtils.h` already do this).
- Test the behaviour, not the implementation, and keep one scenario per test.