#pragma once

#include <shared_mutex>
#include <mutex>
#include <unordered_map>
#include <map>
#include <atomic>
#include "storeforge/repo/IItemRepository.hpp"
#include "storeforge/repo/IVendorRepository.hpp"
#include "storeforge/repo/IReceiptRepository.hpp"
#include "storeforge/repo/IAuditLogRepository.hpp"
#include "storeforge/repo/ITransactionContext.hpp"

namespace storeforge::repo {

class InMemoryItemRepository : public IItemRepository {
public:
    InMemoryItemRepository() = default;

    std::optional<std::unique_ptr<domain::Item>> findById(core::ItemId id) override;
    std::optional<std::unique_ptr<domain::Item>> findBySku(const core::SKU& sku) override;
    std::vector<std::unique_ptr<domain::Item>> findAll() override;
    std::vector<std::unique_ptr<domain::Item>> findByCategory(core::ItemCategory category) override;
    std::vector<std::unique_ptr<domain::Item>> findLowStock() override;

    core::ItemId save(const domain::Item& item) override;
    void update(const domain::Item& item) override;
    bool remove(core::ItemId id) override;
    void updateStock(core::ItemId id, int newStock) override;

    // Snapshot support for rollback emulation
    void createSnapshot();
    void rollbackSnapshot();
    void commitSnapshot();

private:
    mutable std::shared_mutex mutex_;
    std::atomic<core::ItemId> nextId_{1};
    std::unordered_map<core::ItemId, std::unique_ptr<domain::Item>> items_;
    std::unordered_map<std::string, core::ItemId> skuMap_;

    // Transaction snapshot cache
    bool hasSnapshot_{false};
    std::unordered_map<core::ItemId, std::unique_ptr<domain::Item>> snapshotItems_;
    std::unordered_map<std::string, core::ItemId> snapshotSkuMap_;
};

class InMemoryVendorRepository : public IVendorRepository {
public:
    InMemoryVendorRepository() = default;

    std::optional<domain::Vendor> findById(core::VendorId id) override;
    std::vector<domain::Vendor> findAll() override;
    std::vector<domain::Vendor> findActive() override;

    core::VendorId save(const domain::Vendor& vendor) override;
    void update(const domain::Vendor& vendor) override;
    bool remove(core::VendorId id) override;

private:
    mutable std::shared_mutex mutex_;
    std::atomic<core::VendorId> nextId_{1};
    std::unordered_map<core::VendorId, domain::Vendor> vendors_;
};

class InMemoryReceiptRepository : public IReceiptRepository {
public:
    InMemoryReceiptRepository() = default;

    std::optional<domain::CustomerReceipt> findById(core::ReceiptId id) override;
    std::optional<domain::CustomerReceipt> findByReceiptNumber(const std::string& receiptNumber) override;
    std::vector<domain::CustomerReceipt> findAll() override;
    std::vector<domain::CustomerReceipt> findByDateRange(const core::DateTime& start, const core::DateTime& end) override;

    core::ReceiptId save(const domain::CustomerReceipt& receipt) override;

private:
    mutable std::shared_mutex mutex_;
    std::atomic<core::ReceiptId> nextId_{1};
    std::unordered_map<core::ReceiptId, domain::CustomerReceipt> receipts_;
    std::unordered_map<std::string, core::ReceiptId> numberMap_;
};

class InMemoryAuditLogRepository : public IAuditLogRepository {
public:
    InMemoryAuditLogRepository() = default;

    core::AuditLogId save(const domain::InventoryAuditLog& log) override;
    std::vector<domain::InventoryAuditLog> findByItemId(core::ItemId itemId) override;
    std::vector<domain::InventoryAuditLog> findAll(size_t limit = 100) override;

private:
    mutable std::shared_mutex mutex_;
    std::atomic<core::AuditLogId> nextId_{1};
    std::vector<domain::InventoryAuditLog> logs_;
};

class InMemoryTransactionContext : public ITransactionContext {
public:
    explicit InMemoryTransactionContext(InMemoryItemRepository& itemRepo);
    ~InMemoryTransactionContext() override;

    void commit() override;
    void rollback() override;
    [[nodiscard]] bool isActive() const noexcept override { return active_; }

private:
    InMemoryItemRepository& itemRepo_;
    bool active_{true};
};

} // namespace storeforge::repo
