#include "auth/passwordHash.h"
#include <argon2.h>
#include <print>
#include <random>

namespace nubilo {

constexpr int HASH_SIZE = 128;
constexpr int SALT_SIZE = 16;

/**
 * Generates a random salt. "Private" helper function.
 * @param salt Buffer to fill with random bytes.
 * @param saltLength Number of bytes to generate
 */
static void genSalt(uint8_t* salt, size_t saltLength) {
    std::random_device rd;
    std::uniform_int_distribution<int> dist(0, 255);

    for (size_t i = 0; i < saltLength; i++)
        salt[i] = static_cast<uint8_t>(dist(rd));
}

std::expected<std::string, HashError> encodePassword(const std::string& password) {
    uint8_t salt[SALT_SIZE];
    char encodedHash[HASH_SIZE];

    genSalt(salt, SALT_SIZE);

    int result = argon2id_hash_encoded(
        2,
        1 << 16,
        2,
        password.c_str(), password.length(),
        salt, sizeof(salt),
        32,
        encodedHash, sizeof(encodedHash)
    );

    if (result != ARGON2_OK) {
        std::println("Error trying to hash password: {}", argon2_error_message(result));
        return std::unexpected(HashError{"Error trying to hash the password."});
    }

    return std::string(encodedHash);
}

bool verifyPassword(const std::string& encodedHash, const std::string& password) {
    int result = argon2id_verify(encodedHash.c_str(), password.c_str(), password.length());

    return result == ARGON2_OK;
}

}