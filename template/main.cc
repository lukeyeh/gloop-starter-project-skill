// Scores a sequence of keys against a least-recently-used cache:
//
//   bazel run //:lru -- --capacity=2 a b a c b

#include <cstddef>
#include <iostream>
#include <string_view>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "cache/replay.h"

ABSL_FLAG(size_t, capacity, 2, "How many entries the cache holds");

int main(int argc, char** argv) {
  const std::vector<char*> args = absl::ParseCommandLine(argc, argv);
  // args[0] is the program name; the rest are the keys.
  const std::vector<std::string_view> keys(args.begin() + 1, args.end());

  const cache::ReplayResult result =
      cache::Replay(absl::GetFlag(FLAGS_capacity), keys);
  std::cout << "hits=" << result.hits << " misses=" << result.misses << "\n";
  return 0;
}
