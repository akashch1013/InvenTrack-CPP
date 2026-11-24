#pragma once

#include <string>
#include <memory>
#include "storeforge/core/Types.hpp"
#include "storeforge/core/DateTime.hpp"

namespace storeforge::domain {

class Item {
public:
    virtual ~Item() = default;

    // Polymorphic core behaviors
    [[nodiscard]] virtual core::ItemType getType() const noexcept = 0;
    [[nodiscard]] virtual double calculateTax(double standardTaxRate) const = 0;
    [[nodiscard]] virtual double calculateDiscount(int quantity) const = 0;
    [[nodiscard]] virtual double getEffectivePrice(int quantity) const;
    [[nodiscard]] virtual bool isSellable() const = 0;
    [[nodiscard]] virtual std::string serializeJson() const = 0;
    [[nodiscard]] virtual std::unique_ptr<Item> clone() const = 0;

    // Getters
    [[nodiscard]] core::ItemId getId() const noexcept { return id_; }
    [[nodiscard]] const core::SKU& getSku() const noexcept { return sku_; }
    [[nodiscard]] const std::string& getName() const noexcept { return name_; }
    [[nodiscard]] const std::string& getDescription() const noexcept { return description_; }
    [[nodiscard]] double getBasePrice() const noexcept { return basePrice_; }
    [[nodiscard]] int getStockQuantity() const noexcept { return stockQuantity_; }
    [[nodiscard]] int getReorderThreshold() const noexcept { return reorderThreshold_; }
    [[nodiscard]] core::ItemCategory getCategory() const noexcept { return category_; }
    [[nodiscard]] core::VendorId getVendorId() const noexcept { return vendorId_; }
    [[nodiscard]] const core::DateTime& getCreatedAt() const noexcept { return createdAt_; }
    [[nodiscard]] const core::DateTime& getUpdatedAt() const noexcept { return updatedAt_; }

    [[nodiscard]] bool isLowStock() const noexcept { return stockQuantity_ <= reorderThreshold_; }

    // Setters with validation
    void setId(core::ItemId id) noexcept { id_ = id; }
    void setSku(const core::SKU& sku);
    void setName(std::string name);
    void setDescription(std::string desc) { description_ = std::move(desc); }
    void setBasePrice(double price);
    void setStockQuantity(int qty);
    void setReorderThreshold(int threshold);
    void setCategory(core::ItemCategory category) noexcept { category_ = category; }
    void setVendorId(core::VendorId vendorId) noexcept { vendorId_ = vendorId; }
    void setCreatedAt(const core::DateTime& dt) noexcept { createdAt_ = dt; }
    void setUpdatedAt(const core::DateTime& dt) noexcept { updatedAt_ = dt; }

    // Inventory mutations
    void deductStock(int quantity);
    void addStock(int quantity);

protected:
    Item(core::ItemId id,
         core::SKU sku,
         std::string name,
         double basePrice,
         int stockQuantity,
         int reorderThreshold,
         core::ItemCategory category,
         core::VendorId vendorId);

    core::ItemId id_{0};
    core::SKU sku_;
    std::string name_;
    std::string description_;
    double basePrice_{0.0};
    int stockQuantity_{0};
    int reorderThreshold_{5};
    core::ItemCategory category_{core::ItemCategory::General};
    core::VendorId vendorId_{0};
    core::DateTime createdAt_{core::DateTime::now()};
    core::DateTime updatedAt_{core::DateTime::now()};
};

} // namespace storeforge::domain
