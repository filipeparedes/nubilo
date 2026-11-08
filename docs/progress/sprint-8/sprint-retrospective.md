# Sprint 8 - Retrospective

**Dates:** 1 - 8 Nov

**Goal:** Reach v1.0: finish the file browser of `nubilo-web` so that a user
can log in, navigate their folders, upload, download and delete files and
work with the versions of a file from the browser, and document the API and
the deployment.

**Time:** ~65h against a ~40h budget. The five issues of `nubilo-web` carried
over from Sprint 7 took most of it. The sprint reached its goal for the
browser and for the documentation, and left out the deployment itself, the
orphan blob cleanup and the benchmarks.

## Done

`nubilo`:

- Root contents and folder list (#142): `GET /root-contents` returns the
  contents of the root and `GET /folders` returns the list of all the
  folders, so the same collection no longer means two things and the side
  panel can draw the whole tree in one call
- Paths in the responses (#143, #146): the contents of a folder and the list
  of versions of a file return the full path, so the frontend can show where
  the user is without keeping it in a separate place
- Helper scripts (#134): `scripts/build.sh`, `run.sh` and `test.sh`, which
  build, start and test the project with 4 parallel jobs by default
- Integration tests in Python (#114, #36): the integration tests were
  converted from C++ to Python (pytest and requests) and are now separate
  from the unit tests. Each test starts its own server with a temporary
  configuration. The CI runs the unit tests and then the integration tests
- OpenAPI specification (#93): `docs/openapi.yaml`, OpenAPI 3.1, with the 17
  endpoints, schemas, errors and examples
- Deployment guide (#95): `docs/deployment.md`, for a generic Linux server,
  with systemd, a reverse proxy, backup and restore and the known limits
- Documentation consolidation (#37): README and feature status, architecture,
  roadmap, Sprint 8 planning and the module READMEs checked against the code

`nubilo-web`:

- Path navigation, including the side panel with the folder tree
- File upload, to the current folder
- File download and delete
- File history list and restore of a previous version
- Comparison of two versions, with the selection of the two versions to
  compare
- The current folder shown in the path of the footer

## What changed vs the plan

- **The sprint was over budget before it started, and it was.** The work
  carried over from Sprint 7 was the bulk of the sprint, as the planning
  warned, and the hours went from ~40h to ~65h. The items at the end of the
  list were the ones cut
- **Deployment, orphan blob cleanup and benchmarks were moved out of v1.0.**
  With the time spent on the frontend, the decision was to finish the
  documentation of the API and of the deployment instead. The guide describes
  how to deploy, but the system was not deployed. Automated deployment with
  GitHub Actions depends on that and moved with it
- **The password policy was not in the sprint.** It was listed as a need in
  the Sprint 7 retrospective and it stays open
- **The integration tests changed language.** They were written in C++ and
  were converted to Python in the same pull request that separates them from
  the unit tests. The reason was also to use more technologies in the
  project, besides the lower cost of writing HTTP tests in Python
- **Comparing and downloading versions entered the sprint.** They were not
  issues at the start and they were picked up because the history view
  needed them to use the whole API
- **The endpoint of the root is `GET /root-contents`**, not `GET /root` as
  written in the Sprint 7 retrospective and in the first planning

## Blockers / learnings

- A build that uses every core of the machine makes it unusable. The scripts
  limit the build to 4 jobs and build only the server, and the tests are built
  by `test.sh`
- With Apple Clang, `std::jthread` needs `-fexperimental-library`, so the
  `CMakeLists.txt` adds it for Clang
- The GCC used on Linux must be version 14 or newer. The CI uses `g++-14`
  and the deployment guide asks for it
- The `path` of `POST /files` is a query parameter, not a field of the form,
  and the quick tour of the README had it wrong until the consolidation
- Writing the OpenAPI specification and the deployment guide found
  differences between the documentation and the code, which the
  consolidation fixed. Documentation written along with each feature
  drifted, so a review against the code at the end was needed
- Uploads and downloads hold the whole file in memory, which is still open
  from Sprint 7

## Carried over

To a future release (see the roadmap):

- Password policy in the backend
- Deployment of the server and the frontend, and automated deployment with
  GitHub Actions (#123)
- Orphan blob garbage collection (#91)
- Benchmarks (#94)
