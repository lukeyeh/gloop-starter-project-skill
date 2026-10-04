#include "cache/replay.h"

#include <cstddef>
#include <span>
#include <string_view>

#include "cache/lru_cache.h"

namespace cache {
namespace {

// The cache is used as a set: only whether a key is present matters.
struct Present {};

}  // namespace

ReplayResult Replay(const size_t capacity,
                    const std::span<const std::string_view> keys) {
  LruCache<std::string_view, Present> cache(capacity);
  ReplayResult result;
  for (const std::string_view key : keys) {
    if (cache.Find(key) != nullptr) {
      ++result.hits;
    } else {
      ++result.misses;
      cache.Put(key, Present{});
    }
  }
  return result;
}

}  // namespace cache
