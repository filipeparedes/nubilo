# 0004 - Custom Router over direct httplib usage

## Context

cpp-httplib doesn't provide a router abstraction - routes are registered
directly on the `httplib::Server` object (`server.Get(...)`,
`server.Post(...)`), with each module that wants to expose an endpoint
either needing direct access to that server instance, or some agreed-upon
way to register its routes on it.

Two approaches were considered:

- Each module (Auth, Sync, Storage) takes a reference to the
  `httplib::Server` directly and registers its own routes on it via a
  `registerRoutes(httplib::Server&)` method.
- A custom `Router` class sits between `HttpServer` and the modules: modules
  register routes against the `Router` (method, path, handler), and the
  `Router` applies them to the underlying `httplib::Server`.

This mirrors a decision other backend frameworks in different languages
(Express, ASP.NET Core, etc.) resolve with a dedicated router/middleware
layer, which is the model this project is more familiar with and considers
more scalable as the number of endpoints grows across modules.

## Decision

Introduce a custom `Router` class. Modules register their routes against
the `Router`, not directly against `httplib::Server`. The `Router` owns the
list of registered routes and is responsible for applying them to the
underlying `httplib::Server`.

## Reasoning

- Gives a single, central place to add behavior that should apply to every
  route (e.g. logging each registered route, timing requests, generic
  validation), without touching each module individually.
- Decouples modules from the specific HTTP library in use - modules depend
  on the project's own `Router` interface, not on `httplib::Server`
  directly. If the HTTP library were ever replaced, only the `Router`'s
  internals would need to change.
- Matches a router/middleware pattern already familiar from other
  ecosystems, making the codebase more approachable to reason about.

## Trade-offs accepted

- More code to write and maintain than using `httplib::Server` directly -
  the `Router` re-implements, at a small scale, dispatch functionality the
  underlying library already provides.
- This goes against the project's general bias towards minimal abstraction
  (see [ADR 0002](./0002-modular-monolith-vs-microservices.md)) - accepted
  here specifically because the router sits at a boundary crossed by every
  module in the system, where the cost of the abstraction is paid once and
  the benefit (central control point, decoupling from the HTTP library) is
  judged to outweigh it, unlike e.g. splitting the backend into
  microservices.
