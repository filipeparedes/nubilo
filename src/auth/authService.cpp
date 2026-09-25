#include "auth/authService.h"
#include "auth/passwordHash.h"

#include <random>

namespace nubilo {

static bool isUniqueConstraintError(const std::string& msg) {
    return msg.find("UNIQUE constraint failed") != std::string::npos;
}

static std::string genToken() {
    constexpr size_t TOKEN_BYTES = 32;
    uint8_t bytes[TOKEN_BYTES];

    std::random_device rd;
    std::uniform_int_distribution<int> dist(0, 255);
    for (auto& byte : bytes)
        byte = static_cast<uint8_t>(dist(rd));

    std::string token;
    for (auto byte : bytes)
        token += std::format("{:02x}", byte);

    return token;
}


std::expected<int64_t, AuthError> registerUser(SqliteDb& db, const std::string& email, const std::string& password) {
    auto hashRes = encodePassword(password);
    if (!hashRes)
        return std::unexpected(AuthError{AuthError::Type::DatabaseError, hashRes.error().msg});

    auto execRes = db.exec("INSERT INTO users (email, password_hash) VALUES (?, ?)", {email, hashRes.value()});
    if (!execRes) {
        AuthError::Type type = isUniqueConstraintError(execRes.error().msg)
            ? AuthError::Type::EmailAlreadyExists
            : AuthError::Type::DatabaseError;

        return std::unexpected(AuthError{type, execRes.error().msg});
    }

    int64_t id = db.lastInsertId();
    return id;
}

}