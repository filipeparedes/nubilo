#include "blob/chunker.h"

namespace nubilo {

std::expected<std::vector<std::string_view>, BlobError> chunkContent(std::string_view content, size_t chunkSize) {
    if (chunkSize == 0)
        return std::unexpected(BlobError{"Invalid chunk size. Must be greater than zero."});

    std::vector<std::string_view> chunks;
    chunks.reserve((content.size() + chunkSize-1) / chunkSize);

    for (size_t offset=0; offset<content.size(); offset+=chunkSize)
        chunks.emplace_back(content.substr(offset, chunkSize));

    return chunks;
}

}