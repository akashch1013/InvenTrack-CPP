#pragma once

#include "storeforge/domain/Item.hpp"

namespace storeforge::domain {

class PerishableGood : public Item {
public:
    PerishableGood(core::ItemId id,
                   core::SKU sku,
                   std::string name,
                   double basePrice,
                   int stockQuantity,
                   int reorderThreshold,
                   core::ItemCategory category,
                   core::VendorId vendorId,
                   core::DateTime expirationDate,
                   double reducedTaxRate = 0.02);

    [[nodiscard]] core::ItemType getType() const noexcept override {
        return core::ItemType::PerishableGood;
    }

    [[nodiscard]] double calculateTax(double standardTaxRate) const override;
    [[nodiscard]] double calculateDiscount(int quantity) const override;
    [[nodiscard]] bool isSellable() const override;
    [[nodiscard]] std::string serializeJson() const override;
    [[nodiscard]] std::unique_ptr<Item> clone() const override;

    [[nodiscard]] const core::DateTime& getExpirationDate() const noexcept { return expirationDate_; }
    void setExpirationDate(const core::DateTime& dt) noexcept { expirationDate_ = dt; }

    [[nodiscard]] int getDaysUntilExpiration() const;
    [[nodiscard]] bool isExpired() const;
    [[nodiscard]] double getMarkdownPercentage() const;

private:
    core::DateTime expirationDate_;
    double reducedTaxRate_{0.02}; // Grocery staple reduced tax (2%)
};

} // namespace storeforge::domain
