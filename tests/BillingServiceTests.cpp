#include <gtest/gtest.h>
#include "storeforge/repo/InMemoryRepositories.hpp"
#include "storeforge/service/BillingService.hpp"
#include "storeforge/domain/StandardCommodity.hpp"
#include "storeforge/domain/PerishableGood.hpp"
#include "storeforge/core/Exceptions.hpp"

using namespace storeforge;

class BillingServiceTest : public ::testing::Test {
protected:
    repo::InMemoryItemRepository itemRepo;
    repo::InMemoryReceiptRepository receiptRepo;
    repo::InMemoryAuditLogRepository auditRepo;
    std::unique_ptr<service::BillingService> billingService;

    void SetUp() override {
        billingService = std::make_unique<service::BillingService>(itemRepo, receiptRepo, auditRepo);

        // Seed items
        domain::StandardCommodity i1(1, core::SKU("ITEM-1"), "Widget A", 10.0, 20, 5, core::ItemCategory::General, 1);
        domain::StandardCommodity i2(2, core::SKU("ITEM-2"), "Widget B", 25.0, 5, 2, core::ItemCategory::General, 1);

        auto now = core::DateTime::now();
        domain::PerishableGood p1(3, core::SKU("PERI-EXP"), "Old Milk", 5.0, 10, 2, core::ItemCategory::Grocery, 1, now.subtractDays(1));

        itemRepo.save(i1);
        itemRepo.save(i2);
        itemRepo.save(p1);
    }
};

TEST_F(BillingServiceTest, SuccessfulAtomicCheckout) {
    service::CheckoutRequest request;
    request.customerName = "John Doe";
    request.items = {
        {core::SKU("ITEM-1"), 2},
        {core::SKU("ITEM-2"), 1}
    };

    auto receipt = billingService->checkout(request, 0.10);

    EXPECT_GT(receipt.getId(), 0);
    EXPECT_FALSE(receipt.getReceiptNumber().empty());
    EXPECT_EQ(receipt.getCustomerName(), "John Doe");
    EXPECT_EQ(receipt.getLineItems().size(), 2);

    // Verify stock decremented
    auto item1 = itemRepo.findBySku(core::SKU("ITEM-1"));
    auto item2 = itemRepo.findBySku(core::SKU("ITEM-2"));
    EXPECT_EQ(item1.value()->getStockQuantity(), 18);
    EXPECT_EQ(item2.value()->getStockQuantity(), 4);

    // Verify audit logs were written
    auto logs = auditRepo.findAll();
    EXPECT_GE(logs.size(), 2);
}

TEST_F(BillingServiceTest, InsufficientStockTriggersRollback) {
    service::CheckoutRequest request;
    request.customerName = "Jane Doe";
    // ITEM-1 has 20 units, ITEM-2 has 5 units. Requesting 10 units of ITEM-2 must fail!
    request.items = {
        {core::SKU("ITEM-1"), 5},  // would reduce to 15
        {core::SKU("ITEM-2"), 10}  // fails!
    };

    EXPECT_THROW({
        billingService->checkout(request);
    }, core::InsufficientStockException);

    // Rollback verification: ITEM-1 stock MUST remain 20 (NOT modified)!
    auto item1 = itemRepo.findBySku(core::SKU("ITEM-1"));
    auto item2 = itemRepo.findBySku(core::SKU("ITEM-2"));
    EXPECT_EQ(item1.value()->getStockQuantity(), 20);
    EXPECT_EQ(item2.value()->getStockQuantity(), 5);
}

TEST_F(BillingServiceTest, RejectsExpiredPerishableProduct) {
    service::CheckoutRequest request;
    request.items = {
        {core::SKU("PERI-EXP"), 1}
    };

    EXPECT_THROW({
        billingService->checkout(request);
    }, core::ExpiredProductException);

    // Stock remained 10
    auto peri = itemRepo.findBySku(core::SKU("PERI-EXP"));
    EXPECT_EQ(peri.value()->getStockQuantity(), 10);
}
