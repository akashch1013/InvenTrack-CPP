-- ============================================================================
-- StoreForge Retail Inventory and POS Billing System
-- Realistic Sample Seed Data
-- ============================================================================

-- Clean existing seed data
TRUNCATE TABLE receipt_line_items, customer_receipts, inventory_audit_logs, items, vendors RESTART IDENTITY CASCADE;

-- ----------------------------------------------------------------------------
-- 1. Vendors Seed Data
-- ----------------------------------------------------------------------------
INSERT INTO vendors (name, contact_email, phone_number, address, is_active, payment_terms) VALUES
('FreshFarms Agro Ltd', 'orders@freshfarms.com', '+1-800-555-0199', '45 Agri Way, Salinas, CA 93901', TRUE, 'Net 15'),
('Apex Tech Electronics Corp', 'supply@apextech.io', '+1-888-555-4321', '101 Silicon Valley Blvd, San Jose, CA 95110', TRUE, 'Net 30'),
('Evergreen Essentials Co', 'sales@evergreen.com', '+1-800-555-7788', '12 Industrial Blvd, Austin, TX 78701', TRUE, 'Net 30'),
('Nordic Dairy & Provisions', 'wholesale@nordicdairy.dk', '+1-800-555-9012', '78 Pasture Lane, Green Bay, WI 54301', TRUE, 'Net 15'),
('Global Apparel & Textile Ltd', 'accounts@globalapparel.com', '+1-877-555-6677', '500 Garment District, New York, NY 10018', TRUE, 'Net 45');

-- ----------------------------------------------------------------------------
-- 2. Items Seed Data (Polymorphic mix: Standard, Perishable, Warranty)
-- ----------------------------------------------------------------------------

-- Standard Commodities
INSERT INTO items (sku, name, description, base_price, stock_quantity, reorder_threshold, category, vendor_id, item_type) VALUES
('COMM-PAP-01', 'Multi-purpose Copy Paper 500 Sheets', 'High-grade 20lb white paper for everyday printing', 7.99, 120, 25, 'General', 3, 'STANDARD'),
('COMM-PEN-02', 'Gel Pen 12-Pack Black 0.7mm', 'Smooth liquid gel rollerball pens with comfort grip', 12.50, 45, 10, 'General', 3, 'STANDARD'),
('COMM-BAT-03', 'Alkaline AA Batteries 16-Pack', 'Long-lasting 1.5V power for household electronics', 15.99, 85, 20, 'Electronics', 2, 'STANDARD'),
('COMM-FOL-04', 'Heavy Duty Expanding File Folder', 'Letter-size 13-pocket poly organizer with flap', 9.49, 15, 20, 'General', 3, 'STANDARD'), -- Low stock
('COMM-TEE-05', 'Premium Cotton Crew-neck T-Shirt', 'Pre-shrunk 100% combed ringspun cotton jersey', 19.99, 60, 15, 'Apparel', 5, 'STANDARD');

-- Perishable Goods (Groceries with dynamic expiry dates)
INSERT INTO items (sku, name, description, base_price, stock_quantity, reorder_threshold, category, vendor_id, item_type, expiry_date) VALUES
('PERI-MLK-01', 'Organic Whole Milk 1 Gallon', 'Pasteurized grade-A organic whole milk', 4.99, 18, 10, 'Grocery', 4, 'PERISHABLE', CURRENT_DATE + INTERVAL '1 day'), -- 50% markdown tier
('PERI-EGG-02', 'Free-Range Large Brown Eggs 12ct', 'Farm fresh pasture-raised grade AA eggs', 5.49, 34, 12, 'Grocery', 1, 'PERISHABLE', CURRENT_DATE + INTERVAL '14 days'), -- Fresh
('PERI-BRD-03', 'Artisan Sourdough Boule Loaf', 'Naturally fermented sourdough baked fresh daily', 3.89, 4, 10, 'Grocery', 1, 'PERISHABLE', CURRENT_DATE + INTERVAL '3 days'), -- 25% markdown + low stock
('PERI-YGT-04', 'Greek Style Yogurt Plain 32oz', 'Traditional strained probiotic whole milk yogurt', 6.29, 22, 8, 'Grocery', 4, 'PERISHABLE', CURRENT_DATE + INTERVAL '5 days'), -- 25% markdown
('PERI-BER-05', 'Fresh Organic Strawberries 1lb', 'California sweet ripe organic strawberries', 4.49, 12, 15, 'Grocery', 1, 'PERISHABLE', CURRENT_DATE + INTERVAL '2 days'), -- 50% markdown + low stock
('PERI-CHE-06', 'Aged White Cheddar Cheese 8oz', 'Sharp Vermont cheddar block aged 12 months', 7.99, 25, 10, 'Grocery', 4, 'PERISHABLE', CURRENT_DATE + INTERVAL '45 days');

-- Warranty Items (Electronics with serial tracking and warranty)
INSERT INTO items (sku, name, description, base_price, stock_quantity, reorder_threshold, category, vendor_id, item_type, serial_number, warranty_months, warranty_fee) VALUES
('ELEC-LPT-01', 'ForgePro 15-inch Laptop i7/16GB', 'Enterprise aluminum ultrabook with FHD display', 1199.99, 8, 3, 'Electronics', 2, 'WARRANTY', 'SN-FP15-998822', 24, 49.99),
('ELEC-MOU-02', 'ErgoFit Wireless Mouse Bluetooth', 'Precision optical sensor with rechargeable battery', 49.99, 35, 8, 'Electronics', 2, 'WARRANTY', 'SN-EF-554411', 12, 5.00),
('ELEC-KBD-03', 'Mechanical Backlit Keyboard RGB', 'Hot-swappable tactile switches with braided cable', 89.99, 14, 5, 'Electronics', 2, 'WARRANTY', 'SN-KB-773322', 24, 9.99),
('ELEC-MON-04', 'UltraSharp 27-inch 4K IPS Monitor', 'HDR400 factory calibrated sRGB professional monitor', 349.99, 2, 4, 'Electronics', 2, 'WARRANTY', 'SN-US27-001144', 36, 29.99); -- Low stock

-- ----------------------------------------------------------------------------
-- 3. Initial Inventory Audit Logs
-- ----------------------------------------------------------------------------
INSERT INTO inventory_audit_logs (item_id, item_sku, action, quantity_delta, previous_quantity, new_quantity, reason, operator_id, created_at) VALUES
(1, 'COMM-PAP-01', 'RESTOCK', 120, 0, 120, 'Initial bulk stocking', 'SYSTEM', NOW() - INTERVAL '5 days'),
(2, 'COMM-PEN-02', 'RESTOCK', 45, 0, 45, 'Initial bulk stocking', 'SYSTEM', NOW() - INTERVAL '5 days'),
(3, 'COMM-BAT-03', 'RESTOCK', 85, 0, 85, 'Initial bulk stocking', 'SYSTEM', NOW() - INTERVAL '5 days'),
(4, 'COMM-FOL-04', 'RESTOCK', 15, 0, 15, 'Supplier partial shipment', 'SYSTEM', NOW() - INTERVAL '4 days'),
(5, 'COMM-TEE-05', 'RESTOCK', 60, 0, 60, 'Initial seasonal stocking', 'SYSTEM', NOW() - INTERVAL '4 days'),
(6, 'PERI-MLK-01', 'RESTOCK', 24, 0, 24, 'Dairy delivery batch #901', 'SYSTEM', NOW() - INTERVAL '2 days'),
(7, 'PERI-EGG-02', 'RESTOCK', 36, 0, 36, 'Farm delivery batch #402', 'SYSTEM', NOW() - INTERVAL '2 days'),
(8, 'PERI-BRD-03', 'RESTOCK', 10, 0, 10, 'Morning bakery delivery', 'SYSTEM', NOW() - INTERVAL '1 day'),
(12, 'ELEC-LPT-01', 'RESTOCK', 10, 0, 10, 'Warehouse tech intake', 'SYSTEM', NOW() - INTERVAL '7 days'),
(13, 'ELEC-MOU-02', 'RESTOCK', 40, 0, 40, 'Warehouse tech intake', 'SYSTEM', NOW() - INTERVAL '7 days'),
(15, 'ELEC-MON-04', 'RESTOCK', 5, 0, 5, 'High-end display intake', 'SYSTEM', NOW() - INTERVAL '7 days');

-- ----------------------------------------------------------------------------
-- 4. Historical Customer Receipts
-- ----------------------------------------------------------------------------
INSERT INTO customer_receipts (receipt_number, customer_name, customer_phone, payment_method, subtotal, total_discount, total_tax, grand_total, created_at) VALUES
('REC-20261120-1001', 'Alice Johnson', '+1-555-0101', 'CREDIT_CARD', 1249.98, 0.00, 155.99, 1405.97, NOW() - INTERVAL '3 days'),
('REC-20261121-1002', 'Robert Chen', '+1-555-0202', 'DEBIT_CARD', 32.97, 2.50, 2.44, 32.91, NOW() - INTERVAL '2 days'),
('REC-20261122-1003', 'David Miller', '+1-555-0303', 'UPI_DIGITAL', 388.87, 0.00, 46.66, 435.53, NOW() - INTERVAL '1 day'),
('REC-20261124-1004', 'Elena Rostova', '+1-555-0404', 'CASH', 28.46, 5.00, 1.88, 25.34, NOW() - INTERVAL '3 hours');

-- ----------------------------------------------------------------------------
-- 5. Receipt Line Items
-- ----------------------------------------------------------------------------
-- For Alice: 1 Laptop + 1 Mouse
INSERT INTO receipt_line_items (receipt_id, item_id, sku, item_name, quantity, unit_base_price, discount_per_unit, tax_per_unit, line_total) VALUES
(1, 12, 'ELEC-LPT-01', 'ForgePro 15-inch Laptop i7/16GB', 1, 1199.99, 0.00, 149.99, 1349.98),
(1, 13, 'ELEC-MOU-02', 'ErgoFit Wireless Mouse Bluetooth', 1, 49.99, 0.00, 6.00, 55.99);

-- For Robert: Paper + Pens + Milk
INSERT INTO receipt_line_items (receipt_id, item_id, sku, item_name, quantity, unit_base_price, discount_per_unit, tax_per_unit, line_total) VALUES
(2, 1, 'COMM-PAP-01', 'Multi-purpose Copy Paper 500 Sheets', 1, 7.99, 0.00, 0.64, 8.63),
(2, 2, 'COMM-PEN-02', 'Gel Pen 12-Pack Black 0.7mm', 1, 12.50, 0.00, 1.00, 13.50),
(2, 6, 'PERI-MLK-01', 'Organic Whole Milk 1 Gallon', 2, 4.99, 1.25, 0.10, 7.68);

-- For David: Monitor + Keyboard
INSERT INTO receipt_line_items (receipt_id, item_id, sku, item_name, quantity, unit_base_price, discount_per_unit, tax_per_unit, line_total) VALUES
(3, 15, 'ELEC-MON-04', 'UltraSharp 27-inch 4K IPS Monitor', 1, 349.99, 0.00, 42.00, 391.99),
(3, 14, 'ELEC-KBD-03', 'Mechanical Backlit Keyboard RGB', 1, 89.99, 0.00, 10.80, 100.79);

-- For Elena: Bread + Strawberries + Eggs
INSERT INTO receipt_line_items (receipt_id, item_id, sku, item_name, quantity, unit_base_price, discount_per_unit, tax_per_unit, line_total) VALUES
(4, 8, 'PERI-BRD-03', 'Artisan Sourdough Boule Loaf', 2, 3.89, 0.97, 0.08, 6.00),
(4, 10, 'PERI-BER-05', 'Fresh Organic Strawberries 1lb', 2, 4.49, 2.25, 0.09, 4.66),
(4, 7, 'PERI-EGG-02', 'Free-Range Large Brown Eggs 12ct', 2, 5.49, 0.00, 0.11, 11.20);
