#pragma once

#include <memory>
#include "storeforge/db/ConnectionPool.hpp"
#include "storeforge/repo/IItemRepository.hpp"
#include "storeforge/repo/IVendorRepository.hpp"
#include "storeforge/repo/IReceiptRepository.hpp"
#include "storeforge/repo/IAuditLogRepository.hpp"

namespace storeforge::repo {

class PostgresItemRepository : public IItemRepository {
public:
    explicit PostgresItemRepository(std::shared_ptr<db::ConnectionPool> pool);

    std::optional<std::unique_ptr<domain::Item>> findById(core::ItemId id) override;
    std::optional<std::unique_ptr<domain::Item>> findBySku(const core::SKU& sku) override;
    std::vector<std::unique_ptr<domain::Item>> findAll() override;
    std::vector<std::unique_ptr<domain::Item>> findByCategory(core::ItemCategory category) override;
    std::vector<std::unique_ptr<domain::Item>> findLowStock() override;

    core::ItemId save(const domain::Item& item) override;
    void update(const domain::Item& item) override;
    bool remove(core::ItemId id) override;
    void updateStock(core::ItemId id, int newStock) override;

private:
    std::shared_ptr<db::ConnectionPool> pool_;
#if defined(STOREFORGE_HAS_LIBPQXX)
    std::unique_ptr<domain::Item> mapRowToItem(const pqxx::row& row);
#endif
};

class PostgresVendorRepository : public IVendorRepository {
public:
    explicit PostgresVendorRepository(std::shared_ptr<db::ConnectionPool> pool);

    std::optional<domain::Vendor> findById(core::VendorId id) override;
    std::vector<domain::Vendor> findAll() override;
    std::vector<domain::Vendor> findActive() override;

    core::VendorId save(const domain::Vendor& vendor) override;
    void update(const domain::Vendor& vendor) override;
    bool remove(core::VendorId id) override;

private:
    std::shared_ptr<db::ConnectionPool> pool_;
#if defined(STOREFORGE_HAS_LIBPQXX)
    domain::Vendor mapRowToVendor(const pqxx::row& row);
#endif
};

class PostgresReceiptRepository : public IReceiptRepository {
public:
    explicit PostgresReceiptRepository(std::shared_ptr<db::ConnectionPool> pool);

    std::optional<domain::CustomerReceipt> findById(core::ReceiptId id) override;
    std::optional<domain::CustomerReceipt> findByReceiptNumber(const std::string& receiptNumber) override;
    std::vector<domain::CustomerReceipt> findAll() override;
    std::vector<domain::CustomerReceipt> findByDateRange(const core::DateTime& start, const core::DateTime& end) override;

    core::ReceiptId save(const domain::CustomerReceipt& receipt) override;

private:
    std::shared_ptr<db::ConnectionPool> pool_;
#if defined(STOREFORGE_HAS_LIBPQXX)
    domain::CustomerReceipt mapRowToReceipt(pqxx::work& tx, const pqxx::row& row);
#endif
};

class PostgresAuditLogRepository : public IAuditLogRepository {
public:
    explicit PostgresAuditLogRepository(std::shared_ptr<db::ConnectionPool> pool);

    core::AuditLogId save(const domain::InventoryAuditLog& log) override;
    std::vector<domain::InventoryAuditLog> findByItemId(core::ItemId itemId) override;
    std::vector<domain::InventoryAuditLog> findAll(size_t limit = 100) override;

private:
    std::shared_ptr<db::ConnectionPool> pool_;
#if defined(STOREFORGE_HAS_LIBPQXX)
    domain::InventoryAuditLog mapRowToAuditLog(const pqxx::row& row);
#endif
};

} // namespace storeforge::repo
