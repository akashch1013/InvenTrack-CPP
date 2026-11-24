#include "storeforge/service/BillingService.hpp"
#include "storeforge/core/Exceptions.hpp"
#include "storeforge/domain/PerishableGood.hpp"
#include <sstream>
#include <iomanip>
#include <random>

namespace storeforge::service {

BillingService::BillingService(repo::IItemRepository& itemRepo,
                               repo::IReceiptRepository& receiptRepo,
                               repo::IAuditLogRepository& auditRepo)
    : itemRepo_(itemRepo),
      receiptRepo_(receiptRepo),
      auditRepo_(auditRepo) {}

CartValidationResult BillingService::validateCart(const CheckoutRequest& request) const {
    std::lock_guard lock(mutex_);
    CartValidationResult result;
    result.isValid = true;

    if (request.items.empty()) {
        result.isValid = false;
        result.failureReason = "Checkout request cart is empty";
        return result;
    }

    for (const auto& entry : request.items) {
        CartValidationItem vItem;
        vItem.sku = entry.sku.value();
        vItem.requestedQuantity = entry.quantity;

        if (entry.quantity <= 0) {
            vItem.isSellable = false;
            vItem.errorMessage = "Requested quantity must be greater than zero";
            result.isValid = false;
            result.items.push_back(vItem);
            continue;
        }

        auto itemOpt = itemRepo_.findBySku(entry.sku);
        if (!itemOpt.has_value()) {
            vItem.isSellable = false;
            vItem.errorMessage = "Product with SKU " + entry.sku.value() + " does not exist";
            result.isValid = false;
            result.items.push_back(vItem);
            continue;
        }

        const auto& item = *itemOpt.value();
        vItem.itemName = item.getName();
        vItem.availableStock = item.getStockQuantity();

        if (!item.isSellable()) {
            vItem.isSellable = false;
            vItem.errorMessage = "Product is not sellable (e.g., expired or out of stock)";
            result.isValid = false;
        } else if (item.getStockQuantity() < entry.quantity) {
            vItem.isSellable = false;
            vItem.errorMessage = "Insufficient stock: requested " + std::to_string(entry.quantity) +
                                 ", available " + std::to_string(item.getStockQuantity());
            result.isValid = false;
        }

        result.items.push_back(vItem);
    }

    if (!result.isValid && result.failureReason.empty()) {
        result.failureReason = "One or more cart items failed validation";
    }

    return result;
}

std::string BillingService::generateReceiptNumber() const {
    auto now = core::DateTime::now();
    std::string datePart = now.toDateString();
    // remove dashes
    std::string cleanDate;
    for (char c : datePart) {
        if (c != '-') cleanDate += c;
    }

    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(1000, 9999);
    int randomSuffix = dist(rng);

    return "REC-" + cleanDate + "-" + std::to_string(randomSuffix);
}

domain::CustomerReceipt BillingService::checkout(const CheckoutRequest& request, double standardTaxRate) {
    std::lock_guard lock(mutex_);

    // 1. Validation phase
    if (request.items.empty()) {
        throw core::DomainValidationException("Cannot checkout with an empty cart");
    }

    struct ItemProcessInfo {
        std::unique_ptr<domain::Item> item;
        int quantity;
        double discountPerUnit;
        double taxPerUnit;
        int originalStock;
    };

    std::vector<ItemProcessInfo> itemsToProcess;
    itemsToProcess.reserve(request.items.size());

    for (const auto& entry : request.items) {
        if (entry.quantity <= 0) {
            throw core::DomainValidationException("Cart quantity must be positive for SKU: " + entry.sku.value());
        }

        auto itemOpt = itemRepo_.findBySku(entry.sku);
        if (!itemOpt.has_value()) {
            throw core::EntityNotFoundException("Item SKU", entry.sku.value());
        }

        auto item = std::move(itemOpt.value());

        // Check if perishable is expired
        if (item->getType() == core::ItemType::PerishableGood) {
            const auto* pg = dynamic_cast<const domain::PerishableGood*>(item.get());
            if (pg && pg->isExpired()) {
                throw core::ExpiredProductException(entry.sku.value(), pg->getExpirationDate().toDateString());
            }
        }

        if (!item->isSellable()) {
            throw core::DomainValidationException("Product SKU " + entry.sku.value() + " is currently unsellable");
        }

        if (item->getStockQuantity() < entry.quantity) {
            throw core::InsufficientStockException(entry.sku.value(), entry.quantity, item->getStockQuantity());
        }

        double discount = item->calculateDiscount(entry.quantity);
        double tax = item->calculateTax(standardTaxRate);
        int originalStock = item->getStockQuantity();

        itemsToProcess.push_back({std::move(item), entry.quantity, discount, tax, originalStock});
    }

    // 2. Execution phase with rollback safety
    std::vector<std::pair<core::ItemId, int>> modifiedStockItems;
    std::string receiptNumber = generateReceiptNumber();
    domain::CustomerReceipt receipt(
        0, receiptNumber, request.customerName, request.customerPhone, request.paymentMethod, core::DateTime::now()
    );

    try {
        for (auto& info : itemsToProcess) {
            int newStock = info.originalStock - info.quantity;
            itemRepo_.updateStock(info.item->getId(), newStock);
            modifiedStockItems.emplace_back(info.item->getId(), info.originalStock);

            // Audit log
            domain::InventoryAuditLog auditLog(
                0,
                info.item->getId(),
                info.item->getSku().value(),
                core::AuditAction::SALE,
                -info.quantity,
                info.originalStock,
                newStock,
                "POS Checkout Receipt #" + receiptNumber,
                request.operatorId,
                core::DateTime::now()
            );
            auditRepo_.save(auditLog);

            // Line item
            domain::ReceiptLineItem line(
                0,
                0,
                info.item->getId(),
                info.item->getSku(),
                info.item->getName(),
                info.quantity,
                info.item->getBasePrice(),
                info.discountPerUnit,
                info.taxPerUnit
            );
            receipt.addLineItem(line);
        }

        // 3. Persist receipt
        core::ReceiptId savedId = receiptRepo_.save(receipt);
        receipt.setId(savedId);

        return receipt;

    } catch (const std::exception& e) {
        // Rollback modified stock in reverse order
        for (const auto& [itemId, origStock] : modifiedStockItems) {
            try {
                itemRepo_.updateStock(itemId, origStock);
            } catch (...) {
                // Secondary error suppression during compensation
            }
        }
        throw core::TransactionException(std::string("Checkout failed and changes were rolled back: ") + e.what());
    }
}

} // namespace storeforge::service
