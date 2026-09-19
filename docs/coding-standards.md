# Coding Standards

This document defines the coding conventions used throughout the Nubilo
codebase.

## Naming

- **Classes / structs:** `PascalCase` (e.g. `SyncEngine`, `FileMetadata`)
- **Functions / variables:** `camelCase` (e.g. `computeHash`, `filePath`)
- **Constants:** `UPPER_CASE` (e.g. `MAX_CHUNK_SIZE`)
- **Files:** match the primary class they contain, `PascalCase.h` /
  `PascalCase.cpp` (e.g. `HttpServer.h` / `HttpServer.cpp`). Files with
  free functions only use `camelCase.h` / `camelCase.cpp` (e.g.
  `hashUtils.h` / `hashUtils.cpp`). Every component's declaration lives
  under `include/`, mirrored by its implementation under `src/`, at the
  same relative path (e.g. `include/api/Router.h` +
  `src/api/Router.cpp`), applied consistently regardless of file size.
- - **Namespaces:** all project code lives under the `nubilo` namespace.

## Documentation

- Every class, struct, and function declaration in header files must have a Doxygen comment
  (`/** ... */`) describing its purpose,
  explaining *why* it exists, *what* it's responsible for and *how* to use it.
- File-level header blocks (`@file`, `@author`, `@copyright`, `@version`,
    `@date`) are not used as git history already covers authorship and
    change dates more reliably than a static comment. Doxygen is applied
    only where it adds something beyond the name of a function, class, or
    struct.
- Non-obvious or important blocks of logic within a function should have a
  short inline comment explaining intent.
- Trivial getters/setters and self-explanatory one-liners don't need a full
  Doxygen block, a single-line comment is enough, or none at all if the
  name says it all.

## Memory management

- **RAII always.** No raw `new`/`delete`. Ownership is expressed through
  `std::unique_ptr` / `std::shared_ptr`; resources like file handles,
  sockets, or DB connections are wrapped in RAII types so cleanup happens
  automatically, even on early return or exception.
- Raw pointers may be used for non-owning references only (e.g. observing
  an object without taking ownership), never for ownership.

## Formatting

- Brace style: K&R (opening brace on the same line).
- Indentation: 4 spaces, never tabs.
- Maximum line length: 100 characters.
- Single-line blocks (`if`, `for`, `while`) omit braces `{ }`.

## Const-correctness

- Mark everything `const` that can be: parameters, member functions that
  don't mutate state, local variables that aren't reassigned.
- Prefer passing non-trivial types (strings, vectors, custom structs) by
  `const&` rather than by value, unless a copy is genuinely needed.

## Headers & Includes

- `#pragma once` at the top of every header.
- Every component's public interface lives in `include/<path>/Name.h`;
  its implementation lives in `src/<path>/Name.cpp`, mirroring the same
  relative path. Applied to every file, including small ones. No
  exceptions based on size, to keep navigation predictable.
- Use a forward declaration (`class Foo;`) instead of `#include`-ing a
  type's full header whenever a file only needs a reference or pointer
  to that type, not the full type, no method calls on it, not stored by
  value. This reduces include bloat and cascading recompilation when the
  included header changes. Include the full header only where the
  complete type is actually needed (e.g. calling its methods, storing it
  by value as a class member).
- Include order: matching header first (in `.cpp` files, in quotes), then
  C++ standard library, then third-party libraries, then other project
  headers, each group separated by a blank line.
- No `using namespace std;` in headers (acceptable sparingly in `.cpp`
  files if it genuinely improves readability).
- Third-party headers whose types are stored by value as class members
  (e.g. `httplib::Server`) must be included in that class's own header,
  not just its `.cpp` — the compiler needs the complete type to know the
  class's size.

```cpp
module;
#include <string>
#include <iostream>

export module MyModule;

export namespace nubilo {
      //...
}
```

- Third-party dependencies, that aren't modularized follow the same global module fragment pattern. 
- Import order: project modules first, each group
  separated by a blank line, same spirit as the old include-order rule.
- No `using namespace std;` inside module interfaces; acceptable sparingly
  in module implementation units if it genuinely improves readability.


## Error handling

- Prefer `std::expected` (C++23) for operations that can fail in an
  expected way (e.g. "file not found", "hash mismatch") over throwing
  exceptions for control flow.
- Reserve exceptions for truly exceptional/unrecoverable situations.
- Never ignore a return value that indicates possible failure (file I/O,
  DB calls, etc.), this overlaps with the Definition of Done's
  correctness checks.

## Miscellaneous

- Use `nullptr`, never `NULL` or `0`, for null pointers.
- Use `auto` when the type is obvious from context (e.g. iterators, results
  of `make_unique`) or when it improves readability and not as a default for
  every declaration.
- Avoid global mutable state; if shared state is unavoidable, make
  ownership explicit (e.g. passed via reference/pointer, not a hidden
  singleton) unless a singleton is a deliberate, documented choice.
- One class per file, matching filename to class name (see Naming).
