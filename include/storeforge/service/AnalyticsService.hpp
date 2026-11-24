#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "storeforge/repo/IItemRepository.hpp"
#include "storeforge/repo/IVendorRepository.hpp"
#include "storeforge/repo/IReceiptRepository.hpp"

namespace storeforge::service {

struct ReorderAlert {
    core::ItemId itemId{0};
    std::string sku;
    std::string itemName;
    int currentStock{0};
    int reorderThreshold{0};
    int suggestedReorderQuantity{0};
    core::VendorId vendorId{0};
    std::string vendorName;
    std::string vendorEmail;
};

struct RevenueSummary {
    std::string periodLabel;
    size_t receiptCount{0};
    double grossSubtotal{0.0};
    double totalDiscount{0.0};
    double totalTax{0.0};
    double netGrandTotal{0.0};
    double averageBasketValue{0.0};
};

struct CategoryPerformance {
    core::ItemCategory category{core::ItemCategory::General};
    std::string categoryName;
    int totalUnitsSold{0};
    double grossRevenue{0.0};
    double discountGranted{0.0};
    double netRevenue{0.0};
};

struct ExpiryAlert {
    core::ItemId itemId{0};
    std::string sku;
    std::string itemName;
    int currentStock{0};
    int daysRemaining{0};
    std::string expiryDate;
    bool isExpired{false};
};

class AnalyticsService {
public:
    AnalyticsService(repo::IItemRepository& itemRepo,
                     repo::IVendorRepository& vendorRepo,
                     repo::IReceiptRepository& receiptRepo);

    [[nodiscard]] std::vector<ReorderAlert> generateReorderAlerts() const;
    [[nodiscard]] RevenueSummary generateRevenueReport(const core::DateTime& start, const core::DateTime& end, const std::string& label = "Custom Range") const;
    [[nodiscard]] RevenueSummary generateDailyReport(const core::DateTime& date = core::DateTime::now()) const;
    [[nodiscard]] RevenueSummary generateMonthlyReport(int year, int month) const;
    [[nodiscard]] std::vector<CategoryPerformance> generateCategoryBreakdown() const;
    [[nodiscard]] std::vector<ExpiryAlert> generateExpiryAlerts(int thresholdDays = 7) const;

private:
    repo::IItemRepository& itemRepo_;
    repo::IVendorRepository& vendorRepo_;
    repo::IReceiptRepository& receiptRepo_;
};

} // namespace storeforge::service
