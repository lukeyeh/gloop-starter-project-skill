load("@rules_cc//cc:cc_binary.bzl", "cc_binary")

cc_binary(
    name = "lru",
    srcs = ["main.cc"],
    deps = [
        "//cache:replay",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:parse",
    ],
)

alias(
    name = "compile_commands",
    actual = "@wolfd_bazel_compile_commands//:generate_compile_commands",
)
