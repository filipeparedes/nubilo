# Sprint 7 - Planning

**Dates:** 25 Oct - 1 Nov

**Goal:** Build the first usable version of `nubilo-web`: the user can log in,
browse their folders, upload and download files and see the history of a file.
The backend gets the small changes the browser needs.

## Scope

- CORS support in `nubilo`, so the browser frontend can call the API
  (carried over from Sprint 6)
- Frontend project setup: Vite + React + TypeScript, in `nubilo-web`
  (carried over from Sprint 6)
- Frontend: API integration layer (auth, fetching files)
- Frontend: file browser / navigation UI
- Frontend: upload/download UI
- Frontend: file history view, with version comparison if the view needs it


~67h budget, per docs/roadmap.md. The sprint plan is the one in the roadmap.
The two items carried over from Sprint 6 come first, since the rest of the
frontend cannot be tested against the API without them.

## Success criteria

- A browser running `nubilo-web` can call the API (CORS) without errors
- A user can register or log in, and the session is kept while using the app
- A user can navigate their folders, see the files in each one, upload a file
  to the current folder and download a file
- A user can see the versions of a file and open an earlier one, using the
  version endpoints from Sprint 6
- The flow log in, upload, list, download and view history works from the
  browser against a running server
- Covered by tests consistent with the project's Definition of Done

## Risks / open questions

- First sprint with real work in `nubilo-web`: the project structure, how the
  app stores the session token and how it finds the server are still open
- There is no way to move or rename a file or folder in the API. If the file
  browser needs it, it would have to be added in `nubilo`
- Uploads and downloads hold the whole file in memory, both in the API and in
  the browser, so large files may be slow or fail
- Showing the difference between two versions depends on what the comparison
  endpoint returns, which is only the chunks that changed. If the history
  view needs a content diff, it may stay out of this sprint
- The sprint combines a new frontend with the work left from the previous one.
  If time runs short, the version comparison in the history view and the
  end-to-end pass are the items that would carry over