#include "storeforge/db/PostgresTransaction.hpp"

namespace storeforge::db {

#if defined(STOREFORGE_HAS_LIBPQXX)
PostgresTransaction::PostgresTransaction(ConnectionPool::ConnectionPtr conn)
    : conn_(std::move(conn)) {
    if (conn_) {
        work_ = std::make_unique<pqxx::work>(*conn_);
        isActive_ = true;
    }
}

PostgresTransaction::~PostgresTransaction() {
    if (isActive_ && work_) {
        try {
            work_->abort();
        } catch (...) {
            // Suppress exception in destructor
        }
        isActive_ = false;
    }
}

void PostgresTransaction::commit() {
    if (!isActive_ || !work_) {
        throw core::TransactionException("Transaction is not active");
    }
    try {
        work_->commit();
        isActive_ = false;
    } catch (const std::exception& e) {
        isActive_ = false;
        throw core::TransactionException(std::string("Postgres commit failed: ") + e.what());
    }
}

void PostgresTransaction::rollback() {
    if (!isActive_ || !work_) return;
    try {
        work_->abort();
    } catch (...) {}
    isActive_ = false;
}

pqxx::work& PostgresTransaction::getWork() {
    if (!isActive_ || !work_) {
        throw core::TransactionException("No active transaction work available");
    }
    return *work_;
}

#else
PostgresTransaction::PostgresTransaction() = default;
PostgresTransaction::~PostgresTransaction() = default;
#endif

} // namespace storeforge::db
