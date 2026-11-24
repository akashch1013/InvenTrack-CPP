#include "storeforge/domain/Vendor.hpp"
#include "storeforge/core/Exceptions.hpp"
#include <sstream>

namespace storeforge::domain {

Vendor::Vendor(core::VendorId id,
               std::string name,
               std::string contactEmail,
               std::string phoneNumber,
               std::string address,
               bool isActive,
               std::string paymentTerms)
    : id_(id),
      name_(std::move(name)),
      contactEmail_(std::move(contactEmail)),
      phoneNumber_(std::move(phoneNumber)),
      address_(std::move(address)),
      isActive_(isActive),
      paymentTerms_(std::move(paymentTerms)),
      createdAt_(core::DateTime::now()) {
    if (name_.empty()) {
        throw core::DomainValidationException("Vendor name cannot be empty");
    }
    if (contactEmail_.empty() || contactEmail_.find('@') == std::string::npos) {
        throw core::DomainValidationException("Vendor must have a valid contact email");
    }
}

void Vendor::setName(std::string name) {
    if (name.empty()) {
        throw core::DomainValidationException("Vendor name cannot be empty");
    }
    name_ = std::move(name);
}

void Vendor::setContactEmail(std::string email) {
    if (email.empty() || email.find('@') == std::string::npos) {
        throw core::DomainValidationException("Vendor must have a valid contact email");
    }
    contactEmail_ = std::move(email);
}

std::string Vendor::serializeJson() const {
    std::ostringstream ss;
    ss << "{"
       << "\"id\":" << id_ << ","
       << "\"name\":\"" << name_ << "\","
       << "\"contact_email\":\"" << contactEmail_ << "\","
       << "\"phone_number\":\"" << phoneNumber_ << "\","
       << "\"address\":\"" << address_ << "\","
       << "\"is_active\":" << (isActive_ ? "true" : "false") << ","
       << "\"payment_terms\":\"" << paymentTerms_ << "\""
       << "}";
    return ss.str();
}

} // namespace storeforge::domain
