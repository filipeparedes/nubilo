# Roadmap

High-level phases for the project. Two categories from the original proposal "REST API & Authentication" (60h) and "Data & Metadata Persistence" (40h) are split across two phases below, because authentication cannot be built
without some persistence in place first.

The hours in the development phases also account for any research/learning that might have to be done.

## Core development phases

| # | Phase | Description | Hours 
|---|-------|-------------|------|
| 1 | REST API & Basic Persistence | HTTP server skeleton, routing, and the minimal SQLite schema needed to support it (general structure, not yet user-specific). | ~50h
| 2 | Authentication & Extended Persistence | Session/token-based auth, user table, and the rest of the metadata/sync-state schema. | ~50h 
| 3 | Sync Client | Change detection and communication with the server. Uses a simple whole-file SHA-256 hash for change detection at this stage-full chunking comes in Phase 4. | ~60h 
| 4 | File Chunking, Hashing & Deduplication | Splitting files into blocks, extending the simple hashing from Phase 3 into block-level hashing, and deduplication. The technical core of the project. | ~90h
| 5 | File Versioning | Tracking previous versions of a file, enabling rollback/history. | ~50h 
| 6 | Frontend (TypeScript) | Navigation, file history, upload/download UI. Lives in the separate `nubilo-client` repository. | ~70h
| 7 | Extensions | Real-time notifications and file sharing between users. Stretch goals, tackled once the core system is solid. | ~50h 

**Subtotal:** ~420h

## Cross-cutting activities

| Activity | Description | Hours | Distribution |
|----------|-------------|------:|---------------|
| Integration & Unit Testing & System Validation | Unit tests per module as it's built, integration tests as modules connect, end-to-end validation once the system is complete. | ~40h | Spread across Phases 1-7, heavier towards Phases 4-5 and a final validation pass after Phase 7. |
| Documentation | ADRs, architecture notes, sprint planning/retrospectives, API/data-model docs as they emerge, final report and defense prep. | ~30h | Continuous from Sprint 0 onward; the last portion concentrated on final report consolidation. |

**Subtotal:** ~70h

## Total: ~490h

## Notes on sequencing decisions

- **Phase 1/2 split:** authentication needs a place to store users and
  sessions, so a minimal persistence layer is built in Phase 1 alongside
  the API skeleton, ahead of the bulk of the "Persistência de dados"
  category, which continues in Phase 2 alongside auth.
- **Phase 3/4 relationship:** the sync client (Phase 3) needs *some* way to
  detect that a file changed, which conceptually belongs to Phase 4's
  hashing work. Phase 3 uses a simple whole-file SHA-256 hash as a
  placeholder for this, which Phase 4 then replaces/extends with proper
  block-level chunking, hashing, and deduplication.