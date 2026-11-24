#pragma once

#include "storeforge/domain/Item.hpp"

namespace storeforge::domain {

class StandardCommodity : public Item {
public:
    StandardCommodity(core::ItemId id,
                      core::SKU sku,
                      std::string name,
                      double basePrice,
                      int stockQuantity,
                      int reorderThreshold,
                      core::ItemCategory category,
                      core::VendorId vendorId,
                      double customTaxRate = 0.08);

    [[nodiscard]] core::ItemType getType() const noexcept override {
        return core::ItemType::StandardCommodity;
    }

    [[nodiscard]] double calculateTax(double standardTaxRate) const override;
    [[nodiscard]] double calculateDiscount(int quantity) const override;
    [[nodiscard]] bool isSellable() const override;
    [[nodiscard]] std::string serializeJson() const override;
    [[nodiscard]] std::unique_ptr<Item> clone() const override;

    [[nodiscard]] double getCustomTaxRate() const noexcept { return customTaxRate_; }
    void setCustomTaxRate(double rate) noexcept { customTaxRate_ = rate; }

private:
    double customTaxRate_{0.08}; // Default 8%
};

} // namespace storeforge::domain
