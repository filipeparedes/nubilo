#pragma once

#include "storage/SqliteDb.h"

#include <string>
#include <expected>

namespace nubilo {

/**
 * @struct RegisterError
 * @brief Error returned when user registration fails.
 *
 * type distinguishes cases the caller needs to handle differently
 * e.g. mapping to different HTTP status codes, w/o needing to db error messages
 */
struct RegisterError {
    enum class Type { EmailAlreadyExists, DatabaseError } type;
    std::string msg;
};

/**
 * @brief Registers a new user: hashes the password and inserts a new
 * row into the users table.
 * @param db The database to insert into.
 * @param email The new user's email address. Must be unique.
 * @param password The new user's plaintext password.
 * @return The new user's id on success, or a RegisterError on failure.
 */
std::expected<int, RegisterError> registerUser(SqliteDb& db, const std::string& email, const std::string& password);

}