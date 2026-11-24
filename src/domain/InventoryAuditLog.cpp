#include "storeforge/domain/InventoryAuditLog.hpp"
#include <sstream>

namespace storeforge::domain {

InventoryAuditLog::InventoryAuditLog(core::AuditLogId id,
                                     core::ItemId itemId,
                                     std::string itemSku,
                                     core::AuditAction action,
                                     int quantityDelta,
                                     int previousQuantity,
                                     int newQuantity,
                                     std::string reason,
                                     std::string operatorId,
                                     core::DateTime timestamp)
    : id_(id),
      itemId_(itemId),
      itemSku_(std::move(itemSku)),
      action_(action),
      quantityDelta_(quantityDelta),
      previousQuantity_(previousQuantity),
      newQuantity_(newQuantity),
      reason_(std::move(reason)),
      operatorId_(std::move(operatorId)),
      timestamp_(timestamp) {}

std::string InventoryAuditLog::serializeJson() const {
    std::ostringstream ss;
    ss << "{"
       << "\"id\":" << id_ << ","
       << "\"item_id\":" << itemId_ << ","
       << "\"item_sku\":\"" << itemSku_ << "\","
       << "\"action\":\"" << core::auditActionToString(action_) << "\","
       << "\"quantity_delta\":" << quantityDelta_ << ","
       << "\"previous_quantity\":" << previousQuantity_ << ","
       << "\"new_quantity\":" << newQuantity_ << ","
       << "\"reason\":\"" << reason_ << "\","
       << "\"operator_id\":\"" << operatorId_ << "\","
       << "\"timestamp\":\"" << timestamp_.toIsoString() << "\""
       << "}";
    return ss.str();
}

} // namespace storeforge::domain
