#pragma once

#include <memory>
#include <iostream>
#include "storeforge/service/InventoryService.hpp"
#include "storeforge/service/BillingService.hpp"
#include "storeforge/service/AnalyticsService.hpp"
#include "storeforge/repo/IVendorRepository.hpp"

namespace storeforge::cli {

class CliApp {
public:
    CliApp(std::shared_ptr<service::InventoryService> inventoryService,
           std::shared_ptr<service::BillingService> billingService,
           std::shared_ptr<service::AnalyticsService> analyticsService,
           std::shared_ptr<repo::IVendorRepository> vendorRepo);

    void run(std::istream& in = std::cin, std::ostream& out = std::cout);

private:
    std::shared_ptr<service::InventoryService> inventoryService_;
    std::shared_ptr<service::BillingService> billingService_;
    std::shared_ptr<service::AnalyticsService> analyticsService_;
    std::shared_ptr<repo::IVendorRepository> vendorRepo_;

    void displayBanner(std::ostream& out) const;
    void displayMainMenu(std::ostream& out) const;

    void handleListInventory(std::ostream& out) const;
    void handlePosBilling(std::istream& in, std::ostream& out);
    void handleRestock(std::istream& in, std::ostream& out);
    void handleWriteOff(std::istream& in, std::ostream& out);
    void handleVendors(std::ostream& out) const;
    void handleAnalytics(std::ostream& out) const;
    void handleAuditLogs(std::ostream& out) const;

    void printReceipt(const domain::CustomerReceipt& receipt, std::ostream& out) const;
};

} // namespace storeforge::cli
