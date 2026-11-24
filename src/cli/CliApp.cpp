#include "storeforge/cli/CliApp.hpp"
#include "storeforge/cli/TablePrinter.hpp"
#include "storeforge/core/Exceptions.hpp"
#include "storeforge/domain/PerishableGood.hpp"
#include "storeforge/domain/WarrantyItem.hpp"
#include <iostream>
#include <sstream>
#include <limits>

namespace storeforge::cli {

CliApp::CliApp(std::shared_ptr<service::InventoryService> inventoryService,
               std::shared_ptr<service::BillingService> billingService,
               std::shared_ptr<service::AnalyticsService> analyticsService,
               std::shared_ptr<repo::IVendorRepository> vendorRepo)
    : inventoryService_(std::move(inventoryService)),
      billingService_(std::move(billingService)),
      analyticsService_(std::move(analyticsService)),
      vendorRepo_(std::move(vendorRepo)) {}

void CliApp::displayBanner(std::ostream& out) const {
    out << "\n"
        << TablePrinter::cyan("=======================================================================\n")
        << TablePrinter::bold("   STOREFORGE  -  Enterprise Retail Inventory & POS Billing System   \n")
        << TablePrinter::dim("               Modern C++17 Architecture with PostgreSQL Backend       \n")
        << TablePrinter::cyan("=======================================================================\n\n");
}

void CliApp::displayMainMenu(std::ostream& out) const {
    out << TablePrinter::bold("\n--- Main Operational Menu ---\n")
        << "  " << TablePrinter::yellow("1.") << " Inventory Catalog & Stock Overview\n"
        << "  " << TablePrinter::yellow("2.") << " POS Billing & Customer Checkout\n"
        << "  " << TablePrinter::yellow("3.") << " Stock Replenishment (Restock)\n"
        << "  " << TablePrinter::yellow("4.") << " Stock Write-Off (Damage / Spoilage)\n"
        << "  " << TablePrinter::yellow("5.") << " Vendor Directory\n"
        << "  " << TablePrinter::yellow("6.") << " Analytics & Intelligence Reports\n"
        << "  " << TablePrinter::yellow("7.") << " Inventory Audit Trail\n"
        << "  " << TablePrinter::yellow("8.") << " Exit System\n"
        << TablePrinter::cyan("\nEnter selection [1-8]: ");
}

void CliApp::handleListInventory(std::ostream& out) const {
    auto items = inventoryService_->getAllItems();
    out << "\n" << TablePrinter::bold(">> Current Inventory Catalog (") << items.size() << " products):\n";

    TablePrinter table;
    table.addColumn("ID", 4, Alignment::Right);
    table.addColumn("SKU", 12, Alignment::Left);
    table.addColumn("Product Name", 24, Alignment::Left);
    table.addColumn("Type", 12, Alignment::Center);
    table.addColumn("Category", 14, Alignment::Left);
    table.addColumn("Base Price", 10, Alignment::Right);
    table.addColumn("Stock", 8, Alignment::Right);
    table.addColumn("Status", 12, Alignment::Center);

    for (const auto& item : items) {
        std::string status;
        if (!item->isSellable()) {
            status = TablePrinter::red("EXPIRED/UNSELL");
        } else if (item->isLowStock()) {
            status = TablePrinter::yellow("LOW STOCK");
        } else {
            status = TablePrinter::green("IN STOCK");
        }

        table.addRow({
            std::to_string(item->getId()),
            item->getSku().value(),
            item->getName(),
            core::itemTypeToString(item->getType()),
            core::categoryToString(item->getCategory()),
            core::Money::format(item->getBasePrice()),
            std::to_string(item->getStockQuantity()),
            status
        });
    }

    table.print(out);
}

void CliApp::printReceipt(const domain::CustomerReceipt& receipt, std::ostream& out) const {
    out << "\n"
        << TablePrinter::cyan("=======================================================================\n")
        << TablePrinter::bold("                     CUSTOMER INVOICE & RECEIPT                       \n")
        << TablePrinter::cyan("=======================================================================\n")
        << "  Receipt Number: " << TablePrinter::bold(receipt.getReceiptNumber()) << "\n"
        << "  Customer Name : " << receipt.getCustomerName() << "\n"
        << "  Payment Method: " << core::paymentMethodToString(receipt.getPaymentMethod()) << "\n"
        << "  Timestamp     : " << receipt.getCreatedAt().toDisplayString() << "\n"
        << "-----------------------------------------------------------------------\n";

    TablePrinter table;
    table.addColumn("SKU", 12, Alignment::Left);
    table.addColumn("Item Name", 24, Alignment::Left);
    table.addColumn("Qty", 5, Alignment::Right);
    table.addColumn("Unit Price", 10, Alignment::Right);
    table.addColumn("Discount", 10, Alignment::Right);
    table.addColumn("Tax", 10, Alignment::Right);
    table.addColumn("Total", 10, Alignment::Right);

    for (const auto& line : receipt.getLineItems()) {
        table.addRow({
            line.getSku().value(),
            line.getItemName(),
            std::to_string(line.getQuantity()),
            core::Money::format(line.getUnitBasePrice()),
            core::Money::format(line.getTotalDiscount()),
            core::Money::format(line.getTotalTax()),
            core::Money::format(line.getLineTotal())
        });
    }

    table.print(out);

    out << "-----------------------------------------------------------------------\n"
        << "  Subtotal (Gross)  : " << core::Money::format(receipt.getSubtotal()) << "\n"
        << "  Total Discounts   : " << TablePrinter::green("-" + core::Money::format(receipt.getTotalDiscount())) << "\n"
        << "  Total Tax (VAT)   : " << core::Money::format(receipt.getTotalTax()) << "\n"
        << "  GRAND TOTAL DUE   : " << TablePrinter::bold(TablePrinter::green(core::Money::format(receipt.getGrandTotal()))) << "\n"
        << TablePrinter::cyan("=======================================================================\n")
        << "       Thank you for choosing StoreForge Retail Systems!              \n"
        << TablePrinter::cyan("=======================================================================\n\n");
}

void CliApp::handlePosBilling(std::istream& in, std::ostream& out) {
    out << "\n" << TablePrinter::bold(">> Point of Sale (POS) Checkout Terminal\n");
    service::CheckoutRequest request;
    request.operatorId = "POS-TERMINAL-01";

    out << "Enter Customer Name [or press Enter for 'Walk-in Customer']: ";
    std::string custName;
    if (std::getline(in, custName) && !custName.empty()) {
        request.customerName = custName;
    }

    out << "Enter Customer Phone (optional): ";
    std::string custPhone;
    std::getline(in, custPhone);
    request.customerPhone = custPhone;

    out << "\n--- Add Cart Items (type 'DONE' when finished) ---\n";
    while (true) {
        out << "Enter SKU (or 'DONE'): ";
        std::string skuInput;
        if (!(in >> skuInput) || skuInput == "DONE" || skuInput == "done") {
            break;
        }

        auto itemOpt = inventoryService_->getItemBySku(core::SKU(skuInput));
        if (!itemOpt.has_value()) {
            out << TablePrinter::red("Error: Product with SKU '" + skuInput + "' not found!\n");
            continue;
        }

        const auto& item = *itemOpt.value();
        out << "Found: " << item.getName() << " | Available Stock: " << item.getStockQuantity()
            << " | Price: " << core::Money::format(item.getBasePrice()) << "\n";

        if (!item.isSellable()) {
            out << TablePrinter::red("Warning: Item is marked UNSELLABLE (expired or out of stock).\n");
        }

        out << "Enter Quantity to purchase: ";
        int qty = 0;
        if (!(in >> qty) || qty <= 0) {
            out << TablePrinter::red("Invalid quantity.\n");
            in.clear();
            in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        request.items.push_back({core::SKU(skuInput), qty});
        out << TablePrinter::green("Added " + std::to_string(qty) + "x " + item.getName() + " to cart.\n");
    }

    in.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear newline

    if (request.items.empty()) {
        out << TablePrinter::yellow("Cart is empty. Transaction cancelled.\n");
        return;
    }

    out << "\nSelect Payment Method (1=Cash, 2=Credit Card, 3=Debit Card, 4=UPI/Digital): ";
    int pChoice = 1;
    if (in >> pChoice) {
        if (pChoice == 2) request.paymentMethod = core::PaymentMethod::CreditCard;
        else if (pChoice == 3) request.paymentMethod = core::PaymentMethod::DebitCard;
        else if (pChoice == 4) request.paymentMethod = core::PaymentMethod::UPI_Digital;
        else request.paymentMethod = core::PaymentMethod::Cash;
    }
    in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    out << "\nExecuting atomic POS transaction...\n";
    try {
        auto receipt = billingService_->checkout(request);
        out << TablePrinter::green(">> Transaction Committed Successfully!\n");
        printReceipt(receipt, out);
    } catch (const core::StoreForgeException& ex) {
        out << TablePrinter::red(">> Checkout Aborted! Reason: " + std::string(ex.what()) + "\n");
        out << TablePrinter::yellow(">> Stock levels preserved via transactional rollback.\n");
    }
}

void CliApp::handleRestock(std::istream& in, std::ostream& out) {
    out << "\n" << TablePrinter::bold(">> Inventory Restock\n");
    out << "Enter Item ID: ";
    core::ItemId id = 0;
    if (!(in >> id)) {
        in.clear();
        in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }

    out << "Enter Quantity to add: ";
    int qty = 0;
    if (!(in >> qty) || qty <= 0) {
        in.clear();
        in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }
    in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    out << "Enter Reason (or press Enter for 'Vendor Delivery'): ";
    std::string reason;
    std::getline(in, reason);
    if (reason.empty()) reason = "Vendor Delivery Replenishment";

    try {
        inventoryService_->restock(id, qty, reason, "MANAGER-01");
        out << TablePrinter::green(">> Successfully restocked " + std::to_string(qty) + " units for Item ID " + std::to_string(id) + ".\n");
    } catch (const std::exception& e) {
        out << TablePrinter::red(">> Restock failed: " + std::string(e.what()) + "\n");
    }
}

void CliApp::handleWriteOff(std::istream& in, std::ostream& out) {
    out << "\n" << TablePrinter::bold(">> Stock Write-Off (Damage / Spoilage / Expiry)\n");
    out << "Enter Item ID: ";
    core::ItemId id = 0;
    if (!(in >> id)) {
        in.clear();
        in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }

    out << "Enter Quantity to write off: ";
    int qty = 0;
    if (!(in >> qty) || qty <= 0) {
        in.clear();
        in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        return;
    }
    in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    out << "Enter Reason (e.g. Expired batch, Water damage): ";
    std::string reason;
    std::getline(in, reason);
    if (reason.empty()) reason = "Damaged Goods Write-off";

    try {
        inventoryService_->writeOff(id, qty, reason, "AUDITOR-01");
        out << TablePrinter::green(">> Successfully wrote off " + std::to_string(qty) + " units for Item ID " + std::to_string(id) + ".\n");
    } catch (const std::exception& e) {
        out << TablePrinter::red(">> Write-off failed: " + std::string(e.what()) + "\n");
    }
}

void CliApp::handleVendors(std::ostream& out) const {
    auto vendors = vendorRepo_->findAll();
    out << "\n" << TablePrinter::bold(">> Registered Vendor Directory (") << vendors.size() << " vendors):\n";

    TablePrinter table;
    table.addColumn("ID", 4, Alignment::Right);
    table.addColumn("Vendor Name", 24, Alignment::Left);
    table.addColumn("Contact Email", 26, Alignment::Left);
    table.addColumn("Phone", 14, Alignment::Left);
    table.addColumn("Payment Terms", 14, Alignment::Left);
    table.addColumn("Status", 10, Alignment::Center);

    for (const auto& v : vendors) {
        table.addRow({
            std::to_string(v.getId()),
            v.getName(),
            v.getContactEmail(),
            v.getPhoneNumber(),
            v.getPaymentTerms(),
            v.isActive() ? TablePrinter::green("ACTIVE") : TablePrinter::dim("INACTIVE")
        });
    }

    table.print(out);
}

void CliApp::handleAnalytics(std::ostream& out) const {
    out << "\n" << TablePrinter::bold(">> Business Intelligence & Analytical Reports\n");

    // 1. Reorder alerts
    auto alerts = analyticsService_->generateReorderAlerts();
    out << "\n" << TablePrinter::yellow("--- Low-Stock Reorder Alerts (") << alerts.size() << " items below threshold) ---\n";
    if (alerts.empty()) {
        out << TablePrinter::green("All items have healthy stock levels above reorder thresholds.\n");
    } else {
        TablePrinter table;
        table.addColumn("SKU", 12, Alignment::Left);
        table.addColumn("Product Name", 22, Alignment::Left);
        table.addColumn("Current Stock", 14, Alignment::Right);
        table.addColumn("Threshold", 10, Alignment::Right);
        table.addColumn("Suggested Order", 16, Alignment::Right);
        table.addColumn("Vendor", 22, Alignment::Left);

        for (const auto& a : alerts) {
            table.addRow({
                a.sku,
                a.itemName,
                TablePrinter::red(std::to_string(a.currentStock)),
                std::to_string(a.reorderThreshold),
                TablePrinter::cyan(std::to_string(a.suggestedReorderQuantity)),
                a.vendorName
            });
        }
        table.print(out);
    }

    // 2. Revenue Summary
    auto rev = analyticsService_->generateDailyReport();
    out << "\n" << TablePrinter::bold("--- Today's Financial Summary ---") << "\n";
    out << "  Total Receipts Issued: " << rev.receiptCount << "\n"
        << "  Gross Subtotal       : " << core::Money::format(rev.grossSubtotal) << "\n"
        << "  Total Discounts      : " << core::Money::format(rev.totalDiscount) << "\n"
        << "  Total Tax Collected  : " << core::Money::format(rev.totalTax) << "\n"
        << "  Net Revenue Realized : " << TablePrinter::green(core::Money::format(rev.netGrandTotal)) << "\n"
        << "  Average Ticket Value : " << core::Money::format(rev.averageBasketValue) << "\n";

    // 3. Category Breakdown
    auto catPerf = analyticsService_->generateCategoryBreakdown();
    out << "\n" << TablePrinter::bold("--- Category Sales Performance ---") << "\n";
    if (catPerf.empty()) {
        out << "No sales recorded yet across categories.\n";
    } else {
        TablePrinter table;
        table.addColumn("Category", 18, Alignment::Left);
        table.addColumn("Units Sold", 12, Alignment::Right);
        table.addColumn("Gross Sales", 14, Alignment::Right);
        table.addColumn("Discounts", 12, Alignment::Right);
        table.addColumn("Net Sales", 14, Alignment::Right);

        for (const auto& cp : catPerf) {
            table.addRow({
                cp.categoryName,
                std::to_string(cp.totalUnitsSold),
                core::Money::format(cp.grossRevenue),
                core::Money::format(cp.discountGranted),
                TablePrinter::green(core::Money::format(cp.netRevenue))
            });
        }
        table.print(out);
    }
}

void CliApp::handleAuditLogs(std::ostream& out) const {
    auto logs = inventoryService_->getRecentAuditLogs(20);
    out << "\n" << TablePrinter::bold(">> Recent Inventory Audit Trail (Last 20 operations):\n");

    TablePrinter table;
    table.addColumn("ID", 4, Alignment::Right);
    table.addColumn("SKU", 12, Alignment::Left);
    table.addColumn("Action", 12, Alignment::Center);
    table.addColumn("Delta", 8, Alignment::Right);
    table.addColumn("Prev Qty", 10, Alignment::Right);
    table.addColumn("New Qty", 10, Alignment::Right);
    table.addColumn("Operator", 12, Alignment::Left);
    table.addColumn("Reason", 26, Alignment::Left);

    for (const auto& log : logs) {
        std::string deltaStr = (log.getQuantityDelta() > 0 ? "+" : "") + std::to_string(log.getQuantityDelta());
        std::string actionStr;
        if (log.getAction() == core::AuditAction::SALE) {
            actionStr = TablePrinter::cyan("SALE");
        } else if (log.getAction() == core::AuditAction::RESTOCK) {
            actionStr = TablePrinter::green("RESTOCK");
        } else if (log.getAction() == core::AuditAction::WRITE_OFF) {
            actionStr = TablePrinter::red("WRITE_OFF");
        } else {
            actionStr = TablePrinter::yellow("ADJUST");
        }

        table.addRow({
            std::to_string(log.getId()),
            log.getItemSku(),
            actionStr,
            deltaStr,
            std::to_string(log.getPreviousQuantity()),
            std::to_string(log.getNewQuantity()),
            log.getOperatorId(),
            log.getReason()
        });
    }

    table.print(out);
}

void CliApp::run(std::istream& in, std::ostream& out) {
    displayBanner(out);

    bool running = true;
    while (running) {
        displayMainMenu(out);
        int choice = 0;
        if (!(in >> choice)) {
            break;
        }

        switch (choice) {
            case 1:
                handleListInventory(out);
                break;
            case 2:
                handlePosBilling(in, out);
                break;
            case 3:
                handleRestock(in, out);
                break;
            case 4:
                handleWriteOff(in, out);
                break;
            case 5:
                handleVendors(out);
                break;
            case 6:
                handleAnalytics(out);
                break;
            case 7:
                handleAuditLogs(out);
                break;
            case 8:
                out << TablePrinter::cyan("\nExiting StoreForge POS. Goodbye!\n");
                running = false;
                break;
            default:
                out << TablePrinter::red("Invalid selection. Please choose between 1 and 8.\n");
                break;
        }
    }
}

} // namespace storeforge::cli
