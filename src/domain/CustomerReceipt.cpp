#include "storeforge/domain/CustomerReceipt.hpp"
#include <sstream>
#include <iomanip>

namespace storeforge::domain {

CustomerReceipt::CustomerReceipt(core::ReceiptId id,
                                 std::string receiptNumber,
                                 std::string customerName,
                                 std::string customerPhone,
                                 core::PaymentMethod paymentMethod,
                                 core::DateTime createdAt)
    : id_(id),
      receiptNumber_(std::move(receiptNumber)),
      customerName_(std::move(customerName)),
      customerPhone_(std::move(customerPhone)),
      paymentMethod_(paymentMethod),
      createdAt_(createdAt) {}

void CustomerReceipt::addLineItem(ReceiptLineItem item) {
    lineItems_.push_back(std::move(item));
    recalculateTotals();
}

void CustomerReceipt::recalculateTotals() {
    subtotal_ = 0.0;
    totalDiscount_ = 0.0;
    totalTax_ = 0.0;
    grandTotal_ = 0.0;

    for (const auto& item : lineItems_) {
        subtotal_ += item.getGrossSubtotal();
        totalDiscount_ += item.getTotalDiscount();
        totalTax_ += item.getTotalTax();
        grandTotal_ += item.getLineTotal();
    }

    subtotal_ = core::Money::round(subtotal_);
    totalDiscount_ = core::Money::round(totalDiscount_);
    totalTax_ = core::Money::round(totalTax_);
    grandTotal_ = core::Money::round(grandTotal_);
}

std::string CustomerReceipt::serializeJson() const {
    std::ostringstream ss;
    ss << "{"
       << "\"id\":" << id_ << ","
       << "\"receipt_number\":\"" << receiptNumber_ << "\","
       << "\"customer_name\":\"" << customerName_ << "\","
       << "\"customer_phone\":\"" << customerPhone_ << "\","
       << "\"payment_method\":\"" << core::paymentMethodToString(paymentMethod_) << "\","
       << "\"created_at\":\"" << createdAt_.toIsoString() << "\","
       << "\"subtotal\":" << std::fixed << std::setprecision(2) << subtotal_ << ","
       << "\"total_discount\":" << std::fixed << std::setprecision(2) << totalDiscount_ << ","
       << "\"total_tax\":" << std::fixed << std::setprecision(2) << totalTax_ << ","
       << "\"grand_total\":" << std::fixed << std::setprecision(2) << grandTotal_ << ","
       << "\"line_items\":[";

    for (size_t i = 0; i < lineItems_.size(); ++i) {
        if (i > 0) ss << ",";
        ss << lineItems_[i].serializeJson();
    }
    ss << "]}";
    return ss.str();
}

} // namespace storeforge::domain
