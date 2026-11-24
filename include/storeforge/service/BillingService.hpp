#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include "storeforge/repo/IItemRepository.hpp"
#include "storeforge/repo/IReceiptRepository.hpp"
#include "storeforge/repo/IAuditLogRepository.hpp"
#include "storeforge/domain/CustomerReceipt.hpp"

namespace storeforge::service {

struct CartEntry {
    core::SKU sku;
    int quantity{1};
};

struct CheckoutRequest {
    std::string customerName{"Walk-in Customer"};
    std::string customerPhone;
    core::PaymentMethod paymentMethod{core::PaymentMethod::Cash};
    std::string operatorId{"CASHIER-1"};
    std::vector<CartEntry> items;
};

struct CartValidationItem {
    std::string sku;
    std::string itemName;
    int requestedQuantity{0};
    int availableStock{0};
    bool isSellable{true};
    std::string errorMessage;
};

struct CartValidationResult {
    bool isValid{true};
    std::vector<CartValidationItem> items;
    std::string failureReason;
};

class BillingService {
public:
    BillingService(repo::IItemRepository& itemRepo,
                   repo::IReceiptRepository& receiptRepo,
                   repo::IAuditLogRepository& auditRepo);

    [[nodiscard]] CartValidationResult validateCart(const CheckoutRequest& request) const;

    // Executes atomic sale transaction
    domain::CustomerReceipt checkout(const CheckoutRequest& request, double standardTaxRate = 0.08);

    [[nodiscard]] std::string generateReceiptNumber() const;

private:
    repo::IItemRepository& itemRepo_;
    repo::IReceiptRepository& receiptRepo_;
    repo::IAuditLogRepository& auditRepo_;
    mutable std::mutex mutex_;
};

} // namespace storeforge::service
