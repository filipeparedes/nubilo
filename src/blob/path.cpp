#include "blob/path.h"

namespace nubilo {

namespace {

constexpr size_t MAX_PATH_LENGTH = 1024;

}

std::expected<void, BlobError> validatePath(std::string_view path) {
    if (path.empty())
        return std::unexpected(BlobError{"Path is empty."});
    if (path.size() > MAX_PATH_LENGTH)
        return std::unexpected(BlobError{"Path is too long."});
    if (path.front() == '/' || path.back() == '/')
        return std::unexpected(BlobError{"Path must not start or end with a slash."});

    for (char c : path) {
        const auto byte = static_cast<unsigned char>(c);

        if (c == '\\')
            return std::unexpected(BlobError{"Path must not contain backslashes."});
        if (byte < 0x20 || byte == 0x7F)
            return std::unexpected(BlobError{"Path must not contain control characters."});
    }

    // check each segment between slashes
    size_t start = 0;
    while (start <= path.size()) {
        size_t end = path.find('/', start);
        if (end == std::string_view::npos)
            end = path.size();

        const std::string_view segment = path.substr(start, end - start);

        if (segment.empty())
            return std::unexpected(BlobError{"Path must not contain empty segments."});
        if (segment == "." || segment == "..")
            return std::unexpected(BlobError{"Path must not contain '.' or '..' segments."});

        start = end + 1;
    }

    return {};
}

std::vector<std::string> parentPaths(std::string_view path) {
    std::vector<std::string> parents;

    // every slash ends a parent folder: "a/b/c.txt" -> "a", "a/b"
    size_t pos = path.find('/');
    while (pos != std::string_view::npos) {
        parents.emplace_back(path.substr(0, pos));
        pos = path.find('/', pos + 1);
    }

    return parents;
}


}