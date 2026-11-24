#include "storeforge/repo/PostgresRepositories.hpp"
#include "storeforge/core/Exceptions.hpp"
#include "storeforge/domain/StandardCommodity.hpp"
#include "storeforge/domain/PerishableGood.hpp"
#include "storeforge/domain/WarrantyItem.hpp"

namespace storeforge::repo {

// =================== PostgresItemRepository ===================

PostgresItemRepository::PostgresItemRepository(std::shared_ptr<db::ConnectionPool> pool)
    : pool_(std::move(pool)) {}

#if defined(STOREFORGE_HAS_LIBPQXX)
std::unique_ptr<domain::Item> PostgresItemRepository::mapRowToItem(const pqxx::row& row) {
    core::ItemId id = row["id"].as<core::ItemId>();
    core::SKU sku(row["sku"].as<std::string>());
    std::string name = row["name"].as<std::string>();
    std::string desc = row["description"].is_null() ? "" : row["description"].as<std::string>();
    double price = row["base_price"].as<double>();
    int stock = row["stock_quantity"].as<int>();
    int threshold = row["reorder_threshold"].as<int>();
    core::ItemCategory category = core::stringToCategory(row["category"].as<std::string>());
    core::VendorId vendorId = row["vendor_id"].as<core::VendorId>();
    std::string typeStr = row["item_type"].as<std::string>();

    std::unique_ptr<domain::Item> item;

    if (typeStr == "PERISHABLE") {
        std::string expStr = row["expiry_date"].is_null() ? core::DateTime::now().toDateString() : row["expiry_date"].as<std::string>();
        item = std::make_unique<domain::PerishableGood>(
            id, sku, name, price, stock, threshold, category, vendorId, core::DateTime::fromDateString(expStr)
        );
    } else if (typeStr == "WARRANTY") {
        std::string serial = row["serial_number"].is_null() ? "UNKNOWN-SN" : row["serial_number"].as<std::string>();
        int months = row["warranty_months"].is_null() ? 12 : row["warranty_months"].as<int>();
        double fee = row["warranty_fee"].is_null() ? 0.0 : row["warranty_fee"].as<double>();
        item = std::make_unique<domain::WarrantyItem>(
            id, sku, name, price, stock, threshold, category, vendorId, serial, months, fee
        );
    } else {
        item = std::make_unique<domain::StandardCommodity>(
            id, sku, name, price, stock, threshold, category, vendorId
        );
    }

    item->setDescription(desc);
    return item;
}

std::optional<std::unique_ptr<domain::Item>> PostgresItemRepository::findById(core::ItemId id) {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);
    auto res = tx.exec_params(
        "SELECT id, sku, name, description, base_price, stock_quantity, reorder_threshold, "
        "category, vendor_id, item_type, expiry_date, serial_number, warranty_months, warranty_fee "
        "FROM items WHERE id = $1", id);
    if (res.empty()) return std::nullopt;
    return mapRowToItem(res[0]);
}

std::optional<std::unique_ptr<domain::Item>> PostgresItemRepository::findBySku(const core::SKU& sku) {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);
    auto res = tx.exec_params(
        "SELECT id, sku, name, description, base_price, stock_quantity, reorder_threshold, "
        "category, vendor_id, item_type, expiry_date, serial_number, warranty_months, warranty_fee "
        "FROM items WHERE sku = $1", sku.value());
    if (res.empty()) return std::nullopt;
    return mapRowToItem(res[0]);
}

std::vector<std::unique_ptr<domain::Item>> PostgresItemRepository::findAll() {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);
    auto res = tx.exec(
        "SELECT id, sku, name, description, base_price, stock_quantity, reorder_threshold, "
        "category, vendor_id, item_type, expiry_date, serial_number, warranty_months, warranty_fee "
        "FROM items ORDER BY id ASC");
    std::vector<std::unique_ptr<domain::Item>> items;
    items.reserve(res.size());
    for (const auto& row : res) {
        items.push_back(mapRowToItem(row));
    }
    return items;
}

std::vector<std::unique_ptr<domain::Item>> PostgresItemRepository::findByCategory(core::ItemCategory category) {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);
    auto res = tx.exec_params(
        "SELECT id, sku, name, description, base_price, stock_quantity, reorder_threshold, "
        "category, vendor_id, item_type, expiry_date, serial_number, warranty_months, warranty_fee "
        "FROM items WHERE category = $1 ORDER BY id ASC", core::categoryToString(category));
    std::vector<std::unique_ptr<domain::Item>> items;
    for (const auto& row : res) {
        items.push_back(mapRowToItem(row));
    }
    return items;
}

std::vector<std::unique_ptr<domain::Item>> PostgresItemRepository::findLowStock() {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);
    auto res = tx.exec(
        "SELECT id, sku, name, description, base_price, stock_quantity, reorder_threshold, "
        "category, vendor_id, item_type, expiry_date, serial_number, warranty_months, warranty_fee "
        "FROM items WHERE stock_quantity <= reorder_threshold ORDER BY stock_quantity ASC");
    std::vector<std::unique_ptr<domain::Item>> items;
    for (const auto& row : res) {
        items.push_back(mapRowToItem(row));
    }
    return items;
}

core::ItemId PostgresItemRepository::save(const domain::Item& item) {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);

    std::string itemType = core::itemTypeToString(item.getType());
    std::string expiryDate = "";
    std::string serialNumber = "";
    int warrantyMonths = 0;
    double warrantyFee = 0.0;

    if (item.getType() == core::ItemType::PerishableGood) {
        const auto& pg = dynamic_cast<const domain::PerishableGood&>(item);
        expiryDate = pg.getExpirationDate().toDateString();
    } else if (item.getType() == core::ItemType::WarrantyItem) {
        const auto& wi = dynamic_cast<const domain::WarrantyItem&>(item);
        serialNumber = wi.getSerialNumber();
        warrantyMonths = wi.getWarrantyPeriodMonths();
        warrantyFee = wi.getWarrantyFee();
    }

    auto res = tx.exec_params(
        "INSERT INTO items (sku, name, description, base_price, stock_quantity, reorder_threshold, "
        "category, vendor_id, item_type, expiry_date, serial_number, warranty_months, warranty_fee) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, "
        "NULLIF($10, '')::DATE, NULLIF($11, ''), $12, $13) RETURNING id",
        item.getSku().value(), item.getName(), item.getDescription(), item.getBasePrice(),
        item.getStockQuantity(), item.getReorderThreshold(), core::categoryToString(item.getCategory()),
        item.getVendorId(), itemType, expiryDate, serialNumber, warrantyMonths, warrantyFee
    );
    core::ItemId id = res[0][0].as<core::ItemId>();
    tx.commit();
    return id;
}

void PostgresItemRepository::update(const domain::Item& item) {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);
    tx.exec_params(
        "UPDATE items SET sku = $1, name = $2, description = $3, base_price = $4, "
        "stock_quantity = $5, reorder_threshold = $6, category = $7, vendor_id = $8, updated_at = NOW() "
        "WHERE id = $9",
        item.getSku().value(), item.getName(), item.getDescription(), item.getBasePrice(),
        item.getStockQuantity(), item.getReorderThreshold(), core::categoryToString(item.getCategory()),
        item.getVendorId(), item.getId()
    );
    tx.commit();
}

bool PostgresItemRepository::remove(core::ItemId id) {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);
    auto res = tx.exec_params("DELETE FROM items WHERE id = $1", id);
    tx.commit();
    return res.affected_rows() > 0;
}

void PostgresItemRepository::updateStock(core::ItemId id, int newStock) {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);
    tx.exec_params("UPDATE items SET stock_quantity = $1, updated_at = NOW() WHERE id = $2", newStock, id);
    tx.commit();
}

#else
std::optional<std::unique_ptr<domain::Item>> PostgresItemRepository::findById(core::ItemId) { return std::nullopt; }
std::optional<std::unique_ptr<domain::Item>> PostgresItemRepository::findBySku(const core::SKU&) { return std::nullopt; }
std::vector<std::unique_ptr<domain::Item>> PostgresItemRepository::findAll() { return {}; }
std::vector<std::unique_ptr<domain::Item>> PostgresItemRepository::findByCategory(core::ItemCategory) { return {}; }
std::vector<std::unique_ptr<domain::Item>> PostgresItemRepository::findLowStock() { return {}; }
core::ItemId PostgresItemRepository::save(const domain::Item&) { throw core::DatabaseException("Postgres libpqxx disabled in build"); }
void PostgresItemRepository::update(const domain::Item&) {}
bool PostgresItemRepository::remove(core::ItemId) { return false; }
void PostgresItemRepository::updateStock(core::ItemId, int) {}
#endif

// =================== PostgresVendorRepository ===================

PostgresVendorRepository::PostgresVendorRepository(std::shared_ptr<db::ConnectionPool> pool)
    : pool_(std::move(pool)) {}

#if defined(STOREFORGE_HAS_LIBPQXX)
domain::Vendor PostgresVendorRepository::mapRowToVendor(const pqxx::row& row) {
    return domain::Vendor(
        row["id"].as<core::VendorId>(),
        row["name"].as<std::string>(),
        row["contact_email"].as<std::string>(),
        row["phone_number"].is_null() ? "" : row["phone_number"].as<std::string>(),
        row["address"].is_null() ? "" : row["address"].as<std::string>(),
        row["is_active"].as<bool>(),
        row["payment_terms"].is_null() ? "Net 30" : row["payment_terms"].as<std::string>()
    );
}

std::optional<domain::Vendor> PostgresVendorRepository::findById(core::VendorId id) {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);
    auto res = tx.exec_params("SELECT id, name, contact_email, phone_number, address, is_active, payment_terms FROM vendors WHERE id = $1", id);
    if (res.empty()) return std::nullopt;
    return mapRowToVendor(res[0]);
}

std::vector<domain::Vendor> PostgresVendorRepository::findAll() {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);
    auto res = tx.exec("SELECT id, name, contact_email, phone_number, address, is_active, payment_terms FROM vendors ORDER BY id ASC");
    std::vector<domain::Vendor> vendors;
    for (const auto& row : res) vendors.push_back(mapRowToVendor(row));
    return vendors;
}

std::vector<domain::Vendor> PostgresVendorRepository::findActive() {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);
    auto res = tx.exec("SELECT id, name, contact_email, phone_number, address, is_active, payment_terms FROM vendors WHERE is_active = TRUE ORDER BY id ASC");
    std::vector<domain::Vendor> vendors;
    for (const auto& row : res) vendors.push_back(mapRowToVendor(row));
    return vendors;
}

core::VendorId PostgresVendorRepository::save(const domain::Vendor& vendor) {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);
    auto res = tx.exec_params(
        "INSERT INTO vendors (name, contact_email, phone_number, address, is_active, payment_terms) "
        "VALUES ($1, $2, $3, $4, $5, $6) RETURNING id",
        vendor.getName(), vendor.getContactEmail(), vendor.getPhoneNumber(),
        vendor.getAddress(), vendor.isActive(), vendor.getPaymentTerms()
    );
    core::VendorId id = res[0][0].as<core::VendorId>();
    tx.commit();
    return id;
}

void PostgresVendorRepository::update(const domain::Vendor& vendor) {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);
    tx.exec_params(
        "UPDATE vendors SET name = $1, contact_email = $2, phone_number = $3, address = $4, is_active = $5, payment_terms = $6 "
        "WHERE id = $7",
        vendor.getName(), vendor.getContactEmail(), vendor.getPhoneNumber(),
        vendor.getAddress(), vendor.isActive(), vendor.getPaymentTerms(), vendor.getId()
    );
    tx.commit();
}

bool PostgresVendorRepository::remove(core::VendorId id) {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);
    auto res = tx.exec_params("DELETE FROM vendors WHERE id = $1", id);
    tx.commit();
    return res.affected_rows() > 0;
}

#else
std::optional<domain::Vendor> PostgresVendorRepository::findById(core::VendorId) { return std::nullopt; }
std::vector<domain::Vendor> PostgresVendorRepository::findAll() { return {}; }
std::vector<domain::Vendor> PostgresVendorRepository::findActive() { return {}; }
core::VendorId PostgresVendorRepository::save(const domain::Vendor&) { throw core::DatabaseException("Postgres libpqxx disabled in build"); }
void PostgresVendorRepository::update(const domain::Vendor&) {}
bool PostgresVendorRepository::remove(core::VendorId) { return false; }
#endif

// =================== PostgresReceiptRepository ===================

PostgresReceiptRepository::PostgresReceiptRepository(std::shared_ptr<db::ConnectionPool> pool)
    : pool_(std::move(pool)) {}

#if defined(STOREFORGE_HAS_LIBPQXX)
domain::CustomerReceipt PostgresReceiptRepository::mapRowToReceipt(pqxx::work& tx, const pqxx::row& row) {
    core::ReceiptId id = row["id"].as<core::ReceiptId>();
    std::string num = row["receipt_number"].as<std::string>();
    std::string cname = row["customer_name"].as<std::string>();
    std::string cphone = row["customer_phone"].is_null() ? "" : row["customer_phone"].as<std::string>();
    core::PaymentMethod method = core::stringToPaymentMethod(row["payment_method"].as<std::string>());
    std::string createdAtStr = row["created_at"].as<std::string>();

    domain::CustomerReceipt receipt(id, num, cname, cphone, method, core::DateTime::fromIsoString(createdAtStr));

    // Fetch line items
    auto lineRows = tx.exec_params(
        "SELECT id, receipt_id, item_id, sku, item_name, quantity, unit_base_price, discount_per_unit, tax_per_unit "
        "FROM receipt_line_items WHERE receipt_id = $1 ORDER BY id ASC", id);

    for (const auto& lr : lineRows) {
        domain::ReceiptLineItem line(
            lr["id"].as<core::LineItemId>(),
            lr["receipt_id"].as<core::ReceiptId>(),
            lr["item_id"].as<core::ItemId>(),
            core::SKU(lr["sku"].as<std::string>()),
            lr["item_name"].as<std::string>(),
            lr["quantity"].as<int>(),
            lr["unit_base_price"].as<double>(),
            lr["discount_per_unit"].as<double>(),
            lr["tax_per_unit"].as<double>()
        );
        receipt.addLineItem(line);
    }

    return receipt;
}

std::optional<domain::CustomerReceipt> PostgresReceiptRepository::findById(core::ReceiptId id) {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);
    auto res = tx.exec_params("SELECT id, receipt_number, customer_name, customer_phone, payment_method, created_at FROM customer_receipts WHERE id = $1", id);
    if (res.empty()) return std::nullopt;
    return mapRowToReceipt(tx, res[0]);
}

std::optional<domain::CustomerReceipt> PostgresReceiptRepository::findByReceiptNumber(const std::string& receiptNumber) {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);
    auto res = tx.exec_params("SELECT id, receipt_number, customer_name, customer_phone, payment_method, created_at FROM customer_receipts WHERE receipt_number = $1", receiptNumber);
    if (res.empty()) return std::nullopt;
    return mapRowToReceipt(tx, res[0]);
}

std::vector<domain::CustomerReceipt> PostgresReceiptRepository::findAll() {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);
    auto res = tx.exec("SELECT id, receipt_number, customer_name, customer_phone, payment_method, created_at FROM customer_receipts ORDER BY id DESC");
    std::vector<domain::CustomerReceipt> receipts;
    for (const auto& row : res) receipts.push_back(mapRowToReceipt(tx, row));
    return receipts;
}

std::vector<domain::CustomerReceipt> PostgresReceiptRepository::findByDateRange(const core::DateTime& start, const core::DateTime& end) {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);
    auto res = tx.exec_params(
        "SELECT id, receipt_number, customer_name, customer_phone, payment_method, created_at "
        "FROM customer_receipts WHERE created_at >= $1::TIMESTAMP AND created_at <= $2::TIMESTAMP ORDER BY id DESC",
        start.toIsoString(), end.toIsoString()
    );
    std::vector<domain::CustomerReceipt> receipts;
    for (const auto& row : res) receipts.push_back(mapRowToReceipt(tx, row));
    return receipts;
}

core::ReceiptId PostgresReceiptRepository::save(const domain::CustomerReceipt& receipt) {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);

    auto res = tx.exec_params(
        "INSERT INTO customer_receipts (receipt_number, customer_name, customer_phone, payment_method, subtotal, total_discount, total_tax, grand_total, created_at) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9::TIMESTAMP) RETURNING id",
        receipt.getReceiptNumber(), receipt.getCustomerName(), receipt.getCustomerPhone(),
        core::paymentMethodToString(receipt.getPaymentMethod()), receipt.getSubtotal(),
        receipt.getTotalDiscount(), receipt.getTotalTax(), receipt.getGrandTotal(),
        receipt.getCreatedAt().toIsoString()
    );

    core::ReceiptId receiptId = res[0][0].as<core::ReceiptId>();

    for (const auto& line : receipt.getLineItems()) {
        tx.exec_params(
            "INSERT INTO receipt_line_items (receipt_id, item_id, sku, item_name, quantity, unit_base_price, discount_per_unit, tax_per_unit, line_total) "
            "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9)",
            receiptId, line.getItemId(), line.getSku().value(), line.getItemName(),
            line.getQuantity(), line.getUnitBasePrice(), line.getDiscountPerUnit(),
            line.getTaxPerUnit(), line.getLineTotal()
        );
    }

    tx.commit();
    return receiptId;
}

#else
std::optional<domain::CustomerReceipt> PostgresReceiptRepository::findById(core::ReceiptId) { return std::nullopt; }
std::optional<domain::CustomerReceipt> PostgresReceiptRepository::findByReceiptNumber(const std::string&) { return std::nullopt; }
std::vector<domain::CustomerReceipt> PostgresReceiptRepository::findAll() { return {}; }
std::vector<domain::CustomerReceipt> PostgresReceiptRepository::findByDateRange(const core::DateTime&, const core::DateTime&) { return {}; }
core::ReceiptId PostgresReceiptRepository::save(const domain::CustomerReceipt&) { throw core::DatabaseException("Postgres libpqxx disabled in build"); }
#endif

// =================== PostgresAuditLogRepository ===================

PostgresAuditLogRepository::PostgresAuditLogRepository(std::shared_ptr<db::ConnectionPool> pool)
    : pool_(std::move(pool)) {}

#if defined(STOREFORGE_HAS_LIBPQXX)
domain::InventoryAuditLog PostgresAuditLogRepository::mapRowToAuditLog(const pqxx::row& row) {
    return domain::InventoryAuditLog(
        row["id"].as<core::AuditLogId>(),
        row["item_id"].as<core::ItemId>(),
        row["item_sku"].as<std::string>(),
        core::stringToAuditAction(row["action"].as<std::string>()),
        row["quantity_delta"].as<int>(),
        row["previous_quantity"].as<int>(),
        row["new_quantity"].as<int>(),
        row["reason"].is_null() ? "" : row["reason"].as<std::string>(),
        row["operator_id"].as<std::string>(),
        core::DateTime::fromIsoString(row["created_at"].as<std::string>())
    );
}

core::AuditLogId PostgresAuditLogRepository::save(const domain::InventoryAuditLog& log) {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);
    auto res = tx.exec_params(
        "INSERT INTO inventory_audit_logs (item_id, item_sku, action, quantity_delta, previous_quantity, new_quantity, reason, operator_id, created_at) "
        "VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9::TIMESTAMP) RETURNING id",
        log.getItemId(), log.getItemSku(), core::auditActionToString(log.getAction()),
        log.getQuantityDelta(), log.getPreviousQuantity(), log.getNewQuantity(),
        log.getReason(), log.getOperatorId(), log.getTimestamp().toIsoString()
    );
    core::AuditLogId id = res[0][0].as<core::AuditLogId>();
    tx.commit();
    return id;
}

std::vector<domain::InventoryAuditLog> PostgresAuditLogRepository::findByItemId(core::ItemId itemId) {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);
    auto res = tx.exec_params(
        "SELECT id, item_id, item_sku, action, quantity_delta, previous_quantity, new_quantity, reason, operator_id, created_at "
        "FROM inventory_audit_logs WHERE item_id = $1 ORDER BY id DESC", itemId);
    std::vector<domain::InventoryAuditLog> logs;
    for (const auto& row : res) logs.push_back(mapRowToAuditLog(row));
    return logs;
}

std::vector<domain::InventoryAuditLog> PostgresAuditLogRepository::findAll(size_t limit) {
    auto conn = pool_->acquire();
    pqxx::work tx(*conn);
    auto res = tx.exec_params(
        "SELECT id, item_id, item_sku, action, quantity_delta, previous_quantity, new_quantity, reason, operator_id, created_at "
        "FROM inventory_audit_logs ORDER BY id DESC LIMIT $1", limit);
    std::vector<domain::InventoryAuditLog> logs;
    for (const auto& row : res) logs.push_back(mapRowToAuditLog(row));
    return logs;
}

#else
core::AuditLogId PostgresAuditLogRepository::save(const domain::InventoryAuditLog&) { throw core::DatabaseException("Postgres libpqxx disabled in build"); }
std::vector<domain::InventoryAuditLog> PostgresAuditLogRepository::findByItemId(core::ItemId) { return {}; }
std::vector<domain::InventoryAuditLog> PostgresAuditLogRepository::findAll(size_t) { return {}; }
#endif

} // namespace storeforge::repo
