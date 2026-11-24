#pragma once

namespace storeforge::repo {

class ITransactionContext {
public:
    virtual ~ITransactionContext() = default;
    virtual void commit() = 0;
    virtual void rollback() = 0;
    [[nodiscard]] virtual bool isActive() const = 0;
};

} // namespace storeforge::repo
