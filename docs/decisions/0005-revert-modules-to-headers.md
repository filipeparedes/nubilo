# 0005 - Revert from C++20 modules to traditional headers

(supersedes the Modules section previously defined in  docs/coding-standards.md)

## Context

The project initially adopted C++20 modules (see the original Modules
section in coding-standards.md) to build a modern, cleanly bounded
codebase. In practice, this ran into repeated toolchain-level issues when
combined with non-modularized third-party libraries (cpp-httplib):

- Duplicate processing of httplib.h across different translation units
  (once via a module's global module fragment, once via a plain #include)
  caused compilation failures under AddressSanitizer/UBSan builds
  ("cannot add 'abi_tag' attribute in a redeclaration", <regex>
  redefinition errors).
- Attempting to work around this by pinning an older, more stable LLVM
  version (18) surfaced further toolchain problems specific to how
  Homebrew packages older LLVM versions as keg-only, without proper
  libc++ linkage exposed.
- Significant time was spent debugging toolchain/environment issues
  rather than implementing project features, with no fully stable
  configuration found within reasonable effort.

## Decision

Revert from C++20 modules to traditional header/source (.h/.cpp) file
pairs for all project code. Adopt a mirrored `include/` + `src/` folder
structure (see coding-standards.md), applied consistently regardless of
module size.

## Reasoning

- Traditional headers with third-party libraries (cpp-httplib, and later
  SQLite) are a well-trodden path with no equivalent toolchain fragility.
- Removes an entire class of environment-specific bugs unrelated to the
  project's actual logic, at a point in the schedule where debugging time
  is especially costly.

## Trade-offs accepted

- Loses the automatic dependency graph and reduced recompilation modules
  provide, mitigated partially via forward declarations where possible
  (see coding-standards.md).
- Requires manually maintaining a mirrored include/ + src/ structure
  rather than a single .cppm file per component.
- Any documentation or code written referencing C++20 modules (see
  earlier commit history) reflects an approach no longer in use.