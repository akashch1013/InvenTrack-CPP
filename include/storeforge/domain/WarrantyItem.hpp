#pragma once

#include "storeforge/domain/Item.hpp"

namespace storeforge::domain {

class WarrantyItem : public Item {
public:
    WarrantyItem(core::ItemId id,
                 core::SKU sku,
                 std::string name,
                 double basePrice,
                 int stockQuantity,
                 int reorderThreshold,
                 core::ItemCategory category,
                 core::VendorId vendorId,
                 std::string serialNumber,
                 int warrantyPeriodMonths,
                 double warrantyFee = 0.0);

    [[nodiscard]] core::ItemType getType() const noexcept override {
        return core::ItemType::WarrantyItem;
    }

    [[nodiscard]] double calculateTax(double standardTaxRate) const override;
    [[nodiscard]] double calculateDiscount(int quantity) const override;
    [[nodiscard]] bool isSellable() const override;
    [[nodiscard]] std::string serializeJson() const override;
    [[nodiscard]] std::unique_ptr<Item> clone() const override;

    [[nodiscard]] const std::string& getSerialNumber() const noexcept { return serialNumber_; }
    [[nodiscard]] int getWarrantyPeriodMonths() const noexcept { return warrantyPeriodMonths_; }
    [[nodiscard]] double getWarrantyFee() const noexcept { return warrantyFee_; }

    void setSerialNumber(std::string serialNumber);
    void setWarrantyPeriodMonths(int months);
    void setWarrantyFee(double fee);

private:
    std::string serialNumber_;
    int warrantyPeriodMonths_{12};
    double warrantyFee_{0.0};
};

} // namespace storeforge::domain
