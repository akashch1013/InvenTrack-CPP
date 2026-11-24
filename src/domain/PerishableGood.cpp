#include "storeforge/domain/PerishableGood.hpp"
#include <sstream>
#include <iomanip>

namespace storeforge::domain {

PerishableGood::PerishableGood(core::ItemId id,
                               core::SKU sku,
                               std::string name,
                               double basePrice,
                               int stockQuantity,
                               int reorderThreshold,
                               core::ItemCategory category,
                               core::VendorId vendorId,
                               core::DateTime expirationDate,
                               double reducedTaxRate)
    : Item(id, std::move(sku), std::move(name), basePrice, stockQuantity, reorderThreshold, category, vendorId),
      expirationDate_(expirationDate),
      reducedTaxRate_(reducedTaxRate) {}

int PerishableGood::getDaysUntilExpiration() const {
    core::DateTime today = core::DateTime::now();
    return today.daysUntil(expirationDate_);
}

bool PerishableGood::isExpired() const {
    return expirationDate_.isPast(core::DateTime::now());
}

double PerishableGood::getMarkdownPercentage() const {
    if (isExpired()) return 0.0; // Expired, not for sale
    int days = getDaysUntilExpiration();
    if (days <= 2) return 0.50; // 50% discount for immediate clearance
    if (days <= 5) return 0.25; // 25% discount
    if (days <= 7) return 0.10; // 10% discount
    return 0.0;
}

double PerishableGood::calculateTax([[maybe_unused]] double standardTaxRate) const {
    return core::Money::round(basePrice_ * reducedTaxRate_);
}

double PerishableGood::calculateDiscount([[maybe_unused]] int quantity) const {
    double markdownPct = getMarkdownPercentage();
    return core::Money::round(basePrice_ * markdownPct);
}

bool PerishableGood::isSellable() const {
    return (stockQuantity_ > 0) && !isExpired();
}

std::string PerishableGood::serializeJson() const {
    std::ostringstream ss;
    ss << "{"
       << "\"id\":" << id_ << ","
       << "\"type\":\"PERISHABLE\","
       << "\"sku\":\"" << sku_.value() << "\","
       << "\"name\":\"" << name_ << "\","
       << "\"base_price\":" << std::fixed << std::setprecision(2) << basePrice_ << ","
       << "\"stock_quantity\":" << stockQuantity_ << ","
       << "\"reorder_threshold\":" << reorderThreshold_ << ","
       << "\"category\":\"" << core::categoryToString(category_) << "\","
       << "\"vendor_id\":" << vendorId_ << ","
       << "\"expiration_date\":\"" << expirationDate_.toDateString() << "\","
       << "\"days_until_expiration\":" << getDaysUntilExpiration() << ","
       << "\"is_expired\":" << (isExpired() ? "true" : "false")
       << "}";
    return ss.str();
}

std::unique_ptr<Item> PerishableGood::clone() const {
    return std::make_unique<PerishableGood>(*this);
}

} // namespace storeforge::domain
