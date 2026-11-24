#pragma once

#include <stdexcept>
#include <string>
#include "storeforge/core/Types.hpp"

namespace storeforge::core {

class StoreForgeException : public std::runtime_error {
public:
    explicit StoreForgeException(const std::string& message)
        : std::runtime_error(message) {}
};

class DomainValidationException : public StoreForgeException {
public:
    explicit DomainValidationException(const std::string& message)
        : StoreForgeException("Domain Validation Error: " + message) {}
};

class EntityNotFoundException : public StoreForgeException {
public:
    explicit EntityNotFoundException(const std::string& entityType, int64_t id)
        : StoreForgeException("Entity Not Found: " + entityType + " with ID=" + std::to_string(id)),
          entityType_(entityType), id_(id) {}

    explicit EntityNotFoundException(const std::string& entityType, const std::string& identifier)
        : StoreForgeException("Entity Not Found: " + entityType + " with identifier=" + identifier),
          entityType_(entityType), id_(0) {}

    [[nodiscard]] const std::string& getEntityType() const noexcept { return entityType_; }
    [[nodiscard]] int64_t getId() const noexcept { return id_; }

private:
    std::string entityType_;
    int64_t id_;
};

class DuplicateEntityException : public StoreForgeException {
public:
    explicit DuplicateEntityException(const std::string& entityType, const std::string& key)
        : StoreForgeException("Duplicate Entity: " + entityType + " with key '" + key + "' already exists") {}
};

class InsufficientStockException : public StoreForgeException {
public:
    InsufficientStockException(const std::string& sku, int requested, int available)
        : StoreForgeException("Insufficient Stock for SKU " + sku + 
                              ": Requested " + std::to_string(requested) + 
                              ", Available " + std::to_string(available)),
          sku_(sku), requested_(requested), available_(available) {}

    [[nodiscard]] const std::string& getSku() const noexcept { return sku_; }
    [[nodiscard]] int getRequested() const noexcept { return requested_; }
    [[nodiscard]] int getAvailable() const noexcept { return available_; }

private:
    std::string sku_;
    int requested_;
    int available_;
};

class TransactionException : public StoreForgeException {
public:
    explicit TransactionException(const std::string& message)
        : StoreForgeException("Transaction Failed: " + message) {}
};

class DatabaseException : public StoreForgeException {
public:
    explicit DatabaseException(const std::string& message)
        : StoreForgeException("Database Error: " + message) {}
};

class ExpiredProductException : public StoreForgeException {
public:
    explicit ExpiredProductException(const std::string& sku, const std::string& expiryDate)
        : StoreForgeException("Cannot sell expired product SKU " + sku + " (Expired on " + expiryDate + ")") {}
};

} // namespace storeforge::core
