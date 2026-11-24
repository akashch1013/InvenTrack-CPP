#pragma once

#include <vector>
#include <optional>
#include "storeforge/core/Types.hpp"
#include "storeforge/core/DateTime.hpp"
#include "storeforge/domain/CustomerReceipt.hpp"

namespace storeforge::repo {

class IReceiptRepository {
public:
    virtual ~IReceiptRepository() = default;

    [[nodiscard]] virtual std::optional<domain::CustomerReceipt> findById(core::ReceiptId id) = 0;
    [[nodiscard]] virtual std::optional<domain::CustomerReceipt> findByReceiptNumber(const std::string& receiptNumber) = 0;
    [[nodiscard]] virtual std::vector<domain::CustomerReceipt> findAll() = 0;
    [[nodiscard]] virtual std::vector<domain::CustomerReceipt> findByDateRange(const core::DateTime& start, const core::DateTime& end) = 0;

    virtual core::ReceiptId save(const domain::CustomerReceipt& receipt) = 0;
};

} // namespace storeforge::repo
