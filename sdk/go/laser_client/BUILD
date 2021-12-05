load("@io_bazel_rules_go//go:def.bzl", "go_library", "go_test")

go_library(
    name = "laser_client",
    srcs = [
        "laser_client.go",
        "metrics.go",
        "version.go",
    ],
    importpath = "github.com/algo-data-platform/LaserDB/sdk/go/common/laser_client",
    visibility = ["//visibility:public"],
    deps = [
        "//sdk/go/common/ip",
        "//sdk/go/common/service_router",
        "//sdk/go/laser_client/if:laser",
        "//sdk/go/laser_client/lib:laser_client_lib",
        "@com_github_hashicorp_consul//api:go_default_library",
        "@com_github_liubang_tally//:go_default_library",
        "@fbthrift//:fbthrift_go",
    ],
)
