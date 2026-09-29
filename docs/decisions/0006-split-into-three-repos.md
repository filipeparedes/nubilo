# 0006 - Split into three repositories (supersedes ADR 0001)

Supersedes ADR 0001.

## Context

ADR 0001 established a two-repository split: `nubilo` (the backend API)
and `nubilo-client` (an optional frontend, described at the time as a
React demonstration client).

While replanning Sprint 3, it became clear that "client" was being used
to describe two genuinely different things with different runtime
environments:

- A local sync client/CLI that runs on the end user's own machine,
  observes a local folder, and talks to the nubilo server over HTTP.
  This is close to the original project vision (a lightweight, portable
  tool, closer to git/gh than a typical web app).
- A hosted web frontend (React), served like any web application, that
  lets a user interact with their files through a browser without
  installing anything locally.

These have different toolchains (C++ and CMake vs npm/Vite), different
deployment models (a binary distributed to end users vs a hosted web
app), and different release cadences. Keeping both under a single
`nubilo-client` name and repository would have hidden that distinction.

## Decision

Split into three repositories:

- `nubilo`: the C++ server (unchanged from ADR 0001).
- `nubilo-client`: a local executable containing a CLI and the real sync
  client (file-state cache, change detection, diffing, conflict
  groundwork), installed and run on the end user's machine.
- `nubilo-web`: an optional, hosted React frontend, talking to `nubilo`
  directly over HTTP. It shares no code with `nubilo-client` since they
  are different languages.

## Reasoning

- Matches ADR 0001's own reasoning (toolchain separation) applied one
  level further, now that "client" has been shown to cover two
  meaningfully different runtime contexts.
- Keeps `nubilo-client` focused on being the lightweight, portable core
  experience the project was originally envisioned around, without
  entangling it with a web app's toolchain and deployment model.
- Makes the CLI, not the web frontend, the primary interface to the
  sync client, consistent with the project's original direction.

## Trade-offs accepted

- One more repository to set up and track (issues, milestones) compared
  to the original two-repo split.
- The web frontend and the CLI each need their own API-calling logic;
  there is no shared "API client" layer between them, since one is C++
  and the other TypeScript.