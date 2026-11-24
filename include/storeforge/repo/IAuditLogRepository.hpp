#pragma once

#include <vector>
#include "storeforge/core/Types.hpp"
#include "storeforge/domain/InventoryAuditLog.hpp"

namespace storeforge::repo {

class IAuditLogRepository {
public:
    virtual ~IAuditLogRepository() = default;

    virtual core::AuditLogId save(const domain::InventoryAuditLog& log) = 0;
    [[nodiscard]] virtual std::vector<domain::InventoryAuditLog> findByItemId(core::ItemId itemId) = 0;
    [[nodiscard]] virtual std::vector<domain::InventoryAuditLog> findAll(size_t limit = 100) = 0;
};

} // namespace storeforge::repo
