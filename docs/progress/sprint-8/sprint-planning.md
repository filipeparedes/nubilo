# Sprint 8 - Planning

**Dates:** 1 - 8 Nov

**Goal:** Reach v1.0: finish the file browser of `nubilo-web` so that a user
can log in, navigate their folders, upload, download and delete files and
work with the versions of a file from the browser, and document the API and
the deployment.

## Scope

In order of priority, from the first to the last to be cut:

- `nubilo`: separate the root contents from the folder list. `GET /root-contents`
  for the contents of the root and `GET /folders` for the list of all the
  folders, which the file list view and the side panel of the frontend depend on
- `nubilo-web`: path navigation, including the side panel with the folder tree
- `nubilo-web`: file upload
- `nubilo-web`: file download and delete
- `nubilo-web`: file history list
- `nubilo-web`: restore a previous version
- Helper scripts to build, run and test (carried over from the backlog)
- Integration tests in Python, separate from the unit tests (replacing the
  integration tests in C++)
- OpenAPI specification of the API
- Deployment guide
- Documentation consolidation
- Final polish and fixes

Moved out of the sprint, to a future release: password policy, deployment of
the server and the frontend, automated deployment with GitHub Actions, orphan
blob garbage collection and benchmarks (deduplication and version savings,
upload time with and without the thread pool).

Not in the sprint at the start, because they were not issues yet: creating and
deleting folders in the browser, comparing two versions and downloading one
version. They are needed to exercise the whole API from the frontend, so they
are picked up if the five frontend issues finish early.

Configuration through environment variables was in the roadmap for this
sprint. It was replaced by the `config.json` file, which was pulled forward
to Sprint 7, so it is no longer in the scope.

~40h budget, per docs/roadmap.md. The work carried over from Sprint 7 and the
two changes to `nubilo` come on top of the v1.0 items of the roadmap, so the
sprint does not fit in the budget and items were left out. The order above is
the order in which they are cut.

## Success criteria

- A user can log in, navigate their folders, upload a file to the current
  folder, download it, delete it, see its versions and restore an earlier
  one, all from the browser against a running server
- The side panel shows the folder tree, and the listing and the tree come
  from the `GET /root-contents` and `GET /folders` endpoints
- The unit tests and the integration tests are separate and run in the CI
- The API is described by an OpenAPI specification, and the deployment guide
  lets someone run the server and the frontend on a Linux server
- The documentation matches the code
- Covered by tests consistent with the project's Definition of Done

## Risks / open questions

- The sprint is over budget before it starts. The frontend items alone are
  the bulk of it, and each one brings a call to the API, a hook, a screen and
  its documentation. The scope should be reviewed halfway, and the items at
  the end of the list are the ones that would be left out
- v1.0 is the last sprint of the roadmap, so what is not done here stays out
  of v1.0
- The change to the folder endpoints breaks the contract of `GET /folders`.
  The frontend is the only client for now, but the tests and the documentation
  of the endpoints have to change with it
- The side panel needs the whole folder tree. It is built in the frontend from
  the list of folders, so it depends on the new endpoint and on how the
  folders are shown while a folder is loading
- Deleting a folder deletes everything inside it, permanently. The
  confirmation in the browser has to make that clear
- Uploads and downloads hold the whole file in memory, in the API and in the
  browser, so large files may be slow or fail. It is still open from Sprint 7
- Deployment is new for the project: where the server runs, how the frontend
  is served, HTTPS and the permissions of the database and blob directories
  are still to be decided. The sprint only writes the guide. The deployment
  itself was moved to a future release
