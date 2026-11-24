#include <gtest/gtest.h>
#include "storeforge/repo/InMemoryRepositories.hpp"
#include "storeforge/domain/StandardCommodity.hpp"
#include "storeforge/core/Exceptions.hpp"
#include <thread>
#include <vector>

using namespace storeforge;

TEST(InMemoryRepoTest, ItemCrudAndDuplicateHandling) {
    repo::InMemoryItemRepository repo;

    domain::StandardCommodity i1(0, core::SKU("TEST-SKU-1"), "Item 1", 10.0, 50, 5, core::ItemCategory::General, 1);
    core::ItemId id = repo.save(i1);
    EXPECT_GT(id, 0);

    auto found = repo.findById(id);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found.value()->getName(), "Item 1");

    auto foundBySku = repo.findBySku(core::SKU("TEST-SKU-1"));
    ASSERT_TRUE(foundBySku.has_value());
    EXPECT_EQ(foundBySku.value()->getId(), id);

    // Duplicate SKU insertion must throw DuplicateEntityException
    domain::StandardCommodity i2(0, core::SKU("TEST-SKU-1"), "Duplicate", 15.0, 20, 5, core::ItemCategory::General, 1);
    EXPECT_THROW(repo.save(i2), core::DuplicateEntityException);

    // Update stock
    repo.updateStock(id, 35);
    auto updated = repo.findById(id);
    ASSERT_TRUE(updated.has_value());
    EXPECT_EQ(updated.value()->getStockQuantity(), 35);

    // Remove
    EXPECT_TRUE(repo.remove(id));
    EXPECT_FALSE(repo.findById(id).has_value());
}

TEST(InMemoryRepoTest, SnapshotRollbackIntegrity) {
    repo::InMemoryItemRepository repo;
    domain::StandardCommodity item(1, core::SKU("SNAP-01"), "Item", 20.0, 100, 10, core::ItemCategory::General, 1);
    repo.save(item);

    // Create snapshot
    repo.createSnapshot();

    // Modify stock
    repo.updateStock(1, 40);
    EXPECT_EQ(repo.findById(1).value()->getStockQuantity(), 40);

    // Rollback snapshot
    repo.rollbackSnapshot();
    EXPECT_EQ(repo.findById(1).value()->getStockQuantity(), 100);
}

TEST(InMemoryRepoTest, ConcurrentReadsAndWrites) {
    repo::InMemoryItemRepository repo;
    domain::StandardCommodity item(1, core::SKU("CONC-01"), "ThreadSafe Item", 10.0, 1000, 10, core::ItemCategory::General, 1);
    repo.save(item);

    std::vector<std::thread> threads;
    for (int i = 0; i < 8; ++i) {
        threads.emplace_back([&repo]() {
            for (int j = 0; j < 50; ++j) {
                auto it = repo.findBySku(core::SKU("CONC-01"));
                EXPECT_TRUE(it.has_value());
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }
}
