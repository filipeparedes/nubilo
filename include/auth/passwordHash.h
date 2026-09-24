#pragma once

#include <expected>
#include <string>

namespace nubilo {

struct HashError {
    std::string msg;
};

/**
 * @brief Hashes a password using Argon2id, generating a random salt internally.
 * @param password The plaintext password to encode.
 * @return The encoded hash (includes salt and parameters), HashError on failure.
 */
std::expected<std::string, HashError> encodePassword(const std::string& password);

/**
 * Verifies a plaintext password against a previously encoded hash.
 * @param encodedHash The hash produced by encodePassword(), as stored in the database.
 * @param password The plaintext password to check.
 * @return true if the password matches the hash, false otherwise.
 */
bool verifyPassword(const std::string& encodedHash, const std::string& password);

}