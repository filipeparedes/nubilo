# Sprint 5 - Planning

**Dates:** 11 - 18 Oct

**Goal:** Add file versioning on top of the chunk storage built in Sprint 4:
keep the history of each file as a list of versions, store only the chunks
that changed between versions, and allow restoring a previous version.
Also verify the deduplication and reassembly work that Sprint 4 already
covers, and document the project's use of AI.

## Scope

- Deduplication across files/users (#28). The shared chunk store from
  Sprint 4 already stores identical chunks once across files and users, so
  this item is to verify it against the roadmap and cover any gap with tests
- File reassembly from chunks (#29). `readFile` already rebuilds a file
  through its manifest, so this item is to verify it, including large files
  and a missing chunk, and cover any gap with tests
- Version metadata table (#30): file version history
- Version diffing (#31): each version stores only the chunks that changed
- Rollback mechanism (#32): restore a previous version
- Document AI usage in the project (#78)
- Update documentation (#89), to reflect the changes since the last update

~68h budget, per docs/roadmap.md. 

## Success criteria

- A file can have several versions, and each version lists its chunks in
  order
- A new version of a file that differs in a single chunk adds only that
  chunk to the store, and the unchanged chunks are reused
- Restoring a previous version makes the file return exactly that version's
  content
- Identical chunks uploaded by different users are stored once, and a
  multi-chunk file is rebuilt with the same content it was uploaded with
- A document describes how AI was used in the project, and the project
  documentation matches the code after Sprints 3 and 4
- Covered by tests consistent with the project's Definition of Done

## Risks / open questions

- Versioning changes the data model. Today `file_chunks` is keyed by the
  file, and each upload creates a new file. Where the manifest of each
  version lives, and how the file points to its current version, is still to
  be designed. This is likely to need an ADR
- It is not decided when an upload counts as a new version of an existing
  file and when it creates a new file. The upload endpoint has no notion of
  a file being replaced yet
- Old versions keep their chunks referenced. The design should keep it
  possible to count references to a chunk, which orphan blob garbage
  collection will need later
- Version listing and comparison endpoints are planned for Sprint 6, so this
  sprint is about the model and the rollback, not the API around it
- With #28 and #29 largely covered, the effort of the sprint is in the
  versioning design, which is the item with the most uncertainty