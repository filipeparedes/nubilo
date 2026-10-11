# api

The HTTP layer: server, routing, authentication check and the route handlers. It turns requests into calls to the other modules (`auth`, `blob`) and their results into HTTP responses. It does not contain business logic.

## Structure

| File | Role |
|---|---|
| `HttpServer.h` / `HttpServer.cpp` | Owns the cpp-httplib server. `run()` listens on a port and blocks, `stop()` ends it. |
| `Router.h` / `Router.cpp` | Registry of routes (method, path, callback). Modules register routes here and `applyTo()` hands them to the server. |
| `middleware.h` / `middleware.cpp` | `requireAuth`: wraps a handler so it only runs for a valid session. |
| `ErrorResponse.h` / `ErrorResponse.cpp` | `writeErrorResponse`: the single place that builds error bodies. |
| `routes/authRoutes.*` | Registration, login and logout. |
| `routes/fileRoutes.*` | Upload, download, list and delete files. |
| `routes/healthRoutes.*` | The health check. |

## How it works

**Routing.** Each `routes/*` file exposes a `register...Routes` function that adds its routes to the `Router`. Modules never use `httplib::Server` directly (see ADR 0004), so the HTTP library can be replaced by changing only the `Router` internals. At startup `main.cpp` registers every group and applies the router to the server.

**Authentication.** A protected route is registered through `requireAuth(db, handler)`. The middleware reads `Authorization: Bearer <token>`, looks the token up in `sessions` and calls the handler with the token only if the session exists and has not expired. Otherwise the request is answered with 401.

**Ownership.** Handlers resolve the user from the token and check that the file belongs to them. A file that exists but belongs to someone else is answered with 404, the same as a missing file, so its existence is not revealed.

**Errors.** Every error has the same JSON body:

    {"error": {"code": "NOT_FOUND", "message": "File not found."}}

## Endpoints

The server listens on port 8080. Routes marked "auth" need the `Authorization: Bearer <token>` header.

| Method and path | Auth | Request | Success | Errors |
|---|---|---|---|---|
| `GET /health` | no | | 200 `{"status": "ok"}` | |
| `POST /auth/register` | no | JSON `email`, `password` | 201 `{"id": <userId>}` | 400 invalid body or email, 409 email already registered, 500 |
| `POST /auth/login` | no | JSON `email`, `password` | 200 `{"token": "<token>"}` | 400 invalid body, 401 invalid credentials |
| `POST /auth/logout` | yes | | 200 | 401, 500 |
| `POST /files` | yes | multipart field `file`, optional `path` (defaults to the file name) | 201 `{"id": <fileId>}` | 400 missing file or path, 401, 500 |
| `GET /files` | yes | | 200 array of `{id, path, size, contentType, createdAt, updatedAt}` | 401, 500 |
| `GET /files/{id}` | yes | | 200 file content, with `Content-Disposition: attachment` | 401, 404, 500 |
| `DELETE /files/{id}` | yes | | 204 | 401, 404, 500 |

## Notes

* A file is held in memory in full while it is uploaded and when it is downloaded.
* Routes capture `Db` and `BlobStore` by reference, so both must outlive the server.