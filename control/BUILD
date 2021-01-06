load("@io_bazel_rules_go//go:def.bzl", "go_binary", "go_library")
load("@bazel_gazelle//:def.bzl", "gazelle")

# gazelle:prefix laser-control
gazelle(name = "gazelle")

go_library(
    name = "laser_control_lib",
    srcs = ["main.go"],
    importpath = "laser-control",
    visibility = ["//visibility:private"],
    deps = ["//control/cmd"],
)

go_binary(
    name = "laser_control",
    embed = [":laser_control_lib"],
    visibility = ["//visibility:public"],
)

exports_files(["templates"])
