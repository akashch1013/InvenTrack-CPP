#pragma once

#include <string>
#include "storeforge/core/Types.hpp"
#include "storeforge/core/DateTime.hpp"

namespace storeforge::domain {

class InventoryAuditLog {
public:
    InventoryAuditLog() = default;
    InventoryAuditLog(core::AuditLogId id,
                      core::ItemId itemId,
                      std::string itemSku,
                      core::AuditAction action,
                      int quantityDelta,
                      int previousQuantity,
                      int newQuantity,
                      std::string reason = "",
                      std::string operatorId = "SYSTEM",
                      core::DateTime timestamp = core::DateTime::now());

    [[nodiscard]] core::AuditLogId getId() const noexcept { return id_; }
    [[nodiscard]] core::ItemId getItemId() const noexcept { return itemId_; }
    [[nodiscard]] const std::string& getItemSku() const noexcept { return itemSku_; }
    [[nodiscard]] core::AuditAction getAction() const noexcept { return action_; }
    [[nodiscard]] int getQuantityDelta() const noexcept { return quantityDelta_; }
    [[nodiscard]] int getPreviousQuantity() const noexcept { return previousQuantity_; }
    [[nodiscard]] int getNewQuantity() const noexcept { return newQuantity_; }
    [[nodiscard]] const std::string& getReason() const noexcept { return reason_; }
    [[nodiscard]] const std::string& getOperatorId() const noexcept { return operatorId_; }
    [[nodiscard]] const core::DateTime& getTimestamp() const noexcept { return timestamp_; }

    void setId(core::AuditLogId id) noexcept { id_ = id; }

    [[nodiscard]] std::string serializeJson() const;

private:
    core::AuditLogId id_{0};
    core::ItemId itemId_{0};
    std::string itemSku_;
    core::AuditAction action_{core::AuditAction::MANUAL_ADJUSTMENT};
    int quantityDelta_{0};
    int previousQuantity_{0};
    int newQuantity_{0};
    std::string reason_;
    std::string operatorId_{"SYSTEM"};
    core::DateTime timestamp_{core::DateTime::now()};
};

} // namespace storeforge::domain
