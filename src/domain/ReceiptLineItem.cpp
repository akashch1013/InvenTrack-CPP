#include "storeforge/domain/ReceiptLineItem.hpp"
#include "storeforge/core/Exceptions.hpp"
#include <sstream>
#include <iomanip>

namespace storeforge::domain {

ReceiptLineItem::ReceiptLineItem(core::LineItemId lineId,
                                 core::ReceiptId receiptId,
                                 core::ItemId itemId,
                                 core::SKU sku,
                                 std::string itemName,
                                 int quantity,
                                 double unitBasePrice,
                                 double discountPerUnit,
                                 double taxPerUnit)
    : lineId_(lineId),
      receiptId_(receiptId),
      itemId_(itemId),
      sku_(std::move(sku)),
      itemName_(std::move(itemName)),
      quantity_(quantity),
      unitBasePrice_(unitBasePrice),
      discountPerUnit_(discountPerUnit),
      taxPerUnit_(taxPerUnit) {
    if (quantity_ <= 0) {
        throw core::DomainValidationException("Line item quantity must be greater than zero");
    }
    if (unitBasePrice_ < 0.0) {
        throw core::DomainValidationException("Unit base price cannot be negative");
    }
    double effectiveUnit = std::max(0.0, unitBasePrice_ - discountPerUnit_) + taxPerUnit_;
    lineTotal_ = core::Money::round(effectiveUnit * quantity_);
}

std::string ReceiptLineItem::serializeJson() const {
    std::ostringstream ss;
    ss << "{"
       << "\"line_id\":" << lineId_ << ","
       << "\"receipt_id\":" << receiptId_ << ","
       << "\"item_id\":" << itemId_ << ","
       << "\"sku\":\"" << sku_.value() << "\","
       << "\"item_name\":\"" << itemName_ << "\","
       << "\"quantity\":" << quantity_ << ","
       << "\"unit_base_price\":" << std::fixed << std::setprecision(2) << unitBasePrice_ << ","
       << "\"discount_per_unit\":" << std::fixed << std::setprecision(2) << discountPerUnit_ << ","
       << "\"tax_per_unit\":" << std::fixed << std::setprecision(2) << taxPerUnit_ << ","
       << "\"line_total\":" << std::fixed << std::setprecision(2) << lineTotal_
       << "}";
    return ss.str();
}

} // namespace storeforge::domain
