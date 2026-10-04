load("@rules_cc//cc:cc_library.bzl", "cc_library")
load("@rules_cc//cc:cc_test.bzl", "cc_test")

package(default_visibility = ["//:__subpackages__"])

cc_library(
    name = "lru_cache",
    hdrs = ["lru_cache.h"],
    deps = [
        "@abseil-cpp//absl/container:node_hash_map",
        "@gloop//gloop/util/gtl:intrusive_list",
    ],
)

cc_test(
    name = "lru_cache_test",
    srcs = ["lru_cache_test.cc"],
    deps = [
        ":lru_cache",
        "//testing:main",
        "@gbenchmark",
        "@googletest//:gtest",
    ],
)

cc_library(
    name = "replay",
    srcs = ["replay.cc"],
    hdrs = ["replay.h"],
    deps = [":lru_cache"],
)

cc_test(
    name = "replay_test",
    srcs = ["replay_test.cc"],
    deps = [
        ":replay",
        "//testing:main",
        "@googletest//:gtest",
    ],
)
