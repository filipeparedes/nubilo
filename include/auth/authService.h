#pragma once

#include "storage/SqliteDb.h"

#include <string>
#include <expected>

namespace nubilo {

/**
 * @struct AuthError
 * @brief Error returned when user registration fails.
 *
 * type distinguishes cases the caller needs to handle differently
 * e.g. mapping to different HTTP status codes, w/o needing to db error messages
 */
struct AuthError {
    enum class Type { EmailAlreadyExists, DatabaseError, InvalidCredentials} type;
    std::string msg;
};

/**
 * @brief Registers a new user: hashes the password and inserts a new
 * row into the users table.
 * @param db The database to insert into.
 * @param email The new user's email address. Must be unique.
 * @param password The new user's plaintext password.
 * @return The new user's id on success, or an AuthError on failure.
 */
std::expected<int64_t, AuthError> registerUser(SqliteDb& db, const std::string& email, const std::string& password);

/**
 * @brief Attempts to authenticate a user
 * @param db The database where the user is registered
 * @param email The user's email
 * @param password The user's plaintext password
 * @return The user's session token on success, or an AuthError on failure
 */
std::expected<std::string, AuthError> authenticateUser(SqliteDb& db, const std::string& email, const std::string& password);

}