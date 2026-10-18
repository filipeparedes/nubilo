# Sprint 6 - Planning

**Dates:** 18 - 25 Oct

**Goal:** Add folder support to the API, before anything is built on top of
it, set up continuous integration, complete the versioning of files, and
start `nubilo-client` with the pieces that let it compare local files with
the server.

## Scope

- Folder support: files organised in folders, with folder listing. It changes
  the API contract, which is why it comes before the client (#92)
- CI pipeline: build and run the tests on every push and pull request
- Versioning completion: an endpoint to list the versions of a file and a way
  to compare two versions
- Groundwork for conflict detection: what the server needs to know to tell
  that a file was changed in two places
- `nubilo-client`: API client for server communication
- `nubilo-client`: diff computation between local and remote state
- `nubilo-client`: change detection loop
- `nubilo-client`: local file-state cache

~68h budget, per docs/roadmap.md. This is the widest sprint so far, since it
combines work in `nubilo` with the start of a second repository. Should also be the
toughest and will probably leave backlog.

## Success criteria

- A user can create folders, put files in them and list the contents of a
  folder, and the changes to the API are documented in the module READMEs
- Every push and pull request runs the build and the tests automatically,
  and a failing test fails the check
- The versions of a file can be listed, and two versions can be compared to
  see which chunks differ
- It is decided, and recorded in an ADR, how the server detects an upload
  that conflicts with a newer version
- The client can log in, list, upload and download files, compare the local
  state with the server and detect local changes, keeping a local cache of
  the state of the files
- Covered by tests consistent with the project's Definition of Done

## Risks / open questions

- The folder model is not decided. Paths are already strings that can contain
  slashes, so folders could be implied by path prefixes, or they could be rows
  of their own. This affects empty folders, renaming and listing, and is
  likely to need an ADR
- Folder support changes the responses of the existing endpoints, so the
  tests and the documentation of the file endpoints have to change with it
- Conflict detection needs to know which version a client's change was based
  on, and the upload endpoint does not carry that yet
- It is the first sprint with work in two repositories, and the client has
  decisions still open: how it finds the server and how it is configured
- With the folders, the CI pipeline, the versioning and the client all in one
  sprint, the scope should be reviewed halfway. The client items at the end
  of the list are the ones that would carry over if time runs short
- The CI pipeline is set up for `nubilo` first. Whether and when
  `nubilo-client` gets its own is to be decided once it has code to build