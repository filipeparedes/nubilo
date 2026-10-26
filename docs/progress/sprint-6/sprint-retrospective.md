# Sprint 6 - Retrospective

**Dates:** 18 - 25 Oct

**Goal:** Add folder support to the API, before anything is built on top of
it, set up continuous integration, complete the versioning of files, and
start `nubilo-client` with the pieces that let it compare local files with
the server.

**Time:** ~60h against a ~68h budget. Folder support took much longer
than expected, which left no room for `nubilo-client`. Halfway through the
sprint the plan was changed (see below) and the time went into finishing the
backend and the pipeline instead.

## Done

- Folder support (#92): new `folders` table (migrations 15 and 16, the second
  one backfills the parents of existing files) and the endpoints
  `POST /folders`, `GET /folders`, `GET /folders/{id}` and
  `DELETE /folders/{id}`. Folders are rows of their own and files and folders
  keep their full path, so an empty folder can exist and a listing is a
  prefix query. Creating a folder also creates its missing parents and is
  idempotent. Deleting a folder deletes everything inside it, permanently.
  Recorded in ADR 0009
- Conflicts between files and folders: a file and a folder cannot have the
  same path, and a file cannot be the parent of another path. The service
  reports it with a `Conflict` error type and the routes answer 409
  `PATH_CONFLICT`. Nothing is left half-created when it happens
- Version listing (#33): `GET /files/{id}/versions`, newest first
- Version comparison (#127): `GET /files/{id}/versions/compare?from=N&to=M`.
  It compares the chunk lists of two versions by position and returns the
  chunks that differ, with their hashes and the totals. It does not compare
  content, it only reports which chunks changed
- Download of a specific version (#130): `GET /files/{id}/versions/{version}`.
  `readFile` now takes an optional version, and without it still reads the
  latest. Added so the frontend can show an older version and, later, a diff
- CI pipeline (#101): a GitHub Actions workflow that builds the project and
  runs the tests on every pull request to `main` and on every push to `main`,
  cancelling the run in progress when a new commit arrives
- Tests reorganized by feature (#128): the tests were moved from one file per
  source file to feature folders, split into unit tests (`ut_`) and
  integration tests (`it_`), each folder with its own `CMakeLists.txt` and
  `tests/README.md` explaining how it works. Documented in the coding
  standards. The suite grew from 180 to 195 tests
- Documentation: ADR 0009, the module READMEs, the API endpoint table, the
  roadmap and the bibliography brought up to date with folders, version
  listing, comparison and download

## What changed vs the plan

- **`nubilo-client` was cancelled for this sprint.** Folder support took much
  longer than expected: it touched the path model, the migrations, the
  existing endpoints and most of the tests. By the middle of the sprint it
  was clear that there was no time for the four client items. The client is
  moved to after v1.0 and stays as a long-term goal, starting with a CLI
- **The plan was changed.** Instead of ending the sprint with a
  client that had no backend to sync against, the sprint was refocused:
  finish the backend (folders, version listing and comparison), set up the
  CI pipeline and go straight to `nubilo-web`, so that the project has
  something users can see and use. The sprint planning was left as it was
  written and the change is recorded here
- **Groundwork for conflict detection was dropped.** It only makes sense
  with a client that knows which version its change was based on, so without
  the client there was nothing to build it for. The success criterion about an
  ADR for conflict detection was not met, and conflict handling stays on the
  roadmap
- **Version listing and comparison were split.** They started as one issue
  (#33) and became two, listing and comparison (#127), so that each pull
  request stayed small
- **Deleting a folder deletes its content.** The first idea was to refuse to
  delete a folder that is not empty. It was changed to a recursive and
  permanent delete, and a trash that would allow undoing it was noted as a
  possible extension in ADR 0009
- **Unplanned work:** the download of a specific version (#130) and the
  reorganization of the tests (#128). The tests were in a few very large
  files (`blobServiceTest.cpp` had almost 1500 lines) and were getting hard to
  navigate, so the reorganization was done before more tests were added
- **Carried over to Sprint 7:** CORS support and the setup of `nubilo-web`,
  which did not start in this sprint

## Blockers / learnings

- The folder model was the main risk in the planning and it was. Keeping a
  row for every folder means every file path must have all its parents
  recorded, and that invariant has to be kept by `storeFile` and
  `createFolder`, by the migration for existing files, and by the delete.
  Foreign keys are still not enforced, so the delete removes the rows in a
  fixed order
- The sprint was too wide, as the planning already said. The review halfway
  through the sprint is what led to the change of plan, and it worked
- The first runs of the CI pipeline found three things that never showed up
  locally: the runner's default GCC does not have `<print>` (C++23), so the
  workflow installs GCC 14; an unlimited `--parallel` ran the runner out of
  memory; and a file name with the wrong case (`HealthRoutes.cpp`) only fails
  on Linux
- The integration tests had a race: the test helper started the server in a
  thread and sent requests before it was listening, which failed once in a
  while. It now waits for the server to be ready
- There is no way yet to move or rename a file or a folder, only to delete and
  upload again. It is a gap for the file browser in `nubilo-web`
- A version download, comparison and listing are enough for a history view in
  the frontend, but a real diff of the content would need the server or the
  frontend to fetch and compare two versions

## Carried over

- CORS support, so the browser frontend can call the API