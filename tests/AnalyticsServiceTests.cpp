#include <gtest/gtest.h>
#include "storeforge/repo/InMemoryRepositories.hpp"
#include "storeforge/service/AnalyticsService.hpp"
#include "storeforge/service/BillingService.hpp"
#include "storeforge/domain/StandardCommodity.hpp"
#include "storeforge/domain/PerishableGood.hpp"
#include "storeforge/domain/Vendor.hpp"

using namespace storeforge;

class AnalyticsServiceTest : public ::testing::Test {
protected:
    repo::InMemoryItemRepository itemRepo;
    repo::InMemoryVendorRepository vendorRepo;
    repo::InMemoryReceiptRepository receiptRepo;
    repo::InMemoryAuditLogRepository auditRepo;
    std::unique_ptr<service::AnalyticsService> analyticsService;
    std::unique_ptr<service::BillingService> billingService;

    void SetUp() override {
        analyticsService = std::make_unique<service::AnalyticsService>(itemRepo, vendorRepo, receiptRepo);
        billingService = std::make_unique<service::BillingService>(itemRepo, receiptRepo, auditRepo);

        // Vendor
        domain::Vendor v(1, "Global Supplies", "contact@global.com", "+1-800", "Addr", true);
        vendorRepo.save(v);

        // Low stock item: stock 3 <= threshold 5
        domain::StandardCommodity lowItem(1, core::SKU("LOW-01"), "Low Stock Item", 15.0, 3, 5, core::ItemCategory::General, 1);
        // Plentiful item: stock 50 > threshold 5
        domain::StandardCommodity normalItem(2, core::SKU("NORM-01"), "Normal Item", 20.0, 50, 5, core::ItemCategory::Electronics, 1);

        itemRepo.save(lowItem);
        itemRepo.save(normalItem);
    }
};

TEST_F(AnalyticsServiceTest, IdentifiesReorderAlerts) {
    auto alerts = analyticsService->generateReorderAlerts();
    ASSERT_EQ(alerts.size(), 1);
    EXPECT_EQ(alerts[0].sku, "LOW-01");
    EXPECT_EQ(alerts[0].currentStock, 3);
    EXPECT_EQ(alerts[0].vendorName, "Global Supplies");
    EXPECT_GT(alerts[0].suggestedReorderQuantity, 0);
}

TEST_F(AnalyticsServiceTest, DailyRevenueSummaryAggregation) {
    // Perform a sale
    service::CheckoutRequest req;
    req.customerName = "Test Buyer";
    req.items = {{core::SKU("NORM-01"), 2}};
    billingService->checkout(req, 0.10);

    auto summary = analyticsService->generateDailyReport();
    EXPECT_EQ(summary.receiptCount, 1);
    EXPECT_GT(summary.netGrandTotal, 0.0);
    EXPECT_DOUBLE_EQ(summary.averageBasketValue, summary.netGrandTotal);
}

TEST_F(AnalyticsServiceTest, CategoryPerformanceBreakdown) {
    service::CheckoutRequest req;
    req.items = {{core::SKU("NORM-01"), 3}};
    billingService->checkout(req, 0.10);

    auto breakdown = analyticsService->generateCategoryBreakdown();
    ASSERT_FALSE(breakdown.empty());
    EXPECT_EQ(breakdown[0].categoryName, "Electronics");
    EXPECT_EQ(breakdown[0].totalUnitsSold, 3);
}
