#include "storeforge/db/ConnectionPool.hpp"
#include <iostream>
#include <sstream>

namespace storeforge::db {

std::string ConnectionPool::Config::toConnectionString() const {
    std::ostringstream ss;
    ss << "host=" << host
       << " port=" << port
       << " dbname=" << database
       << " user=" << user
       << " password=" << password
       << " connect_timeout=" << connectionTimeoutSeconds;
    return ss.str();
}

ConnectionPool::ConnectionPool(Config config)
    : config_(std::move(config)) {}

ConnectionPool::~ConnectionPool() {
    shutdown();
}

void ConnectionPool::initialize() {
    std::unique_lock lock(mutex_);
    if (isInitialized_) return;

#if defined(STOREFORGE_HAS_LIBPQXX)
    try {
        std::string connStr = config_.toConnectionString();
        for (size_t i = 0; i < config_.minConnections; ++i) {
            auto conn = std::make_unique<pqxx::connection>(connStr);
            if (!conn->is_open()) {
                throw core::DatabaseException("Failed to establish PostgreSQL connection to database: " + config_.database);
            }
            pool_.push(std::move(conn));
            totalCreated_++;
        }
    } catch (const std::exception& e) {
        throw core::DatabaseException(std::string("ConnectionPool initialization failed: ") + e.what());
    }
#endif

    isInitialized_ = true;
}

void ConnectionPool::shutdown() {
    std::unique_lock lock(mutex_);
    if (!isInitialized_) return;

#if defined(STOREFORGE_HAS_LIBPQXX)
    while (!pool_.empty()) {
        pool_.pop(); // Destructor of std::unique_ptr<NativeConnection> closes connection via RAII
    }
    totalCreated_ = 0;
#endif

    isInitialized_ = false;
    cv_.notify_all();
}

#if defined(STOREFORGE_HAS_LIBPQXX)
ConnectionPool::ConnectionPtr ConnectionPool::acquire() {
    std::unique_lock lock(mutex_);
    if (!isInitialized_) {
        throw core::DatabaseException("Connection pool is not initialized");
    }

    auto timeout = std::chrono::seconds(config_.connectionTimeoutSeconds);
    bool acquired = cv_.wait_for(lock, timeout, [this]() {
        return !pool_.empty() || totalCreated_ < config_.maxConnections;
    });

    if (!acquired) {
        throw core::DatabaseException("Timed out waiting for available PostgreSQL connection from pool");
    }

    std::unique_ptr<NativeConnection> rawConn;
    if (!pool_.empty()) {
        rawConn = std::move(pool_.front());
        pool_.pop();
    } else if (totalCreated_ < config_.maxConnections) {
        try {
            rawConn = std::make_unique<NativeConnection>(config_.toConnectionString());
            totalCreated_++;
        } catch (const std::exception& e) {
            throw core::DatabaseException(std::string("Failed to create new pooled connection: ") + e.what());
        }
    }

    // Custom deleter returning connection to pool
    auto* poolPtr = this;
    return ConnectionPtr(rawConn.release(), [poolPtr](NativeConnection* c) {
        if (c) {
            poolPtr->release(std::unique_ptr<NativeConnection>(c));
        }
    });
}

void ConnectionPool::release(std::unique_ptr<NativeConnection> conn) {
    if (!conn) return;
    std::unique_lock lock(mutex_);
    if (isInitialized_ && conn->is_open()) {
        pool_.push(std::move(conn));
    } else {
        if (totalCreated_ > 0) totalCreated_--;
    }
    cv_.notify_one();
}
#endif

size_t ConnectionPool::getActiveConnections() const {
    std::lock_guard lock(mutex_);
#if defined(STOREFORGE_HAS_LIBPQXX)
    return totalCreated_ - pool_.size();
#else
    return 0;
#endif
}

size_t ConnectionPool::getIdleConnections() const {
    std::lock_guard lock(mutex_);
#if defined(STOREFORGE_HAS_LIBPQXX)
    return pool_.size();
#else
    return 0;
#endif
}

} // namespace storeforge::db
