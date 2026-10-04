// Replay by example: the same accesses scored against caches of different
// sizes.

#include "cache/replay.h"

#include <string_view>
#include <vector>

#include "gtest/gtest.h"

namespace cache {
namespace {

// Repeats are hits as long as the cache is big enough to still hold the key.
TEST(ReplayTest, CountsHitsAndMisses) {
  const std::vector<std::string_view> keys = {
      "a", "b", "a", "c", "b",
  };

  // Room for everything: only the first sight of each key misses.
  EXPECT_EQ(Replay(3, keys), (ReplayResult{
                                 .hits = 2,
                                 .misses = 3,
                             }));
  // Room for two: "c" pushes out "b", so the final "b" misses as well.
  EXPECT_EQ(Replay(2, keys), (ReplayResult{
                                 .hits = 1,
                                 .misses = 4,
                             }));
}

// With no capacity nothing is ever remembered.
TEST(ReplayTest, ZeroCapacityAlwaysMisses) {
  const std::vector<std::string_view> keys = {
      "a",
      "a",
  };

  EXPECT_EQ(Replay(0, keys), (ReplayResult{
                                 .hits = 0,
                                 .misses = 2,
                             }));
}

// An empty recording is a valid input.
TEST(ReplayTest, NoAccessesGivesZeroes) {
  EXPECT_EQ(Replay(4, {}), ReplayResult{});
}

}  // namespace
}  // namespace cache
