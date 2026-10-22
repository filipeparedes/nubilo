#pragma once

namespace nubilo {

class Router;
class Db;
class BlobStore;
class ThreadPool;

/**
 * @brief Registers the files and folders routes on the given router.
 * @param router The router to register the route on.
 * @param db A reference to the database.
 * @param blobStore A reference to the disk-backed blob store.
 * @param pool (optional) Pointer to a ThreadPool. Used to store file chunks concurrently
 */
void registerFileRoutes(Router& router, Db& db, BlobStore& blobStore, ThreadPool* pool = nullptr);

}