# vim: ft=bzl

load("@rules_foreign_cc//tools/build_defs:make.bzl", "make")

filegroup(
    name = "all",
    srcs = glob(["**"]),
)

make(
    name = "libaio",
    lib_source = ":all",
    static_libraries = [
        "libaio.a",
    ],
    visibility = ["//visibility:public"],
)
