#pragma once

#include <expected>
#include <string>
#include <vector>
#include <string_view>

namespace nubilo {

struct ChunkError {
    std::string message;
};

/**
 * @brief Splits content into consecutive chunks of at most chunkSize bytes.
 *
 * Every chunk has exactly chunkSize bytes except the last one, which holds whatever remains and may be smaller.
 * Empty content produces no chunks. Joining the chunks in order gives back the original content.
 *
 * @note The returned views point into content, no bytes are copied. They are only valid while the memory
 * behind content stays alive and unchanged.
 *
 * @param content The bytes to split.
 * @param chunkSize The maximum size of each chunk in bytes. Must be greater than zero.
 * @return The chunks in order, or a ChunkError if chunkSize is zero.
 */
std::expected<std::vector<std::string_view>, ChunkError> chunkContent(std::string_view content, size_t chunkSize);

}