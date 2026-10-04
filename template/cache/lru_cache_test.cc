// LruCache by example: what it remembers, what it forgets and in which
// order. The benchmarks for the same code are at the bottom of the file.

#include "cache/lru_cache.h"

#include <benchmark/benchmark.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace cache {
namespace {

using ::testing::IsNull;
using ::testing::Pointee;

// The basic contract: what was put can be found, anything else cannot.
TEST(LruCacheTest, FindsWhatWasPut) {
  LruCache<std::string, int> cache(2);
  cache.Put("a", 1);

  EXPECT_THAT(cache.Find("a"), Pointee(1));
  EXPECT_THAT(cache.Find("b"), IsNull());
}

// Going over capacity drops the entry that was stored first.
TEST(LruCacheTest, DropsTheOldestEntryWhenFull) {
  LruCache<std::string, int> cache(2);
  cache.Put("a", 1);
  cache.Put("b", 2);
  cache.Put("c", 3);

  EXPECT_THAT(cache.Find("a"), IsNull());
  EXPECT_THAT(cache.Find("b"), Pointee(2));
  EXPECT_THAT(cache.Find("c"), Pointee(3));
  EXPECT_EQ(cache.size(), 2);
}

// "Least recently used" counts lookups, not just insertions.
TEST(LruCacheTest, FindKeepsAnEntryAlive) {
  LruCache<std::string, int> cache(2);
  cache.Put("a", 1);
  cache.Put("b", 2);
  ASSERT_THAT(cache.Find("a"), Pointee(1));  // "b" is now the older one.
  cache.Put("c", 3);

  EXPECT_THAT(cache.Find("a"), Pointee(1));
  EXPECT_THAT(cache.Find("b"), IsNull());
}

// Putting an existing key replaces its value without using another slot, and
// counts as a use.
TEST(LruCacheTest, PutReplacesAndRefreshes) {
  LruCache<std::string, int> cache(2);
  cache.Put("a", 1);
  cache.Put("b", 2);
  cache.Put("a", 10);
  EXPECT_EQ(cache.size(), 2);
  cache.Put("c", 3);

  EXPECT_THAT(cache.Find("a"), Pointee(10));
  EXPECT_THAT(cache.Find("b"), IsNull());
}

// The value can be changed in place through the pointer Find returns.
TEST(LruCacheTest, FindGivesMutableAccess) {
  LruCache<std::string, int> cache(1);
  cache.Put("hits", 0);
  ++*cache.Find("hits");

  EXPECT_THAT(cache.Find("hits"), Pointee(1));
}

// A zero-capacity cache is valid and simply remembers nothing, so callers
// need no special case for "caching disabled".
TEST(LruCacheTest, ZeroCapacityKeepsNothing) {
  LruCache<std::string, int> cache(0);
  cache.Put("a", 1);

  EXPECT_THAT(cache.Find("a"), IsNull());
  EXPECT_EQ(cache.size(), 0);
}

// Values only need to be movable.
TEST(LruCacheTest, HoldsMoveOnlyValues) {
  LruCache<int, std::unique_ptr<std::string>> cache(1);
  cache.Put(1, std::make_unique<std::string>("one"));

  EXPECT_THAT(cache.Find(1), Pointee(Pointee(std::string("one"))));
}

// -----------------------------------------------------------------------------
// Benchmarks
// -----------------------------------------------------------------------------
//
//   bazel run -c opt //cache:lru_cache_test -- --benchmark_filter=all

// A lookup that hits: one hash probe plus relinking the entry.
void BM_FindHit(benchmark::State& state) {
  const int64_t entries = state.range(0);
  LruCache<int64_t, int64_t> cache(static_cast<size_t>(entries));
  for (int64_t i = 0; i < entries; ++i) cache.Put(i, i);

  int64_t key = 0;
  for (auto _ : state) {
    benchmark::DoNotOptimize(cache.Find(key));
    key = (key + 1) % entries;
  }
}
BENCHMARK(BM_FindHit)->Range(64, 64 << 10);

// An insertion into a full cache: allocates one entry and frees another.
void BM_PutEvicting(benchmark::State& state) {
  const int64_t entries = state.range(0);
  LruCache<int64_t, int64_t> cache(static_cast<size_t>(entries));
  for (int64_t i = 0; i < entries; ++i) cache.Put(i, i);

  int64_t key = entries;
  for (auto _ : state) {
    cache.Put(key, key);
    ++key;
  }
}
BENCHMARK(BM_PutEvicting)->Range(64, 64 << 10);

}  // namespace
}  // namespace cache
