# StoreForge C++17 API Reference

## 1. `storeforge::service::BillingService`

The core Point-of-Sale billing and atomic checkout engine.

### Header
```cpp
#include "storeforge/service/BillingService.hpp"
```

### Constructor
```cpp
BillingService(repo::IItemRepository& itemRepo,
               repo::IReceiptRepository& receiptRepo,
               repo::IAuditLogRepository& auditRepo);
```

### Methods
- `CartValidationResult validateCart(const CheckoutRequest& request) const`: Pre-validates cart items, checking SKU existence, sellability, and available inventory levels.
- `domain::CustomerReceipt checkout(const CheckoutRequest& request, double standardTaxRate = 0.08)`: Executes atomic checkout. Decrements stock, writes audit entries (`AuditAction::SALE`), generates receipt and persists to repository. Reverts all stock if any item fails validation.
- `std::string generateReceiptNumber() const`: Generates unique receipt identifier format `REC-YYYYMMDD-XXXX`.

---

## 2. `storeforge::service::InventoryService`

Handles product lifecycle, restocking shipments, and inventory write-offs.

### Header
```cpp
#include "storeforge/service/InventoryService.hpp"
```

### Methods
- `core::ItemId registerItem(const domain::Item& item)`: Ingests new item into the catalog and logs initial `RESTOCK` audit entry.
- `void updateItem(const domain::Item& item)`: Updates item metadata.
- `bool removeItem(core::ItemId id)`: Deletes an item from the repository.
- `void restock(core::ItemId itemId, int quantity, const std::string& reason, const std::string& operatorId)`: Increases stock and logs `AuditAction::RESTOCK`.
- `void writeOff(core::ItemId itemId, int quantity, const std::string& reason, const std::string& operatorId)`: Decreases damaged or expired stock and logs `AuditAction::WRITE_OFF`.
- `void adjustStock(core::ItemId itemId, int targetQuantity, const std::string& reason, const std::string& operatorId)`: Overwrites stock count to target value and records `AuditAction::MANUAL_ADJUSTMENT`.
- `std::vector<std::unique_ptr<domain::Item>> getLowStockItems() const`: Returns items where `stock <= reorderThreshold`.
- `std::vector<domain::InventoryAuditLog> getItemAuditTrail(core::ItemId itemId) const`: Retrieves complete chronological audit trail for a specific product.

---

## 3. `storeforge::service::AnalyticsService`

Generates business intelligence, financial summaries, and inventory replenishment alerts.

### Header
```cpp
#include "storeforge/service/AnalyticsService.hpp"
```

### Methods
- `std::vector<ReorderAlert> generateReorderAlerts() const`: Identifies items below threshold and includes vendor contact information.
- `RevenueSummary generateDailyReport(const core::DateTime& date = core::DateTime::now()) const`: Aggregates today's gross sales, tax, discounts, and order counts.
- `RevenueSummary generateMonthlyReport(int year, int month) const`: Calculates monthly accounting aggregates.
- `std::vector<CategoryPerformance> generateCategoryBreakdown() const`: Evaluates sales and margins across all retail product categories.
- `std::vector<ExpiryAlert> generateExpiryAlerts(int thresholdDays = 7) const`: Scans perishable inventory for goods expiring within the specified threshold.

---

## 4. `storeforge::db::ConnectionPool`

Thread-safe connection pool for PostgreSQL connections via `libpqxx`.

### Header
```cpp
#include "storeforge/db/ConnectionPool.hpp"
```

### Methods
- `ConnectionPool(Config config)`: Constructs pool with host, port, dbname, user, password, min/max connections.
- `void initialize()`: Establishes initial minimum connection quota.
- `ConnectionPtr acquire()`: Leases an open connection. Returns RAII `std::shared_ptr<pqxx::connection>` that returns the connection to the pool when destructed.
- `void shutdown()`: Closes all pooled connections safely via RAII.
