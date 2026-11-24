# StoreForge Domain Models & OOP Architecture

## 1. Domain Model Hierarchy

StoreForge employs object-oriented polymorphism to model heterogeneous retail catalog merchandise.

```
                      +-----------------------------+
                      |     domain::Item (Abstract) |
                      +-----------------------------+
                      | + calculateTax()            |
                      | + calculateDiscount()       |
                      | + getEffectivePrice()       |
                      | + isSellable()              |
                      | + serializeJson()           |
                      | + clone()                   |
                      +--------------+--------------+
                                     |
         +---------------------------+---------------------------+
         |                                                       |
+------------------------------+                       +------------------------------+
|   StandardCommodity          |                       |       PerishableGood         |
+------------------------------+                       +------------------------------+
| - customTaxRate_             |                       | - expirationDate_            |
| + calculateDiscount()        |                       | - reducedTaxRate_            |
| + calculateTax()             |                       | + getMarkdownPercentage()    |
|                              |                       | + isExpired()                |
+------------------------------+                       +------------------------------+
                                                               |
                                                       +------------------------------+
                                                       |         WarrantyItem         |
                                                       +------------------------------+
                                                       | - serialNumber_              |
                                                       | - warrantyPeriodMonths_      |
                                                       | - warrantyFee_               |
                                                       | + calculateTax()             |
                                                       +------------------------------+
```

---

## 2. Specialized Item Subclasses

### 2.1 `StandardCommodity`
Represents standard retail inventory (e.g. stationery, household items, apparel).
- **Tax Policy**: Applies standard retail VAT (default 8% or customized per product).
- **Volume Tier Discount**:
  - Buying $\ge 10$ units: 10% unit price discount.
  - Buying $\ge 5$ units: 5% unit price discount.
  - Buying $< 5$ units: 0% discount.

### 2.2 `PerishableGood`
Represents grocery goods, dairy, produce, and baked products subject to time decay.
- **Tax Policy**: Applies grocery reduced tax bracket (default 2%).
- **Dynamic Markdown Engine**:
  - `daysUntilExpiration <= 0`: Marked as **EXPIRED**. Unsellable (`isSellable() == false`). Attempted checkout raises `ExpiredProductException`.
  - `daysUntilExpiration <= 2`: **50% Clearance Markdown**.
  - `daysUntilExpiration <= 5`: **25% Markdown**.
  - `daysUntilExpiration <= 7`: **10% Markdown**.
  - `daysUntilExpiration > 7`: Standard price.

### 2.3 `WarrantyItem`
Represents consumer electronics, appliances, and high-value equipment requiring individual tracking.
- **Serial Tracking**: Must possess a non-empty unique factory serial number (`serial_number`).
- **Warranty Terms**: Tracks warranty duration (e.g. 12, 24, 36 months).
- **Protection Fee Surcharge**: Calculates taxation over base price plus optional protection fee surcharges.

---

## 3. Strongly Typed Domain Primitives

StoreForge avoids primitive obsession by wrapping fundamental retail identifiers in strong types:

- **`core::SKU`**: Stock Keeping Unit strong wrapper providing equality and hash operations.
- **`core::Money`**: Monetary utility handling 2-decimal currency rounding and symbol formatting.
- **`core::DateTime`**: ISO 8601 parser/formatter with duration arithmetic (`daysUntil`, `addDays`).
- **Enumerations**:
  - `ItemCategory`: `Grocery`, `Electronics`, `Apparel`, `HomeAndKitchen`, `Healthcare`, `Automotive`, `General`.
  - `ItemType`: `StandardCommodity`, `PerishableGood`, `WarrantyItem`.
  - `PaymentMethod`: `Cash`, `CreditCard`, `DebitCard`, `UPI_Digital`, `StoreCredit`.
  - `AuditAction`: `RESTOCK`, `SALE`, `RETURN`, `WRITE_OFF`, `MANUAL_ADJUSTMENT`.

---

## 4. Domain Exception Hierarchy

All domain violations inherit from `core::StoreForgeException`:

```
std::exception
  └── std::runtime_error
        └── StoreForgeException
              ├── DomainValidationException   (Invalid fields, negative prices/stock)
              ├── EntityNotFoundException     (Missing SKU, item ID, or vendor ID)
              ├── DuplicateEntityException    (SKU collision during registration)
              ├── InsufficientStockException  (Checkout requested exceeds available stock)
              ├── ExpiredProductException     (Attempted sale of past-expiration goods)
              ├── TransactionException        (ACID checkout failure / rollback)
              └── DatabaseException           (libpqxx connectivity or query error)
```
