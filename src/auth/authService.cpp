#include "auth/authService.h"
#include "auth/passwordHash.h"

#include <random>
#include <chrono>

namespace nubilo {

static constexpr const char* SESSION_DURATION = "+7 days";

static bool isUniqueConstraintError(const std::string& msg) {
    return msg.find("UNIQUE constraint failed") != std::string::npos;
}

static std::string genToken() {
    constexpr size_t TOKEN_BYTES = 32;
    uint8_t bytes[TOKEN_BYTES];

    std::random_device rd;
    const auto now = std::chrono::system_clock::now().time_since_epoch().count();

    //mix the OS entropy source with the current timestamp, which completely avoids a weak or
    //deterministic random_device producing the same seed twice
    std::seed_seq seed{rd(), rd(), static_cast<unsigned>(now), static_cast<unsigned>(now >> 32)};
    std::mt19937_64 engine(seed);

    std::uniform_int_distribution<int> dist(0, 255);
    for (auto& byte : bytes)
        byte = static_cast<uint8_t>(dist(engine));

    //hex-encode each byte, 2 characters per byte, 64 character total
    std::string token;
    for (auto byte : bytes)
        token += std::format("{:02x}", byte);

    return token;
}


std::expected<int64_t, AuthError> registerUser(Db& db, const std::string& email, const std::string& password) {
    auto hashRes = encodePassword(password);
    if (!hashRes)
        return std::unexpected(AuthError{AuthError::Type::DatabaseError, hashRes.error().msg});

    auto execRes = db.exec("INSERT INTO users (email, password_hash) VALUES (?, ?);", {email, hashRes.value()});
    if (!execRes) {
        AuthError::Type type = isUniqueConstraintError(execRes.error().msg)
            ? AuthError::Type::EmailAlreadyExists
            : AuthError::Type::DatabaseError;

        return std::unexpected(AuthError{type, execRes.error().msg});
    }

    int64_t id = db.lastInsertId();
    return id;
}

std::expected<std::string, AuthError> authenticateUser(Db& db, const std::string& email, const std::string& password) {
    auto queryRes = db.query("SELECT id, password_hash FROM users WHERE email = ?;", {email});
    if (!queryRes)
        return std::unexpected(AuthError{AuthError::Type::DatabaseError, queryRes.error().msg});

    if (queryRes.value().empty() || !verifyPassword(queryRes.value()[0]["password_hash"].get<std::string>(), password))
        return std::unexpected(AuthError{AuthError::Type::InvalidCredentials, "Invalid credentials."});

    int64_t id = queryRes.value()[0]["id"].get<int64_t>();
    auto token = genToken();

    auto insertRes = db.exec(
    "INSERT INTO sessions (token, user_id, expires_at) VALUES (?, ?, datetime('now', ?));",
{token, std::to_string(id), SESSION_DURATION});
    if (!insertRes)
        return std::unexpected(AuthError{AuthError::Type::DatabaseError, insertRes.error().msg});

    return token;
}

std::expected<void, AuthError> invalidateSession(Db& db, const std::string& token) {
    auto delRes = db.exec("DELETE from sessions WHERE token = ?;", {token});
    if (!delRes)
        return std::unexpected(AuthError{AuthError::Type::DatabaseError, "Couldn't invalidate session."});

    return {};
}

}