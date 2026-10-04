// Answers "how well would a cache of this size have done?" for a recorded
// sequence of key accesses.

#ifndef CACHE_REPLAY_H_
#define CACHE_REPLAY_H_

#include <cstddef>
#include <span>
#include <string_view>

namespace cache {

// How a sequence of accesses fared. `hits + misses` is the sequence's length.
struct ReplayResult {
  // Accesses whose key was still in the cache.
  size_t hits = 0;
  // Accesses whose key was new or had already been dropped.
  size_t misses = 0;

  friend bool operator==(const ReplayResult&, const ReplayResult&) = default;
};

// Runs `keys`, in order, against an initially empty least-recently-used cache
// holding at most `capacity` entries. Each key is looked up and, on a miss,
// stored.
ReplayResult Replay(size_t capacity, std::span<const std::string_view> keys);

}  // namespace cache

#endif  // CACHE_REPLAY_H_
