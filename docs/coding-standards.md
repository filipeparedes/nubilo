# Coding Standards

This document defines the coding conventions used throughout the Nubilo
codebase.

## Naming

- **Classes / structs:** `PascalCase` (e.g. `SyncEngine`, `FileMetadata`)
- **Functions / variables:** `camelCase` (e.g. `computeHash`, `filePath`)
- **Constants:** `UPPER_CASE` (e.g. `MAX_CHUNK_SIZE`)
- **Files**: match the primary class they contain, `PascalCase.cppm` (e.g. `SyncEngine.cppm`). Files with free functions only use `camelCase.cppm` (e.g. `hashUtils.cppm`). 
- **Namespaces:** all project code lives under the `nubilo` namespace.

## Documentation

- Every file, class, struct, and function must have a Doxygen comment
  (`/** ... */`) describing its purpose,
  explaining *why* it exists, *what* it's responsible for and *how* to use it.
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

## Modules

- Project code is organized as C++20 modules, not header/source pairs.
- Module interface files use the `.cppm` extension; match the filename to
  the primary class/component it exports.
- One module per logical component (e.g. `SyncEngine.cppm`,
  `FileMetadata.cppm`). Use module partitions (`module SyncEngine:Internal;`)
  if a module grows large enough to need internal-only splitting.
- Import order: standard library first (`import std;`, available under
  C++23), then third-party libraries, then project modules, each group
  separated by a blank line, same spirit as the old include-order rule.
- No `using namespace std;` inside module interfaces; acceptable sparingly
  in module implementation units if it genuinely improves readability.
- Anything not yet modularized by its library (some third-party
  dependencies may still only ship headers) is included via a global module
  fragment (`module; #include <...> export module X;`) rather than a plain
  `#include` scattered in the module body.

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