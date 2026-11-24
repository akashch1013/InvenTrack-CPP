-- ============================================================================
-- StoreForge Retail Inventory and POS Billing System
-- Normalized PostgreSQL Database Schema (3NF)
-- Compatible with PostgreSQL 13+ and libpqxx
-- ============================================================================

-- Drop existing tables and views if re-running
DROP VIEW IF EXISTS view_perishable_expiry_watch CASCADE;
DROP VIEW IF EXISTS view_category_performance CASCADE;
DROP VIEW IF EXISTS view_daily_sales_summary CASCADE;
DROP VIEW IF EXISTS view_low_stock_reorder_alerts CASCADE;

DROP TABLE IF EXISTS inventory_audit_logs CASCADE;
DROP TABLE IF EXISTS receipt_line_items CASCADE;
DROP TABLE IF EXISTS customer_receipts CASCADE;
DROP TABLE IF EXISTS items CASCADE;
DROP TABLE IF EXISTS vendors CASCADE;

-- Extensions
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

-- ----------------------------------------------------------------------------
-- 1. Vendors Table
-- ----------------------------------------------------------------------------
CREATE TABLE vendors (
    id BIGSERIAL PRIMARY KEY,
    name VARCHAR(120) NOT NULL,
    contact_email VARCHAR(160) NOT NULL,
    phone_number VARCHAR(32),
    address TEXT,
    is_active BOOLEAN NOT NULL DEFAULT TRUE,
    payment_terms VARCHAR(64) DEFAULT 'Net 30',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),

    CONSTRAINT chk_vendor_name_nonempty CHECK (LENGTH(TRIM(name)) > 0),
    CONSTRAINT chk_vendor_email_format CHECK (contact_email ~* '^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}$')
);

CREATE INDEX idx_vendors_name ON vendors(name);
CREATE INDEX idx_vendors_active ON vendors(is_active);

-- ----------------------------------------------------------------------------
-- 2. Items Table (Polymorphic Domain Storage)
-- ----------------------------------------------------------------------------
CREATE TABLE items (
    id BIGSERIAL PRIMARY KEY,
    sku VARCHAR(64) NOT NULL UNIQUE,
    name VARCHAR(160) NOT NULL,
    description TEXT,
    base_price NUMERIC(12, 2) NOT NULL DEFAULT 0.00,
    stock_quantity INT NOT NULL DEFAULT 0,
    reorder_threshold INT NOT NULL DEFAULT 5,
    category VARCHAR(64) NOT NULL DEFAULT 'General',
    vendor_id BIGINT REFERENCES vendors(id) ON DELETE SET NULL,
    item_type VARCHAR(32) NOT NULL DEFAULT 'STANDARD',
    
    -- Specialized Polymorphic Subclass Attributes
    expiry_date DATE,                       -- Used by PerishableGood
    serial_number VARCHAR(128),              -- Used by WarrantyItem
    warranty_months INT DEFAULT 0,          -- Used by WarrantyItem
    warranty_fee NUMERIC(10, 2) DEFAULT 0.00,-- Used by WarrantyItem
    
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),

    CONSTRAINT chk_item_sku_nonempty CHECK (LENGTH(TRIM(sku)) > 0),
    CONSTRAINT chk_item_name_nonempty CHECK (LENGTH(TRIM(name)) > 0),
    CONSTRAINT chk_item_base_price CHECK (base_price >= 0.00),
    CONSTRAINT chk_item_stock_quantity CHECK (stock_quantity >= 0),
    CONSTRAINT chk_item_reorder_threshold CHECK (reorder_threshold >= 0),
    CONSTRAINT chk_item_type CHECK (item_type IN ('STANDARD', 'PERISHABLE', 'WARRANTY')),
    CONSTRAINT chk_perishable_has_expiry CHECK (
        (item_type = 'PERISHABLE' AND expiry_date IS NOT NULL) OR (item_type <> 'PERISHABLE')
    ),
    CONSTRAINT chk_warranty_has_serial CHECK (
        (item_type = 'WARRANTY' AND serial_number IS NOT NULL AND LENGTH(TRIM(serial_number)) > 0) OR (item_type <> 'WARRANTY')
    )
);

CREATE INDEX idx_items_sku ON items(sku);
CREATE INDEX idx_items_category ON items(category);
CREATE INDEX idx_items_vendor_id ON items(vendor_id);
CREATE INDEX idx_items_type ON items(item_type);
CREATE INDEX idx_items_low_stock ON items(stock_quantity) WHERE stock_quantity <= reorder_threshold;
CREATE INDEX idx_items_perishable_expiry ON items(expiry_date) WHERE item_type = 'PERISHABLE';

-- ----------------------------------------------------------------------------
-- 3. Customer Receipts Table (POS Transactions)
-- ----------------------------------------------------------------------------
CREATE TABLE customer_receipts (
    id BIGSERIAL PRIMARY KEY,
    receipt_number VARCHAR(64) NOT NULL UNIQUE,
    customer_name VARCHAR(120) NOT NULL DEFAULT 'Walk-in Customer',
    customer_phone VARCHAR(32),
    payment_method VARCHAR(32) NOT NULL DEFAULT 'CASH',
    subtotal NUMERIC(12, 2) NOT NULL DEFAULT 0.00,
    total_discount NUMERIC(12, 2) NOT NULL DEFAULT 0.00,
    total_tax NUMERIC(12, 2) NOT NULL DEFAULT 0.00,
    grand_total NUMERIC(12, 2) NOT NULL DEFAULT 0.00,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),

    CONSTRAINT chk_receipt_number_nonempty CHECK (LENGTH(TRIM(receipt_number)) > 0),
    CONSTRAINT chk_receipt_subtotal CHECK (subtotal >= 0.00),
    CONSTRAINT chk_receipt_total_discount CHECK (total_discount >= 0.00),
    CONSTRAINT chk_receipt_total_tax CHECK (total_tax >= 0.00),
    CONSTRAINT chk_receipt_grand_total CHECK (grand_total >= 0.00),
    CONSTRAINT chk_receipt_payment_method CHECK (payment_method IN ('CASH', 'CREDIT_CARD', 'DEBIT_CARD', 'UPI_DIGITAL', 'STORE_CREDIT'))
);

CREATE INDEX idx_customer_receipts_number ON customer_receipts(receipt_number);
CREATE INDEX idx_customer_receipts_created_at ON customer_receipts(created_at);

-- ----------------------------------------------------------------------------
-- 4. Receipt Line Items Table (Sales Breakdown)
-- ----------------------------------------------------------------------------
CREATE TABLE receipt_line_items (
    id BIGSERIAL PRIMARY KEY,
    receipt_id BIGINT NOT NULL REFERENCES customer_receipts(id) ON DELETE CASCADE,
    item_id BIGINT NOT NULL REFERENCES items(id) ON DELETE RESTRICT,
    sku VARCHAR(64) NOT NULL,
    item_name VARCHAR(160) NOT NULL,
    quantity INT NOT NULL,
    unit_base_price NUMERIC(12, 2) NOT NULL,
    discount_per_unit NUMERIC(12, 2) NOT NULL DEFAULT 0.00,
    tax_per_unit NUMERIC(12, 2) NOT NULL DEFAULT 0.00,
    line_total NUMERIC(12, 2) NOT NULL,

    CONSTRAINT chk_line_quantity CHECK (quantity > 0),
    CONSTRAINT chk_line_unit_price CHECK (unit_base_price >= 0.00),
    CONSTRAINT chk_line_discount CHECK (discount_per_unit >= 0.00),
    CONSTRAINT chk_line_tax CHECK (tax_per_unit >= 0.00),
    CONSTRAINT chk_line_total CHECK (line_total >= 0.00)
);

CREATE INDEX idx_line_items_receipt_id ON receipt_line_items(receipt_id);
CREATE INDEX idx_line_items_item_id ON receipt_line_items(item_id);
CREATE INDEX idx_line_items_sku ON receipt_line_items(sku);

-- ----------------------------------------------------------------------------
-- 5. Inventory Audit Logs Table (Full Traceability)
-- ----------------------------------------------------------------------------
CREATE TABLE inventory_audit_logs (
    id BIGSERIAL PRIMARY KEY,
    item_id BIGINT NOT NULL REFERENCES items(id) ON DELETE CASCADE,
    item_sku VARCHAR(64) NOT NULL,
    action VARCHAR(32) NOT NULL,
    quantity_delta INT NOT NULL,
    previous_quantity INT NOT NULL,
    new_quantity INT NOT NULL,
    reason TEXT,
    operator_id VARCHAR(64) NOT NULL DEFAULT 'SYSTEM',
    created_at TIMESTAMP WITH TIME ZONE DEFAULT NOW(),

    CONSTRAINT chk_audit_action CHECK (action IN ('RESTOCK', 'SALE', 'RETURN', 'WRITE_OFF', 'ADJUSTMENT'))
);

CREATE INDEX idx_audit_logs_item_id ON inventory_audit_logs(item_id);
CREATE INDEX idx_audit_logs_action ON inventory_audit_logs(action);
CREATE INDEX idx_audit_logs_created_at ON inventory_audit_logs(created_at);

-- ----------------------------------------------------------------------------
-- 6. Automated Updated At Trigger Function
-- ----------------------------------------------------------------------------
CREATE OR REPLACE FUNCTION update_timestamp_column()
RETURNS TRIGGER AS $$
BEGIN
    NEW.updated_at = NOW();
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER trg_items_updated_at
BEFORE UPDATE ON items
FOR EACH ROW EXECUTE PROCEDURE update_timestamp_column();

CREATE TRIGGER trg_vendors_updated_at
BEFORE UPDATE ON vendors
FOR EACH ROW EXECUTE PROCEDURE update_timestamp_column();

-- ============================================================================
-- Analytical Views (Business Intelligence & Reports)
-- ============================================================================

-- View 1: Reorder Alerts with Vendor contact info
CREATE VIEW view_low_stock_reorder_alerts AS
SELECT 
    i.id AS item_id,
    i.sku,
    i.name AS item_name,
    i.category,
    i.stock_quantity,
    i.reorder_threshold,
    GREATEST(20, i.reorder_threshold * 3) AS suggested_reorder_quantity,
    v.id AS vendor_id,
    COALESCE(v.name, 'Unassigned') AS vendor_name,
    COALESCE(v.contact_email, 'N/A') AS vendor_email,
    v.phone_number AS vendor_phone
FROM items i
LEFT JOIN vendors v ON i.vendor_id = v.id
WHERE i.stock_quantity <= i.reorder_threshold
ORDER BY i.stock_quantity ASC;

-- View 2: Daily Sales & Revenue Aggregation
CREATE VIEW view_daily_sales_summary AS
SELECT 
    DATE(created_at) AS sale_date,
    COUNT(id) AS total_receipts,
    SUM(subtotal) AS gross_subtotal,
    SUM(total_discount) AS total_discount,
    SUM(total_tax) AS total_tax,
    SUM(grand_total) AS net_revenue,
    ROUND(AVG(grand_total), 2) AS average_basket_value
FROM customer_receipts
GROUP BY DATE(created_at)
ORDER BY sale_date DESC;

-- View 3: Category Performance Breakdown
CREATE VIEW view_category_performance AS
SELECT 
    i.category,
    COUNT(DISTINCT rli.receipt_id) AS orders_count,
    SUM(rli.quantity) AS total_units_sold,
    SUM(rli.unit_base_price * rli.quantity) AS gross_sales,
    SUM(rli.discount_per_unit * rli.quantity) AS discounts_granted,
    SUM(rli.tax_per_unit * rli.quantity) AS taxes_collected,
    SUM(rli.line_total) AS net_revenue
FROM receipt_line_items rli
JOIN items i ON rli.item_id = i.id
GROUP BY i.category
ORDER BY net_revenue DESC;

-- View 4: Perishable Expiry Watch List (Near-Expiry & Expired)
CREATE VIEW view_perishable_expiry_watch AS
SELECT 
    i.id AS item_id,
    i.sku,
    i.name,
    i.stock_quantity,
    i.expiry_date,
    (i.expiry_date - CURRENT_DATE) AS days_remaining,
    CASE 
        WHEN i.expiry_date < CURRENT_DATE THEN 'EXPIRED'
        WHEN (i.expiry_date - CURRENT_DATE) <= 2 THEN 'CRITICAL (50% Markdown)'
        WHEN (i.expiry_date - CURRENT_DATE) <= 5 THEN 'NEAR EXPIRY (25% Markdown)'
        ELSE 'NORMAL'
    END AS markdown_tier
FROM items i
WHERE i.item_type = 'PERISHABLE'
  AND (i.expiry_date <= CURRENT_DATE + INTERVAL '7 days')
ORDER BY i.expiry_date ASC;
