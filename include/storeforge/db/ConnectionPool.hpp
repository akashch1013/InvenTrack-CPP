#pragma once

#include <string>
#include <memory>
#include <vector>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <functional>
#include "storeforge/core/Exceptions.hpp"

#if defined(STOREFORGE_HAS_LIBPQXX)
#include <pqxx/pqxx>
#endif

namespace storeforge::db {

class ConnectionPool {
public:
    struct Config {
        std::string host{"localhost"};
        int port{5432};
        std::string database{"storeforge_db"};
        std::string user{"postgres"};
        std::string password{"postgres"};
        size_t minConnections{2};
        size_t maxConnections{10};
        int connectionTimeoutSeconds{5};

        [[nodiscard]] std::string toConnectionString() const;
    };

    explicit ConnectionPool(Config config);
    ~ConnectionPool();

    ConnectionPool(const ConnectionPool&) = delete;
    ConnectionPool& operator=(const ConnectionPool&) = delete;

    void initialize();
    void shutdown();

#if defined(STOREFORGE_HAS_LIBPQXX)
    using NativeConnection = pqxx::connection;
    using ConnectionPtr = std::shared_ptr<NativeConnection>;

    [[nodiscard]] ConnectionPtr acquire();
    void release(std::unique_ptr<NativeConnection> conn);
#endif

    [[nodiscard]] bool isConnected() const noexcept { return isInitialized_; }
    [[nodiscard]] size_t getActiveConnections() const;
    [[nodiscard]] size_t getIdleConnections() const;
    [[nodiscard]] const Config& getConfig() const noexcept { return config_; }

private:
    Config config_;
    bool isInitialized_{false};
    mutable std::mutex mutex_;
    std::condition_variable cv_;

#if defined(STOREFORGE_HAS_LIBPQXX)
    std::queue<std::unique_ptr<NativeConnection>> pool_;
#endif
    size_t totalCreated_{0};
};

} // namespace storeforge::db
