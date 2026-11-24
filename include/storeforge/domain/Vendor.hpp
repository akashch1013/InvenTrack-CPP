#pragma once

#include <string>
#include "storeforge/core/Types.hpp"
#include "storeforge/core/DateTime.hpp"

namespace storeforge::domain {

class Vendor {
public:
    Vendor() = default;
    Vendor(core::VendorId id,
           std::string name,
           std::string contactEmail,
           std::string phoneNumber,
           std::string address,
           bool isActive = true,
           std::string paymentTerms = "Net 30");

    [[nodiscard]] core::VendorId getId() const noexcept { return id_; }
    [[nodiscard]] const std::string& getName() const noexcept { return name_; }
    [[nodiscard]] const std::string& getContactEmail() const noexcept { return contactEmail_; }
    [[nodiscard]] const std::string& getPhoneNumber() const noexcept { return phoneNumber_; }
    [[nodiscard]] const std::string& getAddress() const noexcept { return address_; }
    [[nodiscard]] bool isActive() const noexcept { return isActive_; }
    [[nodiscard]] const std::string& getPaymentTerms() const noexcept { return paymentTerms_; }
    [[nodiscard]] const core::DateTime& getCreatedAt() const noexcept { return createdAt_; }

    void setId(core::VendorId id) noexcept { id_ = id; }
    void setName(std::string name);
    void setContactEmail(std::string email);
    void setPhoneNumber(std::string phone) { phoneNumber_ = std::move(phone); }
    void setAddress(std::string address) { address_ = std::move(address); }
    void setIsActive(bool active) noexcept { isActive_ = active; }
    void setPaymentTerms(std::string terms) { paymentTerms_ = std::move(terms); }

    [[nodiscard]] std::string serializeJson() const;

private:
    core::VendorId id_{0};
    std::string name_;
    std::string contactEmail_;
    std::string phoneNumber_;
    std::string address_;
    bool isActive_{true};
    std::string paymentTerms_{"Net 30"};
    core::DateTime createdAt_{core::DateTime::now()};
};

} // namespace storeforge::domain
