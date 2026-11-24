#pragma once

#include <string>
#include <vector>
#include "storeforge/core/Types.hpp"
#include "storeforge/core/DateTime.hpp"
#include "storeforge/domain/ReceiptLineItem.hpp"

namespace storeforge::domain {

class CustomerReceipt {
public:
    CustomerReceipt() = default;
    CustomerReceipt(core::ReceiptId id,
                    std::string receiptNumber,
                    std::string customerName,
                    std::string customerPhone,
                    core::PaymentMethod paymentMethod,
                    core::DateTime createdAt = core::DateTime::now());

    [[nodiscard]] core::ReceiptId getId() const noexcept { return id_; }
    [[nodiscard]] const std::string& getReceiptNumber() const noexcept { return receiptNumber_; }
    [[nodiscard]] const std::string& getCustomerName() const noexcept { return customerName_; }
    [[nodiscard]] const std::string& getCustomerPhone() const noexcept { return customerPhone_; }
    [[nodiscard]] core::PaymentMethod getPaymentMethod() const noexcept { return paymentMethod_; }
    [[nodiscard]] const core::DateTime& getCreatedAt() const noexcept { return createdAt_; }

    [[nodiscard]] const std::vector<ReceiptLineItem>& getLineItems() const noexcept { return lineItems_; }
    [[nodiscard]] double getSubtotal() const noexcept { return subtotal_; }
    [[nodiscard]] double getTotalDiscount() const noexcept { return totalDiscount_; }
    [[nodiscard]] double getTotalTax() const noexcept { return totalTax_; }
    [[nodiscard]] double getGrandTotal() const noexcept { return grandTotal_; }

    void setId(core::ReceiptId id) noexcept { id_ = id; }
    void setReceiptNumber(std::string num) { receiptNumber_ = std::move(num); }
    void setCustomerName(std::string name) { customerName_ = std::move(name); }
    void setCustomerPhone(std::string phone) { customerPhone_ = std::move(phone); }
    void setPaymentMethod(core::PaymentMethod method) noexcept { paymentMethod_ = method; }
    void setCreatedAt(const core::DateTime& dt) noexcept { createdAt_ = dt; }

    void addLineItem(ReceiptLineItem item);
    void recalculateTotals();

    [[nodiscard]] std::string serializeJson() const;

private:
    core::ReceiptId id_{0};
    std::string receiptNumber_;
    std::string customerName_{"Walk-in Customer"};
    std::string customerPhone_;
    core::PaymentMethod paymentMethod_{core::PaymentMethod::Cash};
    core::DateTime createdAt_{core::DateTime::now()};

    std::vector<ReceiptLineItem> lineItems_;
    double subtotal_{0.0};
    double totalDiscount_{0.0};
    double totalTax_{0.0};
    double grandTotal_{0.0};
};

} // namespace storeforge::domain
