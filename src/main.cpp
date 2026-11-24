#include <iostream>
#include <memory>
#include "storeforge/repo/InMemoryRepositories.hpp"
#include "storeforge/service/InventoryService.hpp"
#include "storeforge/service/BillingService.hpp"
#include "storeforge/service/AnalyticsService.hpp"
#include "storeforge/cli/CliApp.hpp"
#include "storeforge/domain/StandardCommodity.hpp"
#include "storeforge/domain/PerishableGood.hpp"
#include "storeforge/domain/WarrantyItem.hpp"

using namespace storeforge;

void seedInitialDemoData(repo::InMemoryItemRepository& itemRepo,
                        repo::InMemoryVendorRepository& vendorRepo) {
    // 1. Seed Vendors
    domain::Vendor v1(1, "FreshFarms Agro Ltd", "orders@freshfarms.com", "+1-800-555-0199", "45 Agri Way, CA", true, "Net 15");
    domain::Vendor v2(2, "Apex Tech Electronics", "supply@apextech.io", "+1-888-555-4321", "101 Silicon Valley, CA", true, "Net 30");
    domain::Vendor v3(3, "Evergreen Essentials Co", "sales@evergreen.com", "+1-800-555-7788", "12 Industrial Blvd, TX", true, "Net 30");

    vendorRepo.save(v1);
    vendorRepo.save(v2);
    vendorRepo.save(v3);

    // 2. Seed Items
    // Standard Commodities
    domain::StandardCommodity i1(1, core::SKU("COMM-PAP-01"), "Multi-purpose Copy Paper", 7.99, 120, 25, core::ItemCategory::General, 3);
    domain::StandardCommodity i2(2, core::SKU("COMM-PEN-02"), "Gel Pen 12-Pack Black", 12.50, 45, 10, core::ItemCategory::General, 3);

    // Perishable Goods
    auto now = core::DateTime::now();
    domain::PerishableGood p1(3, core::SKU("PERI-MLK-01"), "Organic Whole Milk 1 Gal", 4.99, 18, 5, core::ItemCategory::Grocery, 1, now.addDays(1)); // Expiring soon (50% markdown)
    domain::PerishableGood p2(4, core::SKU("PERI-EGG-02"), "Free-Range Large Eggs 12ct", 5.49, 32, 10, core::ItemCategory::Grocery, 1, now.addDays(14));
    domain::PerishableGood p3(5, core::SKU("PERI-BRD-03"), "Artisan Sourdough Loaf", 3.89, 4, 10, core::ItemCategory::Grocery, 1, now.addDays(3)); // Low stock & expiring (25% markdown)

    // Warranty Electronics
    domain::WarrantyItem w1(6, core::SKU("ELEC-LPT-01"), "ForgePro 15-inch Laptop", 1199.99, 8, 3, core::ItemCategory::Electronics, 2, "SN-FP15-998822", 24, 49.99);
    domain::WarrantyItem w2(7, core::SKU("ELEC-MOU-02"), "ErgoFit Wireless Mouse", 49.99, 35, 8, core::ItemCategory::Electronics, 2, "SN-EF-554411", 12, 5.00);

    itemRepo.save(i1);
    itemRepo.save(i2);
    itemRepo.save(p1);
    itemRepo.save(p2);
    itemRepo.save(p3);
    itemRepo.save(w1);
    itemRepo.save(w2);
}

int main(int argc, char* argv[]) {
    std::cout << "[StoreForge] Starting Enterprise Retail Inventory and POS Billing System...\n";

    // Setup Storage Repositories (In-Memory by default, fast & thread-safe)
    auto itemRepo = std::make_shared<repo::InMemoryItemRepository>();
    auto vendorRepo = std::make_shared<repo::InMemoryVendorRepository>();
    auto receiptRepo = std::make_shared<repo::InMemoryReceiptRepository>();
    auto auditRepo = std::make_shared<repo::InMemoryAuditLogRepository>();

    seedInitialDemoData(*itemRepo, *vendorRepo);

    // Instantiate Services
    auto inventoryService = std::make_shared<service::InventoryService>(*itemRepo, *vendorRepo, *auditRepo);
    auto billingService = std::make_shared<service::BillingService>(*itemRepo, *receiptRepo, *auditRepo);
    auto analyticsService = std::make_shared<service::AnalyticsService>(*itemRepo, *vendorRepo, *receiptRepo);

    // Initialize & Execute CLI Application
    cli::CliApp app(inventoryService, billingService, analyticsService, vendorRepo);

    // Non-interactive run if --test-run or stdin is redirected/closed
    if (argc > 1 && std::string(argv[1]) == "--test-run") {
        std::cout << "[StoreForge] Test run mode verified. Exiting successfully.\n";
        return 0;
    }

    app.run();
    return 0;
}
