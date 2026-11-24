#pragma once

#include <vector>
#include <memory>
#include <optional>
#include "storeforge/core/Types.hpp"
#include "storeforge/domain/Item.hpp"

namespace storeforge::repo {

class IItemRepository {
public:
    virtual ~IItemRepository() = default;

    [[nodiscard]] virtual std::optional<std::unique_ptr<domain::Item>> findById(core::ItemId id) = 0;
    [[nodiscard]] virtual std::optional<std::unique_ptr<domain::Item>> findBySku(const core::SKU& sku) = 0;
    [[nodiscard]] virtual std::vector<std::unique_ptr<domain::Item>> findAll() = 0;
    [[nodiscard]] virtual std::vector<std::unique_ptr<domain::Item>> findByCategory(core::ItemCategory category) = 0;
    [[nodiscard]] virtual std::vector<std::unique_ptr<domain::Item>> findLowStock() = 0;

    virtual core::ItemId save(const domain::Item& item) = 0;
    virtual void update(const domain::Item& item) = 0;
    virtual bool remove(core::ItemId id) = 0;
    virtual void updateStock(core::ItemId id, int newStock) = 0;
};

} // namespace storeforge::repo
