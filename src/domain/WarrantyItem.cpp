#include "storeforge/domain/WarrantyItem.hpp"
#include "storeforge/core/Exceptions.hpp"
#include <sstream>
#include <iomanip>

namespace storeforge::domain {

WarrantyItem::WarrantyItem(core::ItemId id,
                           core::SKU sku,
                           std::string name,
                           double basePrice,
                           int stockQuantity,
                           int reorderThreshold,
                           core::ItemCategory category,
                           core::VendorId vendorId,
                           std::string serialNumber,
                           int warrantyPeriodMonths,
                           double warrantyFee)
    : Item(id, std::move(sku), std::move(name), basePrice, stockQuantity, reorderThreshold, category, vendorId),
      serialNumber_(std::move(serialNumber)),
      warrantyPeriodMonths_(warrantyPeriodMonths),
      warrantyFee_(warrantyFee) {
    if (serialNumber_.empty()) {
        throw core::DomainValidationException("Serial number cannot be empty for a warranty-backed item");
    }
    if (warrantyPeriodMonths_ < 0) {
        throw core::DomainValidationException("Warranty period cannot be negative");
    }
}

void WarrantyItem::setSerialNumber(std::string serialNumber) {
    if (serialNumber.empty()) {
        throw core::DomainValidationException("Serial number cannot be empty");
    }
    serialNumber_ = std::move(serialNumber);
    updatedAt_ = core::DateTime::now();
}

void WarrantyItem::setWarrantyPeriodMonths(int months) {
    if (months < 0) {
        throw core::DomainValidationException("Warranty period cannot be negative");
    }
    warrantyPeriodMonths_ = months;
    updatedAt_ = core::DateTime::now();
}

void WarrantyItem::setWarrantyFee(double fee) {
    if (fee < 0.0) {
        throw core::DomainValidationException("Warranty fee cannot be negative");
    }
    warrantyFee_ = fee;
    updatedAt_ = core::DateTime::now();
}

double WarrantyItem::calculateTax(double standardTaxRate) const {
    // Electronics / warranty items tax applied on base price + warranty fee
    double taxable = basePrice_ + warrantyFee_;
    double rate = (standardTaxRate > 0.0) ? standardTaxRate : 0.12; // 12% default for electronics
    return core::Money::round(taxable * rate);
}

double WarrantyItem::calculateDiscount(int quantity) const {
    if (quantity >= 3) {
        return core::Money::round(basePrice_ * 0.05); // 5% bundle discount
    }
    return 0.0;
}

bool WarrantyItem::isSellable() const {
    return (stockQuantity_ > 0) && !serialNumber_.empty();
}

std::string WarrantyItem::serializeJson() const {
    std::ostringstream ss;
    ss << "{"
       << "\"id\":" << id_ << ","
       << "\"type\":\"WARRANTY\","
       << "\"sku\":\"" << sku_.value() << "\","
       << "\"name\":\"" << name_ << "\","
       << "\"base_price\":" << std::fixed << std::setprecision(2) << basePrice_ << ","
       << "\"stock_quantity\":" << stockQuantity_ << ","
       << "\"reorder_threshold\":" << reorderThreshold_ << ","
       << "\"category\":\"" << core::categoryToString(category_) << "\","
       << "\"vendor_id\":" << vendorId_ << ","
       << "\"serial_number\":\"" << serialNumber_ << "\","
       << "\"warranty_period_months\":" << warrantyPeriodMonths_ << ","
       << "\"warranty_fee\":" << std::fixed << std::setprecision(2) << warrantyFee_
       << "}";
    return ss.str();
}

std::unique_ptr<Item> WarrantyItem::clone() const {
    return std::make_unique<WarrantyItem>(*this);
}

} // namespace storeforge::domain
