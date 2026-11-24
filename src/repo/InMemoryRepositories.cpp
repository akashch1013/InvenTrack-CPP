#include "storeforge/repo/InMemoryRepositories.hpp"
#include "storeforge/core/Exceptions.hpp"
#include <algorithm>

namespace storeforge::repo {

// ---------------- InMemoryItemRepository ----------------

std::optional<std::unique_ptr<domain::Item>> InMemoryItemRepository::findById(core::ItemId id) {
    std::shared_lock lock(mutex_);
    auto it = items_.find(id);
    if (it == items_.end()) {
        return std::nullopt;
    }
    return it->second->clone();
}

std::optional<std::unique_ptr<domain::Item>> InMemoryItemRepository::findBySku(const core::SKU& sku) {
    std::shared_lock lock(mutex_);
    auto it = skuMap_.find(sku.value());
    if (it == skuMap_.end()) {
        return std::nullopt;
    }
    auto itemIt = items_.find(it->second);
    if (itemIt == items_.end()) {
        return std::nullopt;
    }
    return itemIt->second->clone();
}

std::vector<std::unique_ptr<domain::Item>> InMemoryItemRepository::findAll() {
    std::shared_lock lock(mutex_);
    std::vector<std::unique_ptr<domain::Item>> result;
    result.reserve(items_.size());
    for (const auto& [_, item] : items_) {
        result.push_back(item->clone());
    }
    return result;
}

std::vector<std::unique_ptr<domain::Item>> InMemoryItemRepository::findByCategory(core::ItemCategory category) {
    std::shared_lock lock(mutex_);
    std::vector<std::unique_ptr<domain::Item>> result;
    for (const auto& [_, item] : items_) {
        if (item->getCategory() == category) {
            result.push_back(item->clone());
        }
    }
    return result;
}

std::vector<std::unique_ptr<domain::Item>> InMemoryItemRepository::findLowStock() {
    std::shared_lock lock(mutex_);
    std::vector<std::unique_ptr<domain::Item>> result;
    for (const auto& [_, item] : items_) {
        if (item->isLowStock()) {
            result.push_back(item->clone());
        }
    }
    return result;
}

core::ItemId InMemoryItemRepository::save(const domain::Item& item) {
    std::unique_lock lock(mutex_);
    if (skuMap_.find(item.getSku().value()) != skuMap_.end()) {
        throw core::DuplicateEntityException("Item", item.getSku().value());
    }

    core::ItemId id = item.getId() > 0 ? item.getId() : nextId_++;
    auto cloned = item.clone();
    cloned->setId(id);
    skuMap_[item.getSku().value()] = id;
    items_[id] = std::move(cloned);
    return id;
}

void InMemoryItemRepository::update(const domain::Item& item) {
    std::unique_lock lock(mutex_);
    auto it = items_.find(item.getId());
    if (it == items_.end()) {
        throw core::EntityNotFoundException("Item", item.getId());
    }

    auto skuIt = skuMap_.find(item.getSku().value());
    if (skuIt != skuMap_.end() && skuIt->second != item.getId()) {
        throw core::DuplicateEntityException("Item SKU", item.getSku().value());
    }

    skuMap_.erase(it->second->getSku().value());
    skuMap_[item.getSku().value()] = item.getId();
    items_[item.getId()] = item.clone();
}

bool InMemoryItemRepository::remove(core::ItemId id) {
    std::unique_lock lock(mutex_);
    auto it = items_.find(id);
    if (it == items_.end()) {
        return false;
    }
    skuMap_.erase(it->second->getSku().value());
    items_.erase(it);
    return true;
}

void InMemoryItemRepository::updateStock(core::ItemId id, int newStock) {
    std::unique_lock lock(mutex_);
    auto it = items_.find(id);
    if (it == items_.end()) {
        throw core::EntityNotFoundException("Item", id);
    }
    it->second->setStockQuantity(newStock);
}

void InMemoryItemRepository::createSnapshot() {
    std::unique_lock lock(mutex_);
    snapshotItems_.clear();
    snapshotSkuMap_ = skuMap_;
    for (const auto& [id, item] : items_) {
        snapshotItems_[id] = item->clone();
    }
    hasSnapshot_ = true;
}

void InMemoryItemRepository::rollbackSnapshot() {
    std::unique_lock lock(mutex_);
    if (!hasSnapshot_) return;
    items_.clear();
    skuMap_ = snapshotSkuMap_;
    for (auto& [id, item] : snapshotItems_) {
        items_[id] = std::move(item);
    }
    snapshotItems_.clear();
    snapshotSkuMap_.clear();
    hasSnapshot_ = false;
}

void InMemoryItemRepository::commitSnapshot() {
    std::unique_lock lock(mutex_);
    snapshotItems_.clear();
    snapshotSkuMap_.clear();
    hasSnapshot_ = false;
}

// ---------------- InMemoryVendorRepository ----------------

std::optional<domain::Vendor> InMemoryVendorRepository::findById(core::VendorId id) {
    std::shared_lock lock(mutex_);
    auto it = vendors_.find(id);
    if (it == vendors_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::vector<domain::Vendor> InMemoryVendorRepository::findAll() {
    std::shared_lock lock(mutex_);
    std::vector<domain::Vendor> result;
    result.reserve(vendors_.size());
    for (const auto& [_, v] : vendors_) {
        result.push_back(v);
    }
    return result;
}

std::vector<domain::Vendor> InMemoryVendorRepository::findActive() {
    std::shared_lock lock(mutex_);
    std::vector<domain::Vendor> result;
    for (const auto& [_, v] : vendors_) {
        if (v.isActive()) {
            result.push_back(v);
        }
    }
    return result;
}

core::VendorId InMemoryVendorRepository::save(const domain::Vendor& vendor) {
    std::unique_lock lock(mutex_);
    core::VendorId id = vendor.getId() > 0 ? vendor.getId() : nextId_++;
    domain::Vendor v = vendor;
    v.setId(id);
    vendors_[id] = v;
    return id;
}

void InMemoryVendorRepository::update(const domain::Vendor& vendor) {
    std::unique_lock lock(mutex_);
    auto it = vendors_.find(vendor.getId());
    if (it == vendors_.end()) {
        throw core::EntityNotFoundException("Vendor", vendor.getId());
    }
    vendors_[vendor.getId()] = vendor;
}

bool InMemoryVendorRepository::remove(core::VendorId id) {
    std::unique_lock lock(mutex_);
    return vendors_.erase(id) > 0;
}

// ---------------- InMemoryReceiptRepository ----------------

std::optional<domain::CustomerReceipt> InMemoryReceiptRepository::findById(core::ReceiptId id) {
    std::shared_lock lock(mutex_);
    auto it = receipts_.find(id);
    if (it == receipts_.end()) {
        return std::nullopt;
    }
    return it->second;
}

std::optional<domain::CustomerReceipt> InMemoryReceiptRepository::findByReceiptNumber(const std::string& receiptNumber) {
    std::shared_lock lock(mutex_);
    auto it = numberMap_.find(receiptNumber);
    if (it == numberMap_.end()) {
        return std::nullopt;
    }
    auto recIt = receipts_.find(it->second);
    if (recIt == receipts_.end()) {
        return std::nullopt;
    }
    return recIt->second;
}

std::vector<domain::CustomerReceipt> InMemoryReceiptRepository::findAll() {
    std::shared_lock lock(mutex_);
    std::vector<domain::CustomerReceipt> result;
    result.reserve(receipts_.size());
    for (const auto& [_, r] : receipts_) {
        result.push_back(r);
    }
    return result;
}

std::vector<domain::CustomerReceipt> InMemoryReceiptRepository::findByDateRange(const core::DateTime& start, const core::DateTime& end) {
    std::shared_lock lock(mutex_);
    std::vector<domain::CustomerReceipt> result;
    for (const auto& [_, r] : receipts_) {
        if (r.getCreatedAt() >= start && r.getCreatedAt() <= end) {
            result.push_back(r);
        }
    }
    return result;
}

core::ReceiptId InMemoryReceiptRepository::save(const domain::CustomerReceipt& receipt) {
    std::unique_lock lock(mutex_);
    core::ReceiptId id = receipt.getId() > 0 ? receipt.getId() : nextId_++;
    domain::CustomerReceipt r = receipt;
    r.setId(id);
    numberMap_[r.getReceiptNumber()] = id;
    receipts_[id] = std::move(r);
    return id;
}

// ---------------- InMemoryAuditLogRepository ----------------

core::AuditLogId InMemoryAuditLogRepository::save(const domain::InventoryAuditLog& log) {
    std::unique_lock lock(mutex_);
    core::AuditLogId id = log.getId() > 0 ? log.getId() : nextId_++;
    domain::InventoryAuditLog l = log;
    l.setId(id);
    logs_.push_back(std::move(l));
    return id;
}

std::vector<domain::InventoryAuditLog> InMemoryAuditLogRepository::findByItemId(core::ItemId itemId) {
    std::shared_lock lock(mutex_);
    std::vector<domain::InventoryAuditLog> result;
    for (const auto& log : logs_) {
        if (log.getItemId() == itemId) {
            result.push_back(log);
        }
    }
    return result;
}

std::vector<domain::InventoryAuditLog> InMemoryAuditLogRepository::findAll(size_t limit) {
    std::shared_lock lock(mutex_);
    std::vector<domain::InventoryAuditLog> result;
    size_t count = std::min(limit, logs_.size());
    result.reserve(count);
    // Return latest first
    for (auto it = logs_.rbegin(); it != logs_.rend() && result.size() < limit; ++it) {
        result.push_back(*it);
    }
    return result;
}

// ---------------- InMemoryTransactionContext ----------------

InMemoryTransactionContext::InMemoryTransactionContext(InMemoryItemRepository& itemRepo)
    : itemRepo_(itemRepo), active_(true) {
    itemRepo_.createSnapshot();
}

InMemoryTransactionContext::~InMemoryTransactionContext() {
    if (active_) {
        rollback();
    }
}

void InMemoryTransactionContext::commit() {
    if (!active_) return;
    itemRepo_.commitSnapshot();
    active_ = false;
}

void InMemoryTransactionContext::rollback() {
    if (!active_) return;
    itemRepo_.rollbackSnapshot();
    active_ = false;
}

} // namespace storeforge::repo
