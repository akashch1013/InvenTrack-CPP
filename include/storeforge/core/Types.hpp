#pragma once

#include <string>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <iomanip>

namespace storeforge::core {

using ItemId = int64_t;
using VendorId = int64_t;
using ReceiptId = int64_t;
using LineItemId = int64_t;
using AuditLogId = int64_t;

enum class ItemType {
    StandardCommodity = 1,
    PerishableGood = 2,
    WarrantyItem = 3
};

inline std::string itemTypeToString(ItemType type) {
    switch (type) {
        case ItemType::StandardCommodity: return "STANDARD";
        case ItemType::PerishableGood:     return "PERISHABLE";
        case ItemType::WarrantyItem:       return "WARRANTY";
        default:                           return "UNKNOWN";
    }
}

inline ItemType stringToItemType(const std::string& str) {
    if (str == "PERISHABLE" || str == "PerishableGood") return ItemType::PerishableGood;
    if (str == "WARRANTY" || str == "WarrantyItem")     return ItemType::WarrantyItem;
    return ItemType::StandardCommodity;
}

enum class ItemCategory {
    Grocery,
    Electronics,
    Apparel,
    HomeAndKitchen,
    Healthcare,
    Automotive,
    General
};

inline std::string categoryToString(ItemCategory cat) {
    switch (cat) {
        case ItemCategory::Grocery:        return "Grocery";
        case ItemCategory::Electronics:    return "Electronics";
        case ItemCategory::Apparel:        return "Apparel";
        case ItemCategory::HomeAndKitchen: return "Home & Kitchen";
        case ItemCategory::Healthcare:     return "Healthcare";
        case ItemCategory::Automotive:     return "Automotive";
        case ItemCategory::General:
        default:                           return "General";
    }
}

inline ItemCategory stringToCategory(const std::string& str) {
    if (str == "Grocery")        return ItemCategory::Grocery;
    if (str == "Electronics")    return ItemCategory::Electronics;
    if (str == "Apparel")        return ItemCategory::Apparel;
    if (str == "Home & Kitchen" || str == "HomeAndKitchen") return ItemCategory::HomeAndKitchen;
    if (str == "Healthcare")     return ItemCategory::Healthcare;
    if (str == "Automotive")     return ItemCategory::Automotive;
    return ItemCategory::General;
}

enum class PaymentMethod {
    Cash,
    CreditCard,
    DebitCard,
    UPI_Digital,
    StoreCredit
};

inline std::string paymentMethodToString(PaymentMethod method) {
    switch (method) {
        case PaymentMethod::Cash:        return "CASH";
        case PaymentMethod::CreditCard:  return "CREDIT_CARD";
        case PaymentMethod::DebitCard:   return "DEBIT_CARD";
        case PaymentMethod::UPI_Digital: return "UPI_DIGITAL";
        case PaymentMethod::StoreCredit: return "STORE_CREDIT";
        default:                         return "CASH";
    }
}

inline PaymentMethod stringToPaymentMethod(const std::string& str) {
    if (str == "CREDIT_CARD")  return PaymentMethod::CreditCard;
    if (str == "DEBIT_CARD")   return PaymentMethod::DebitCard;
    if (str == "UPI_DIGITAL")  return PaymentMethod::UPI_Digital;
    if (str == "STORE_CREDIT") return PaymentMethod::StoreCredit;
    return PaymentMethod::Cash;
}

enum class AuditAction {
    RESTOCK,
    SALE,
    RETURN,
    WRITE_OFF,
    MANUAL_ADJUSTMENT
};

inline std::string auditActionToString(AuditAction action) {
    switch (action) {
        case AuditAction::RESTOCK:           return "RESTOCK";
        case AuditAction::SALE:              return "SALE";
        case AuditAction::RETURN:            return "RETURN";
        case AuditAction::WRITE_OFF:         return "WRITE_OFF";
        case AuditAction::MANUAL_ADJUSTMENT: return "ADJUSTMENT";
        default:                             return "UNKNOWN";
    }
}

inline AuditAction stringToAuditAction(const std::string& str) {
    if (str == "RESTOCK")    return AuditAction::RESTOCK;
    if (str == "SALE")       return AuditAction::SALE;
    if (str == "RETURN")     return AuditAction::RETURN;
    if (str == "WRITE_OFF")  return AuditAction::WRITE_OFF;
    return AuditAction::MANUAL_ADJUSTMENT;
}

// Strong type wrapper for SKU (Stock Keeping Unit)
class SKU {
public:
    SKU() = default;
    explicit SKU(std::string value) : value_(std::move(value)) {}

    [[nodiscard]] const std::string& value() const noexcept { return value_; }
    [[nodiscard]] bool empty() const noexcept { return value_.empty(); }

    bool operator==(const SKU& other) const noexcept { return value_ == other.value_; }
    bool operator!=(const SKU& other) const noexcept { return value_ != other.value_; }
    bool operator<(const SKU& other) const noexcept { return value_ < other.value_; }

    friend std::ostream& operator<<(std::ostream& os, const SKU& sku) {
        return os << sku.value_;
    }

private:
    std::string value_;
};

// Monetary value helper with double precision and 2-decimal rounding
struct Money {
    static constexpr double EPSILON = 0.0001;

    static double round(double amount) {
        return static_cast<double>(static_cast<int64_t>(amount * 100.0 + (amount >= 0 ? 0.5 : -0.5))) / 100.0;
    }

    static std::string format(double amount, const std::string& currencySymbol = "$") {
        std::ostringstream ss;
        ss << currencySymbol << std::fixed << std::setprecision(2) << amount;
        return ss.str();
    }
};

} // namespace storeforge::core
