# Sprint 7 - Retrospective

**Dates:** 25 Oct - 1 Nov

**Goal:** Build the first usable version of `nubilo-web`: the user can log in,
browse their folders, upload and download files and see the history of a
file. The backend gets the small changes the browser needs.

**Time:** ~60h against a ~67h budget. The two carried-over items and the
configuration file were done first, and the rest of the time went into the
frontend. The sprint ends with the foundation of `nubilo-web` finished and
with the file browser, the upload, the download and the history still to do.

## Done

`nubilo`:

- CORS support (#136): `HttpServer::enableCors(origin)` allows one origin,
  answers the preflight request and adds the CORS headers only when the
  `Origin` of the request is the allowed one, including on error responses.
  The origin is read from the configuration. Covered by five integration
  tests
- Configuration file (#100): `config.json` with the host, port, allowed
  origin, database path, blob directory, number of threads and session
  duration. It is read and validated at startup by `loadConfig`, there are
  no defaults in the code, and a file with a wrong or missing value stops the
  server with a clear message. The application version moved into the file
- Tests and documentation: the suite grew from 195 to 209 tests. The module
  README, the root README (configuration table) and the bibliography were
  updated

`nubilo-web`:

- Project setup (#1): Vite, React and TypeScript, with Tailwind, React Router,
  TanStack Query, ESLint, Prettier and Vitest. The settings (API address and
  dev server port) are in `public/config.json`, read when the app starts, so
  they change without rebuilding
- API client (#4): `request()` with the token, the query and the JSON body,
  `ApiError` with the code and message of the backend, and the token kept in
  `localStorage`. Covered by Vitest tests
- Session handling (#6): the authentication calls, a session context
  (`AuthProvider`, `useAuth`), the route guard `RequireAuth` and the handling
  of a rejected session (401), which ends the session in the UI
- Authentication pages (#5): login and register (with password confirmation)
  in a shared `AuthLayout`, and the first set of reusable components built
  from the design: button, field, modal, rule title, status line and wordmark
- App layout and routing (#3): the layout of the app after login (top bar
  with the state of the API, command bar, side panel, status bar), the health
  check hook, the email kept with the session and all the routes of the app
  defined in one place
- File list view: the files and folders of the root in a generic table
  component (`DataTable`), with a filter, a counter and the loading, empty and
  error states. It needs the backend change described below
- Documentation: a README for each frontend folder and the bibliography

## What changed vs the plan

- **The sprint did not reach the success criteria about the file browser.**
  Navigating the folders, uploading, downloading and the history of a file
  were not finished. The plan was too wide for a first sprint in a new
  project, as the planning already warned, and the foundation (client,
  session, authentication, layout, design components) took the time. Five
  issues of `nubilo-web` move to Sprint 8 (see Carried over). The sprint
  planning was left as it was written and the change is recorded here
- **The configuration file was pulled forward from Sprint 8.** CORS needs the
  allowed origin to be a setting, and putting it in the code would have been
  undone a few days later. The file was done first and CORS was built on it
- **The frontend work was split into more issues than planned.** "API
  integration layer" became the API client, the session and the
  authentication pages, and the file browser became several issues (list,
  navigation, upload, download, history, restore), each with its own pull
  request
- **The endpoints of the folders will change.** While building the list, it
  became clear that `GET /folders` returns the contents of the root while
  `GET /folders/{id}` returns the contents of a folder, so the same
  collection means two things. It will be split: `GET /root` for the contents
  of the root and `GET /folders` for the list of all the folders, which also
  lets the side panel draw the whole tree in one call. The file list view
  depends on it
- **Parts of the design have no data behind them** and were left out: the
  storage meter and the name of the node. The login only returns a token, so
  the email of the session is kept by the frontend

## Blockers / learnings

- The first sprint in a new project is mostly structure. Choosing where each
  thing lives (calls in `api`, hooks in `hooks`, paths in `routes.ts` and
  `endpoints.ts`) took time but made each new screen smaller
- Newer Node versions have their own `localStorage` that does not work
  without a file, which broke the client tests. They now use an in-memory
  replacement
- The server accepts any password, even a single character. A password policy
  is needed in the backend, where it cannot be bypassed
- `listFolder` compares the paths with `substr` and `instr`, so each listing
  reads all the rows of the user, and it holds the database lock while it
  does. It is fine at this scale but it would not scale to a very large
  number of files
- `GET /files` returns every file in a flat list and the file browser does not
  need it. It stays for other clients, and the frontend still has to decide
  how to cover it
- Uploads and downloads hold the whole file in memory, which is still open
  from the planning

## Carried over

`nubilo-web` (Sprint 8):

- Path navigation, including the side panel with the folder tree
- File upload
- File download and delete
- File history list
- Restore a previous version

Not issues yet: creating and deleting folders, comparing two versions and
downloading one version.
