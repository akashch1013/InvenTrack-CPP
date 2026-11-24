#include "storeforge/service/AnalyticsService.hpp"
#include "storeforge/domain/PerishableGood.hpp"
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace storeforge::service {

AnalyticsService::AnalyticsService(repo::IItemRepository& itemRepo,
                                   repo::IVendorRepository& vendorRepo,
                                   repo::IReceiptRepository& receiptRepo)
    : itemRepo_(itemRepo),
      vendorRepo_(vendorRepo),
      receiptRepo_(receiptRepo) {}

std::vector<ReorderAlert> AnalyticsService::generateReorderAlerts() const {
    auto lowStockItems = itemRepo_.findLowStock();
    std::vector<ReorderAlert> alerts;
    alerts.reserve(lowStockItems.size());

    for (const auto& item : lowStockItems) {
        ReorderAlert alert;
        alert.itemId = item->getId();
        alert.sku = item->getSku().value();
        alert.itemName = item->getName();
        alert.currentStock = item->getStockQuantity();
        alert.reorderThreshold = item->getReorderThreshold();
        alert.suggestedReorderQuantity = std::max(20, item->getReorderThreshold() * 3);
        alert.vendorId = item->getVendorId();

        if (item->getVendorId() > 0) {
            auto vendorOpt = vendorRepo_.findById(item->getVendorId());
            if (vendorOpt.has_value()) {
                alert.vendorName = vendorOpt->getName();
                alert.vendorEmail = vendorOpt->getContactEmail();
            } else {
                alert.vendorName = "Unassigned Vendor";
                alert.vendorEmail = "N/A";
            }
        } else {
            alert.vendorName = "Direct Ingestion";
            alert.vendorEmail = "N/A";
        }

        alerts.push_back(alert);
    }

    return alerts;
}

RevenueSummary AnalyticsService::generateRevenueReport(const core::DateTime& start,
                                                       const core::DateTime& end,
                                                       const std::string& label) const {
    auto receipts = receiptRepo_.findByDateRange(start, end);
    RevenueSummary summary;
    summary.periodLabel = label;
    summary.receiptCount = receipts.size();

    for (const auto& receipt : receipts) {
        summary.grossSubtotal += receipt.getSubtotal();
        summary.totalDiscount += receipt.getTotalDiscount();
        summary.totalTax += receipt.getTotalTax();
        summary.netGrandTotal += receipt.getGrandTotal();
    }

    summary.grossSubtotal = core::Money::round(summary.grossSubtotal);
    summary.totalDiscount = core::Money::round(summary.totalDiscount);
    summary.totalTax = core::Money::round(summary.totalTax);
    summary.netGrandTotal = core::Money::round(summary.netGrandTotal);

    if (summary.receiptCount > 0) {
        summary.averageBasketValue = core::Money::round(summary.netGrandTotal / static_cast<double>(summary.receiptCount));
    }

    return summary;
}

RevenueSummary AnalyticsService::generateDailyReport(const core::DateTime& date) const {
    std::string dateStr = date.toDateString();
    core::DateTime dayStart = core::DateTime::fromDateString(dateStr);
    core::DateTime dayEnd = dayStart.addDays(1).subtractDays(0);

    return generateRevenueReport(dayStart, dayEnd, "Daily: " + dateStr);
}

RevenueSummary AnalyticsService::generateMonthlyReport(int year, int month) const {
    std::ostringstream ssStart, ssEnd;
    ssStart << year << "-" << std::setw(2) << std::setfill('0') << month << "-01";
    int nextYear = (month == 12) ? year + 1 : year;
    int nextMonth = (month == 12) ? 1 : month + 1;
    ssEnd << nextYear << "-" << std::setw(2) << std::setfill('0') << nextMonth << "-01";

    core::DateTime start = core::DateTime::fromDateString(ssStart.str());
    core::DateTime end = core::DateTime::fromDateString(ssEnd.str());

    std::ostringstream label;
    label << "Month: " << year << "-" << std::setw(2) << std::setfill('0') << month;
    return generateRevenueReport(start, end, label.str());
}

std::vector<CategoryPerformance> AnalyticsService::generateCategoryBreakdown() const {
    auto allReceipts = receiptRepo_.findAll();

    struct CatAccumulator {
        int units{0};
        double gross{0.0};
        double discount{0.0};
        double net{0.0};
    };

    std::unordered_map<core::ItemCategory, CatAccumulator> catMap;
    // Map item id to category
    std::unordered_map<core::ItemId, core::ItemCategory> itemCategoryMap;
    auto allItems = itemRepo_.findAll();
    for (const auto& it : allItems) {
        itemCategoryMap[it->getId()] = it->getCategory();
    }

    for (const auto& receipt : allReceipts) {
        for (const auto& line : receipt.getLineItems()) {
            core::ItemCategory cat = core::ItemCategory::General;
            auto itCat = itemCategoryMap.find(line.getItemId());
            if (itCat != itemCategoryMap.end()) {
                cat = itCat->second;
            }

            auto& acc = catMap[cat];
            acc.units += line.getQuantity();
            acc.gross += line.getGrossSubtotal();
            acc.discount += line.getTotalDiscount();
            acc.net += line.getLineTotal();
        }
    }

    std::vector<CategoryPerformance> results;
    results.reserve(catMap.size());

    for (const auto& [cat, acc] : catMap) {
        CategoryPerformance perf;
        perf.category = cat;
        perf.categoryName = core::categoryToString(cat);
        perf.totalUnitsSold = acc.units;
        perf.grossRevenue = core::Money::round(acc.gross);
        perf.discountGranted = core::Money::round(acc.discount);
        perf.netRevenue = core::Money::round(acc.net);
        results.push_back(perf);
    }

    std::sort(results.begin(), results.end(), [](const auto& a, const auto& b) {
        return a.netRevenue > b.netRevenue;
    });

    return results;
}

std::vector<ExpiryAlert> AnalyticsService::generateExpiryAlerts(int thresholdDays) const {
    auto allItems = itemRepo_.findAll();
    std::vector<ExpiryAlert> alerts;

    for (const auto& item : allItems) {
        if (item->getType() == core::ItemType::PerishableGood) {
            const auto* pg = dynamic_cast<const domain::PerishableGood*>(item.get());
            if (pg) {
                int days = pg->getDaysUntilExpiration();
                if (days <= thresholdDays) {
                    ExpiryAlert alert;
                    alert.itemId = pg->getId();
                    alert.sku = pg->getSku().value();
                    alert.itemName = pg->getName();
                    alert.currentStock = pg->getStockQuantity();
                    alert.daysRemaining = days;
                    alert.expiryDate = pg->getExpirationDate().toDateString();
                    alert.isExpired = pg->isExpired();
                    alerts.push_back(alert);
                }
            }
        }
    }

    std::sort(alerts.begin(), alerts.end(), [](const auto& a, const auto& b) {
        return a.daysRemaining < b.daysRemaining;
    });

    return alerts;
}

} // namespace storeforge::service
