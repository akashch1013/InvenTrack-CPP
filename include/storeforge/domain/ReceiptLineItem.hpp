#pragma once

#include <string>
#include "storeforge/core/Types.hpp"

namespace storeforge::domain {

class ReceiptLineItem {
public:
    ReceiptLineItem() = default;
    ReceiptLineItem(core::LineItemId lineId,
                    core::ReceiptId receiptId,
                    core::ItemId itemId,
                    core::SKU sku,
                    std::string itemName,
                    int quantity,
                    double unitBasePrice,
                    double discountPerUnit,
                    double taxPerUnit);

    [[nodiscard]] core::LineItemId getLineId() const noexcept { return lineId_; }
    [[nodiscard]] core::ReceiptId getReceiptId() const noexcept { return receiptId_; }
    [[nodiscard]] core::ItemId getItemId() const noexcept { return itemId_; }
    [[nodiscard]] const core::SKU& getSku() const noexcept { return sku_; }
    [[nodiscard]] const std::string& getItemName() const noexcept { return itemName_; }
    [[nodiscard]] int getQuantity() const noexcept { return quantity_; }
    [[nodiscard]] double getUnitBasePrice() const noexcept { return unitBasePrice_; }
    [[nodiscard]] double getDiscountPerUnit() const noexcept { return discountPerUnit_; }
    [[nodiscard]] double getTaxPerUnit() const noexcept { return taxPerUnit_; }
    [[nodiscard]] double getLineTotal() const noexcept { return lineTotal_; }

    [[nodiscard]] double getTotalDiscount() const noexcept { return core::Money::round(discountPerUnit_ * quantity_); }
    [[nodiscard]] double getTotalTax() const noexcept { return core::Money::round(taxPerUnit_ * quantity_); }
    [[nodiscard]] double getGrossSubtotal() const noexcept { return core::Money::round(unitBasePrice_ * quantity_); }

    void setLineId(core::LineItemId id) noexcept { lineId_ = id; }
    void setReceiptId(core::ReceiptId id) noexcept { receiptId_ = id; }

    [[nodiscard]] std::string serializeJson() const;

private:
    core::LineItemId lineId_{0};
    core::ReceiptId receiptId_{0};
    core::ItemId itemId_{0};
    core::SKU sku_;
    std::string itemName_;
    int quantity_{1};
    double unitBasePrice_{0.0};
    double discountPerUnit_{0.0};
    double taxPerUnit_{0.0};
    double lineTotal_{0.0};
};

} // namespace storeforge::domain
