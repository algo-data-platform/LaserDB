load("@io_bazel_rules_go//go:def.bzl", "go_library", "go_test")

go_library(
    name = "laser",
    srcs = [
        "laser/constants.go",
        "laser/laserservice.go",
        "laser/ttypes.go",
    ],
    importpath = "github.com/algo-data-platform/LaserDB/sdk/go/laser_client/if/laser",
    # visibility = ["//sdk/go/laser_client:__subpackages__"],
    visibility = ["//visibility:public"],
    deps = [
        "@fbthrift//:fbthrift_go",
    ],
)
