#include "auth/authService.h"
#include "auth/passwordHash.h"

namespace nubilo {

static bool isUniqueConstraintError(const std::string& msg) {
    return msg.find("UNIQUE constraint failed") != std::string::npos;
}

std::expected<int64_t, RegisterError> registerUser(SqliteDb& db, const std::string& email, const std::string& password) {
    auto hashRes = encodePassword(password);
    if (!hashRes) {
        return std::unexpected(RegisterError{RegisterError::Type::DatabaseError, hashRes.error().msg});
    }

    auto execRes = db.exec("INSERT INTO users (email, password_hash) VALUES (?, ?)", {email, hashRes.value()});
    if (!execRes) {
        RegisterError::Type type = isUniqueConstraintError(execRes.error().msg)
            ? RegisterError::Type::EmailAlreadyExists
            : RegisterError::Type::DatabaseError;

        return std::unexpected(RegisterError{type, execRes.error().msg});
    }

    int64_t id = db.lastInsertId();
    return id;
}

}