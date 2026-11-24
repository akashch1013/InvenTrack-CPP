#include "storeforge/domain/StandardCommodity.hpp"
#include <sstream>
#include <iomanip>

namespace storeforge::domain {

StandardCommodity::StandardCommodity(core::ItemId id,
                                     core::SKU sku,
                                     std::string name,
                                     double basePrice,
                                     int stockQuantity,
                                     int reorderThreshold,
                                     core::ItemCategory category,
                                     core::VendorId vendorId,
                                     double customTaxRate)
    : Item(id, std::move(sku), std::move(name), basePrice, stockQuantity, reorderThreshold, category, vendorId),
      customTaxRate_(customTaxRate) {}

double StandardCommodity::calculateTax(double standardTaxRate) const {
    double rate = (customTaxRate_ > 0.0) ? customTaxRate_ : standardTaxRate;
    return core::Money::round(basePrice_ * rate);
}

double StandardCommodity::calculateDiscount(int quantity) const {
    if (quantity >= 10) {
        return core::Money::round(basePrice_ * 0.10); // 10% volume discount per unit
    }
    if (quantity >= 5) {
        return core::Money::round(basePrice_ * 0.05); // 5% volume discount per unit
    }
    return 0.0;
}

bool StandardCommodity::isSellable() const {
    return stockQuantity_ > 0;
}

std::string StandardCommodity::serializeJson() const {
    std::ostringstream ss;
    ss << "{"
       << "\"id\":" << id_ << ","
       << "\"type\":\"STANDARD\","
       << "\"sku\":\"" << sku_.value() << "\","
       << "\"name\":\"" << name_ << "\","
       << "\"base_price\":" << std::fixed << std::setprecision(2) << basePrice_ << ","
       << "\"stock_quantity\":" << stockQuantity_ << ","
       << "\"reorder_threshold\":" << reorderThreshold_ << ","
       << "\"category\":\"" << core::categoryToString(category_) << "\","
       << "\"vendor_id\":" << vendorId_ << ","
       << "\"custom_tax_rate\":" << std::fixed << std::setprecision(4) << customTaxRate_
       << "}";
    return ss.str();
}

std::unique_ptr<Item> StandardCommodity::clone() const {
    return std::make_unique<StandardCommodity>(*this);
}

} // namespace storeforge::domain
