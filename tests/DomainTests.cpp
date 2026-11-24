#include <gtest/gtest.h>
#include "storeforge/domain/StandardCommodity.hpp"
#include "storeforge/domain/PerishableGood.hpp"
#include "storeforge/domain/WarrantyItem.hpp"
#include "storeforge/core/Exceptions.hpp"

using namespace storeforge;

TEST(DomainTest, StandardCommodityVolumeDiscount) {
    domain::StandardCommodity item(1, core::SKU("COMM-01"), "Notepad", 10.0, 100, 10, core::ItemCategory::General, 1);

    // Quantity < 5: no discount
    EXPECT_DOUBLE_EQ(item.calculateDiscount(1), 0.0);
    EXPECT_DOUBLE_EQ(item.calculateDiscount(4), 0.0);

    // Quantity 5 - 9: 5% discount ($0.50 per unit)
    EXPECT_DOUBLE_EQ(item.calculateDiscount(5), 0.50);

    // Quantity >= 10: 10% discount ($1.00 per unit)
    EXPECT_DOUBLE_EQ(item.calculateDiscount(10), 1.00);
    EXPECT_DOUBLE_EQ(item.calculateDiscount(20), 1.00);

    // Tax calculation with default custom tax 8%
    EXPECT_DOUBLE_EQ(item.calculateTax(0.05), 0.80);
}

TEST(DomainTest, PerishableGoodMarkdownTiers) {
    auto now = core::DateTime::now();

    // Expiring in 1 day -> 50% markdown
    domain::PerishableGood urgent(1, core::SKU("PERI-01"), "Milk", 4.00, 10, 5, core::ItemCategory::Grocery, 1, now.addDays(1));
    EXPECT_TRUE(urgent.isSellable());
    EXPECT_DOUBLE_EQ(urgent.getMarkdownPercentage(), 0.50);
    EXPECT_DOUBLE_EQ(urgent.calculateDiscount(1), 2.00);

    // Expiring in 4 days -> 25% markdown
    domain::PerishableGood near(2, core::SKU("PERI-02"), "Bread", 4.00, 10, 5, core::ItemCategory::Grocery, 1, now.addDays(4));
    EXPECT_TRUE(near.isSellable());
    EXPECT_DOUBLE_EQ(near.getMarkdownPercentage(), 0.25);
    EXPECT_DOUBLE_EQ(near.calculateDiscount(1), 1.00);

    // Expired product -> not sellable
    domain::PerishableGood expired(3, core::SKU("PERI-03"), "Yogurt", 4.00, 10, 5, core::ItemCategory::Grocery, 1, now.subtractDays(2));
    EXPECT_FALSE(expired.isSellable());
    EXPECT_TRUE(expired.isExpired());
}

TEST(DomainTest, WarrantyItemValidationAndTax) {
    // Empty serial should throw validation exception
    EXPECT_THROW({
        domain::WarrantyItem invalid(1, core::SKU("ELEC-01"), "Mouse", 50.0, 10, 2, core::ItemCategory::Electronics, 1, "", 12);
    }, core::DomainValidationException);

    domain::WarrantyItem valid(1, core::SKU("ELEC-01"), "Laptop", 1000.0, 5, 2, core::ItemCategory::Electronics, 1, "SN-TEST-123", 24, 50.0);
    EXPECT_TRUE(valid.isSellable());
    EXPECT_EQ(valid.getSerialNumber(), "SN-TEST-123");
    EXPECT_EQ(valid.getWarrantyPeriodMonths(), 24);

    // Tax on base price + warranty fee (1050 * 0.12 = 126.00)
    EXPECT_DOUBLE_EQ(valid.calculateTax(0.12), 126.00);
}

TEST(DomainTest, PolymorphicCloningAndJsonSerialization) {
    std::unique_ptr<domain::Item> item = std::make_unique<domain::StandardCommodity>(
        42, core::SKU("POLY-01"), "Desk Lamp", 29.99, 15, 3, core::ItemCategory::HomeAndKitchen, 2
    );

    auto cloned = item->clone();
    EXPECT_NE(cloned, nullptr);
    EXPECT_EQ(cloned->getId(), 42);
    EXPECT_EQ(cloned->getSku().value(), "POLY-01");
    EXPECT_EQ(cloned->getType(), core::ItemType::StandardCommodity);

    std::string json = item->serializeJson();
    EXPECT_TRUE(json.find("POLY-01") != std::string::npos);
    EXPECT_TRUE(json.find("Desk Lamp") != std::string::npos);
}
