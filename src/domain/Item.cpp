#include "storeforge/domain/Item.hpp"
#include "storeforge/core/Exceptions.hpp"
#include <algorithm>

namespace storeforge::domain {

Item::Item(core::ItemId id,
           core::SKU sku,
           std::string name,
           double basePrice,
           int stockQuantity,
           int reorderThreshold,
           core::ItemCategory category,
           core::VendorId vendorId)
    : id_(id),
      sku_(std::move(sku)),
      name_(std::move(name)),
      basePrice_(basePrice),
      stockQuantity_(stockQuantity),
      reorderThreshold_(reorderThreshold),
      category_(category),
      vendorId_(vendorId),
      createdAt_(core::DateTime::now()),
      updatedAt_(core::DateTime::now()) {
    if (sku_.empty()) {
        throw core::DomainValidationException("Item SKU cannot be empty");
    }
    if (name_.empty()) {
        throw core::DomainValidationException("Item name cannot be empty");
    }
    if (basePrice_ < 0.0) {
        throw core::DomainValidationException("Base price cannot be negative");
    }
    if (stockQuantity_ < 0) {
        throw core::DomainValidationException("Stock quantity cannot be negative");
    }
}

double Item::getEffectivePrice(int quantity) const {
    double discount = calculateDiscount(quantity);
    double pricePerUnit = std::max(0.0, basePrice_ - discount);
    return core::Money::round(pricePerUnit);
}

void Item::setSku(const core::SKU& sku) {
    if (sku.empty()) {
        throw core::DomainValidationException("Item SKU cannot be empty");
    }
    sku_ = sku;
    updatedAt_ = core::DateTime::now();
}

void Item::setName(std::string name) {
    if (name.empty()) {
        throw core::DomainValidationException("Item name cannot be empty");
    }
    name_ = std::move(name);
    updatedAt_ = core::DateTime::now();
}

void Item::setBasePrice(double price) {
    if (price < 0.0) {
        throw core::DomainValidationException("Base price cannot be negative");
    }
    basePrice_ = price;
    updatedAt_ = core::DateTime::now();
}

void Item::setStockQuantity(int qty) {
    if (qty < 0) {
        throw core::DomainValidationException("Stock quantity cannot be negative");
    }
    stockQuantity_ = qty;
    updatedAt_ = core::DateTime::now();
}

void Item::setReorderThreshold(int threshold) {
    if (threshold < 0) {
        throw core::DomainValidationException("Reorder threshold cannot be negative");
    }
    reorderThreshold_ = threshold;
    updatedAt_ = core::DateTime::now();
}

void Item::deductStock(int quantity) {
    if (quantity <= 0) {
        throw core::DomainValidationException("Deduct quantity must be positive");
    }
    if (stockQuantity_ < quantity) {
        throw core::InsufficientStockException(sku_.value(), quantity, stockQuantity_);
    }
    stockQuantity_ -= quantity;
    updatedAt_ = core::DateTime::now();
}

void Item::addStock(int quantity) {
    if (quantity <= 0) {
        throw core::DomainValidationException("Restock quantity must be positive");
    }
    stockQuantity_ += quantity;
    updatedAt_ = core::DateTime::now();
}

} // namespace storeforge::domain
