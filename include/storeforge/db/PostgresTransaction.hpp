#pragma once

#include <memory>
#include "storeforge/repo/ITransactionContext.hpp"
#include "storeforge/db/ConnectionPool.hpp"

namespace storeforge::db {

class PostgresTransaction : public repo::ITransactionContext {
public:
#if defined(STOREFORGE_HAS_LIBPQXX)
    explicit PostgresTransaction(ConnectionPool::ConnectionPtr conn);
    ~PostgresTransaction() override;

    void commit() override;
    void rollback() override;
    [[nodiscard]] bool isActive() const noexcept override { return isActive_; }

    [[nodiscard]] pqxx::work& getWork();

private:
    ConnectionPool::ConnectionPtr conn_;
    std::unique_ptr<pqxx::work> work_;
    bool isActive_{false};
#else
    explicit PostgresTransaction();
    ~PostgresTransaction() override;
    void commit() override {}
    void rollback() override {}
    [[nodiscard]] bool isActive() const noexcept override { return false; }
#endif
};

} // namespace storeforge::db
