load("@io_bazel_rules_go//go:def.bzl", "go_library", "go_test")

go_library(
    name = "laser_client_lib",
    srcs = [
        "config_manager.go",
        "format.go",
        "partition.go",
    ],
    importpath = "github.com/algo-data-platform/LaserDB/sdk/go/laser_client/lib",
    # visibility = ["//sdk/go/laser_client:__subpackages__"],
    visibility = ["//visibility:public"],
    deps = [
        "//sdk/go/common/service_router",
        "@com_github_zentures_cityhash//:go_default_library",
    ],
)
