// A cache of bounded size that forgets the entry used longest ago.
//
// An example of Gloop's gtl::intrusive_list: each entry carries its own list
// links, so moving it to the "most recent" end on every access costs a few
// pointer writes and no allocation.

#ifndef CACHE_LRU_CACHE_H_
#define CACHE_LRU_CACHE_H_

#include <cstddef>
#include <utility>

#include "absl/container/node_hash_map.h"
#include "gloop/util/gtl/intrusive_list.h"

namespace cache {

// Maps keys to values, keeping at most a fixed number of entries. When a new
// key would exceed that, the entry that was least recently stored or looked
// up is dropped. All operations take constant time on average.
//
// `Key` must be hashable, equality comparable and copyable. Not thread-safe.
template <typename Key, typename Value>
class LruCache {
 public:
  // A cache that keeps at most `capacity` entries. With a capacity of zero it
  // keeps nothing: every Find misses.
  explicit LruCache(const size_t capacity) : capacity_(capacity) {}

  // Returns the value stored under `key`, or nullptr if there is none. A hit
  // makes the entry the most recently used. The pointer is valid until the
  // entry is dropped or replaced by a later Put.
  Value* Find(const Key& key) {
    const auto it = entries_.find(key);
    if (it == entries_.end()) return nullptr;
    Touch(&it->second);
    return &it->second.value;
  }

  // Stores `value` under `key`, replacing any value already there, and makes
  // the entry the most recently used. May drop the least recently used entry
  // to make room.
  void Put(const Key& key, Value value) {
    if (capacity_ == 0) return;
    const auto [it, inserted] =
        entries_.try_emplace(key, key, std::move(value));
    Entry* const entry = &it->second;
    if (!inserted) {
      entry->value = std::move(value);
      Touch(entry);
      return;
    }
    by_recency_.push_back(entry);
    if (entries_.size() > capacity_) DropLeastRecent();
  }

  // The number of entries currently held; never more than the capacity.
  size_t size() const { return entries_.size(); }

 private:
  // The list links live in the entry itself. That is what "intrusive" means,
  // and why an entry can be unlinked given only a pointer to it.
  struct Entry : gtl::intrusive_link<Entry> {
    Entry(const Key& key, Value value) : key(key), value(std::move(value)) {}

    // A copy of the map key, so that the entry found at the front of the
    // list can be erased from the map.
    const Key key;
    Value value;
  };

  void Touch(Entry* const entry) {
    by_recency_.erase(entry);
    by_recency_.push_back(entry);
  }

  void DropLeastRecent() {
    Entry* const victim = &by_recency_.front();
    by_recency_.erase(victim);
    entries_.erase(entries_.find(victim->key));
  }

  const size_t capacity_;
  // Owns the entries. A node map, because a linked entry must not move.
  absl::node_hash_map<Key, Entry> entries_;
  // Every entry, least recently used first.
  gtl::intrusive_list<Entry> by_recency_;
};

}  // namespace cache

#endif  // CACHE_LRU_CACHE_H_
