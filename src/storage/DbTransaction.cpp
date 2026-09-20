#include "storage/DbTransaction.h"

namespace nubilo {

DbTransaction::DbTransaction(SqliteDb& db) : db_(&db) {}

std::expected<DbTransaction, DbError> DbTransaction::begin(SqliteDb& db) {
    auto result = db.beginTx();
    if (!result)
        return std::unexpected(result.error());

    return DbTransaction(db);
}

DbTransaction::~DbTransaction() {
    if (active_)
        db_->rollbackTx();
}

DbTransaction::DbTransaction(DbTransaction&& other) noexcept : db_(other.db_) {
    other.active_ = false;
}

DbTransaction& DbTransaction::operator=(DbTransaction&& other) noexcept {
    if (this != &other) {
        if (active_)
            db_->rollbackTx();

        db_ = other.db_;
        active_ = other.active_;
        other.active_ = false;
    }
    return *this;
}

std::expected<void, DbError> DbTransaction::commit() {
    active_ = false;
    return db_->commitTx();
}

}