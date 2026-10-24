#pragma once

#include "blob/BlobError.h"

#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace nubilo {

/**
 * @brief Checks that a path is acceptable as the path of a file or a folder.
 *
 * A valid path is made of segments separated by single slashes, with no slash at the start or at
 * the end, for example "a", "a/b" or "a/b/c.txt". It is rejected if it is empty, contains a
 * backslash, an empty segment ("a//b"), a "." or ".." segment, a control character, or is longer
 * than 1024 bytes.
 * @param path The path to check.
 * @return Nothing on success, or a BlobError saying why the path is not valid.
 */
std::expected<void, BlobError> validatePath(std::string_view path);

/**
 * @brief Lists the folders that contain a path, from the outermost to the innermost.
 *
 * The path is expected to be valid (see validatePath). For "a/b/c.txt" the result is {"a", "a/b"},
 * and for a path without slashes it is empty.
 * @param path The path of a file or folder.
 * @return The paths of its parent folders, outermost first.
 */
std::vector<std::string> parentPaths(std::string_view path);

}