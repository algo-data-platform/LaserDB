# vim: ft=bzl

load("@rules_foreign_cc//tools/build_defs:make.bzl", "make")

filegroup(
    name = "all",
    srcs = glob(["**"]),
)

make(
    name = "libaio",
    static_libraries = [
        "libaio.a",
    ],
    lib_source = ":all",
    visibility = ["//visibility:public"],
)
