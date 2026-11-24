#pragma once

#include <vector>
#include <optional>
#include "storeforge/core/Types.hpp"
#include "storeforge/domain/Vendor.hpp"

namespace storeforge::repo {

class IVendorRepository {
public:
    virtual ~IVendorRepository() = default;

    [[nodiscard]] virtual std::optional<domain::Vendor> findById(core::VendorId id) = 0;
    [[nodiscard]] virtual std::vector<domain::Vendor> findAll() = 0;
    [[nodiscard]] virtual std::vector<domain::Vendor> findActive() = 0;

    virtual core::VendorId save(const domain::Vendor& vendor) = 0;
    virtual void update(const domain::Vendor& vendor) = 0;
    virtual bool remove(core::VendorId id) = 0;
};

} // namespace storeforge::repo
