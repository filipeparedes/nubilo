#pragma once

#include <string>

namespace nubilo {

/// What kind of failure the BlobError describes, so callers can map it to the right response.
enum class BlobErrorType {
    Unknown, ///< To avoid generally. Default value for BlobError
    Internal, ///< Unexpected failure (db, disk..)
    Conflict ///< Request clashes with the current state
};

struct BlobError {
    std::string message;
    BlobErrorType type = BlobErrorType::Unknown;
};

}