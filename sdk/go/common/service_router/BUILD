load("@io_bazel_rules_go//go:def.bzl", "go_library", "go_test")

go_library(
    name = "service_router",
    srcs = [
        "constants.go",
        "load_balance.go",
        "node.go",
        "pool.go",
        "registry.go",
        "router.go",
        "router_db.go",
        "service_puller.go",
        "service_pusher.go",
        "thrift.go",
    ],
    importpath = "github.com/algo-data-platform/LaserDB/sdk/go/common/service_router",
    visibility = ["//visibility:public"],
    deps = [
        "//sdk/go/common/ip",
        "@com_github_hashicorp_consul//api:go_default_library",
        "@com_github_liubang_tally//:go_default_library",
        "@com_github_liubang_tally//prometheus:go_default_library",
        "@com_github_zentures_cityhash//:go_default_library",
        "@fbthrift//:fbthrift_go",
        "@org_uber_go_atomic//:go_default_library",
    ],
)

go_test(
    name = "service_router_test",
    srcs = [
        "load_balance_test.go",
        "pool_test.go",
        "router_db_test.go",
        "thrift_test.go",
    ],
    embed = [":service_router"],
    deps = [
        "@com_github_stretchr_testify//assert:go_default_library",
    ],
)
