# auth

User accounts and login sessions: registration, password hashing, login and logout. The check that protects a route (`requireAuth`) lives in the `api` module and uses the sessions created here.

## Structure

| File | Role |
|---|---|
| `authService.h` / `authService.cpp` | `registerUser`, `authenticateUser` and `invalidateSession`, plus the `AuthError` type. |
| `passwordHash.h` / `passwordHash.cpp` | `encodePassword` and `verifyPassword`, built on Argon2id, plus the `HashError` type. |

## How it works

**Registration.** `registerUser` hashes the password and inserts the user into `users`. The email is unique, and a duplicate returns `AuthError::Type::EmailAlreadyExists`. The insert and `lastInsertId` run under `db.lock()`, so the returned id always belongs to this user.

**Passwords.** They are hashed with Argon2id (2 passes, 64 MiB of memory, 2 lanes, 32-byte hash) using a random 16-byte salt per password. The stored value is the encoded hash, which already contains the salt and the parameters, so `verifyPassword` needs nothing else. Plaintext passwords are never stored.

**Login.** `authenticateUser` looks the user up by email and verifies the password. On success it creates a session: a random token of 32 bytes written as 64 hex characters, stored in `sessions` with an expiration date 7 days ahead. An unknown email and a wrong password both return the same `InvalidCredentials` error, so a caller cannot tell which one failed.

**Logout.** `invalidateSession` deletes the session row.

**Expiration.** A session is valid until its `expires_at`. Expired sessions are rejected, and removed, the next time their token is used (`requireAuth` in the `api` module). They are not cleaned up in the background.

**Errors.** `AuthError` carries a type (`EmailAlreadyExists`, `DatabaseError`, `InvalidCredentials`) so the routes can map each case to an HTTP status without reading database messages.

## Notes

* **Token generation** uses `std::mt19937_64` seeded from `std::random_device` and the clock. That generator is not a cryptographically secure one. It is listed for the security hardening review.
* Sessions created before expiration was added have no valid expiration date and count as expired.
* Email verification is not implemented.