#include "storeforge/service/InventoryService.hpp"
#include "storeforge/core/Exceptions.hpp"

namespace storeforge::service {

InventoryService::InventoryService(repo::IItemRepository& itemRepo,
                                   repo::IVendorRepository& vendorRepo,
                                   repo::IAuditLogRepository& auditRepo)
    : itemRepo_(itemRepo),
      vendorRepo_(vendorRepo),
      auditRepo_(auditRepo) {}

core::ItemId InventoryService::registerItem(const domain::Item& item) {
    std::lock_guard lock(mutex_);
    if (item.getVendorId() > 0) {
        auto vendorOpt = vendorRepo_.findById(item.getVendorId());
        if (!vendorOpt.has_value()) {
            throw core::EntityNotFoundException("Vendor", item.getVendorId());
        }
    }

    core::ItemId id = itemRepo_.save(item);

    // Initial audit log
    domain::InventoryAuditLog auditLog(
        0,
        id,
        item.getSku().value(),
        core::AuditAction::RESTOCK,
        item.getStockQuantity(),
        0,
        item.getStockQuantity(),
        "Initial Inventory Ingestion",
        "SYSTEM",
        core::DateTime::now()
    );
    auditRepo_.save(auditLog);

    return id;
}

void InventoryService::updateItem(const domain::Item& item) {
    std::lock_guard lock(mutex_);
    auto existingOpt = itemRepo_.findById(item.getId());
    if (!existingOpt.has_value()) {
        throw core::EntityNotFoundException("Item", item.getId());
    }
    itemRepo_.update(item);
}

bool InventoryService::removeItem(core::ItemId id) {
    std::lock_guard lock(mutex_);
    return itemRepo_.remove(id);
}

void InventoryService::restock(core::ItemId itemId, int quantity, const std::string& reason, const std::string& operatorId) {
    std::lock_guard lock(mutex_);
    if (quantity <= 0) {
        throw core::DomainValidationException("Restock quantity must be greater than zero");
    }

    auto itemOpt = itemRepo_.findById(itemId);
    if (!itemOpt.has_value()) {
        throw core::EntityNotFoundException("Item", itemId);
    }

    auto& item = *itemOpt.value();
    int prevStock = item.getStockQuantity();
    int newStock = prevStock + quantity;

    itemRepo_.updateStock(itemId, newStock);

    domain::InventoryAuditLog log(
        0,
        itemId,
        item.getSku().value(),
        core::AuditAction::RESTOCK,
        quantity,
        prevStock,
        newStock,
        reason,
        operatorId,
        core::DateTime::now()
    );
    auditRepo_.save(log);
}

void InventoryService::writeOff(core::ItemId itemId, int quantity, const std::string& reason, const std::string& operatorId) {
    std::lock_guard lock(mutex_);
    if (quantity <= 0) {
        throw core::DomainValidationException("Write-off quantity must be greater than zero");
    }

    auto itemOpt = itemRepo_.findById(itemId);
    if (!itemOpt.has_value()) {
        throw core::EntityNotFoundException("Item", itemId);
    }

    auto& item = *itemOpt.value();
    int prevStock = item.getStockQuantity();
    if (prevStock < quantity) {
        throw core::InsufficientStockException(item.getSku().value(), quantity, prevStock);
    }

    int newStock = prevStock - quantity;
    itemRepo_.updateStock(itemId, newStock);

    domain::InventoryAuditLog log(
        0,
        itemId,
        item.getSku().value(),
        core::AuditAction::WRITE_OFF,
        -quantity,
        prevStock,
        newStock,
        reason,
        operatorId,
        core::DateTime::now()
    );
    auditRepo_.save(log);
}

void InventoryService::adjustStock(core::ItemId itemId, int targetQuantity, const std::string& reason, const std::string& operatorId) {
    std::lock_guard lock(mutex_);
    if (targetQuantity < 0) {
        throw core::DomainValidationException("Target stock quantity cannot be negative");
    }

    auto itemOpt = itemRepo_.findById(itemId);
    if (!itemOpt.has_value()) {
        throw core::EntityNotFoundException("Item", itemId);
    }

    auto& item = *itemOpt.value();
    int prevStock = item.getStockQuantity();
    int delta = targetQuantity - prevStock;

    itemRepo_.updateStock(itemId, targetQuantity);

    domain::InventoryAuditLog log(
        0,
        itemId,
        item.getSku().value(),
        core::AuditAction::MANUAL_ADJUSTMENT,
        delta,
        prevStock,
        targetQuantity,
        reason,
        operatorId,
        core::DateTime::now()
    );
    auditRepo_.save(log);
}

std::vector<std::unique_ptr<domain::Item>> InventoryService::getLowStockItems() const {
    std::lock_guard lock(mutex_);
    return itemRepo_.findLowStock();
}

std::vector<std::unique_ptr<domain::Item>> InventoryService::getAllItems() const {
    std::lock_guard lock(mutex_);
    return itemRepo_.findAll();
}

std::optional<std::unique_ptr<domain::Item>> InventoryService::getItemById(core::ItemId id) const {
    std::lock_guard lock(mutex_);
    return itemRepo_.findById(id);
}

std::optional<std::unique_ptr<domain::Item>> InventoryService::getItemBySku(const core::SKU& sku) const {
    std::lock_guard lock(mutex_);
    return itemRepo_.findBySku(sku);
}

std::vector<domain::InventoryAuditLog> InventoryService::getItemAuditTrail(core::ItemId itemId) const {
    std::lock_guard lock(mutex_);
    return auditRepo_.findByItemId(itemId);
}

std::vector<domain::InventoryAuditLog> InventoryService::getRecentAuditLogs(size_t limit) const {
    std::lock_guard lock(mutex_);
    return auditRepo_.findAll(limit);
}

} // namespace storeforge::service
