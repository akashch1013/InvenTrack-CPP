#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include "storeforge/repo/IItemRepository.hpp"
#include "storeforge/repo/IVendorRepository.hpp"
#include "storeforge/repo/IAuditLogRepository.hpp"
#include "storeforge/domain/Item.hpp"

namespace storeforge::service {

class InventoryService {
public:
    InventoryService(repo::IItemRepository& itemRepo,
                     repo::IVendorRepository& vendorRepo,
                     repo::IAuditLogRepository& auditRepo);

    core::ItemId registerItem(const domain::Item& item);
    void updateItem(const domain::Item& item);
    bool removeItem(core::ItemId id);

    void restock(core::ItemId itemId, int quantity, const std::string& reason = "Stock Replenishment", const std::string& operatorId = "SYSTEM");
    void writeOff(core::ItemId itemId, int quantity, const std::string& reason = "Damaged / Expired", const std::string& operatorId = "SYSTEM");
    void adjustStock(core::ItemId itemId, int targetQuantity, const std::string& reason = "Audit Adjustment", const std::string& operatorId = "SYSTEM");

    [[nodiscard]] std::vector<std::unique_ptr<domain::Item>> getLowStockItems() const;
    [[nodiscard]] std::vector<std::unique_ptr<domain::Item>> getAllItems() const;
    [[nodiscard]] std::optional<std::unique_ptr<domain::Item>> getItemById(core::ItemId id) const;
    [[nodiscard]] std::optional<std::unique_ptr<domain::Item>> getItemBySku(const core::SKU& sku) const;

    [[nodiscard]] std::vector<domain::InventoryAuditLog> getItemAuditTrail(core::ItemId itemId) const;
    [[nodiscard]] std::vector<domain::InventoryAuditLog> getRecentAuditLogs(size_t limit = 50) const;

private:
    repo::IItemRepository& itemRepo_;
    repo::IVendorRepository& vendorRepo_;
    repo::IAuditLogRepository& auditRepo_;
    mutable std::mutex mutex_;
};

} // namespace storeforge::service
