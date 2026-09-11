# 0002 - Modular monolith vs microservices

## Context

Nubilo's backend needs to handle several distinct concerns: HTTP routing,
authentication, file synchronization logic, and storage. One option
considered was splitting these concerns into separate microservices, each
running as its own process with its own boundary.

This project is developed solo, within a fixed budget of roughly 486 hours,
by a student with a course to finish (having already secured employment).
The project runs on a single machine (macOS) with no distributed deployment
target, and there is no expected user base at a scale that would require
independent scaling of individual components.

## Decision

The backend is built as a **modular monolith**: a single C++ process,
internally divided into clearly bounded modules (API layer, Auth, Sync
engine, Storage), rather than a set of independently deployed microservices.

## Reasoning

- Microservices introduce infrastructure costs (separate processes, ports,
  inter-service communication, possibly service discovery and
  orchestration) that provide no benefit at this scale — there's no real
  concurrent load or team to justify independent scaling or deployment.
- Communication between microservices (HTTP/gRPC) would add failure modes
  (timeouts, serialization issues, partial failures) to debug, for
  components that are all running on the same machine, written by the same
  person.
- The hour budget is limited; time spent on inter-service plumbing is time
  not spent on the actual synchronization logic, which is the point of the
  project.
- A modular monolith still provides clear internal boundaries between
  concerns, which is the main organizational benefit people look for in
  microservices, without the operational overhead.

## Trade-offs accepted

- If the project ever needed real independent scaling (e.g. running the sync
  engine as a separate background daemon), that would require extracting a
  module later. This is considered acceptable, since the internal module
  boundaries are designed to make that extraction straightforward if it ever
  becomes necessary.
- All modules currently share a single database connection/process; there's
  no isolation between them at the OS/process level.
