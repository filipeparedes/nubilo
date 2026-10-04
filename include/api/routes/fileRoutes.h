#pragma once

namespace nubilo {

class Router;
class Db;
class BlobStore;

/**
 * @brief Registers the file routes on the given router.
 * @param router The router to register the route on.
 * @param db A reference to the database.
 * @param blobStore A reference to the disk-backed blob store.
 */
void registerFileRoutes(Router& router, Db& db, BlobStore& blobStore);

}