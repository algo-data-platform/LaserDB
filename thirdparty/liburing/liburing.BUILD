# vim: ft=bzl

genrule(
    name = "compat_h",
    outs = [
        "src/include/liburing/compat.h",
    ],
    tools = [
        "configure",
    ],
    cmd = "./$(location configure) --compat=$@",
)

cc_library(
    name = "liburing",
    srcs = glob(["src/*.c"]) + [
        "src/syscall.h",
    ],
    hdrs = [
        ":compat_h",
        "src/include/liburing.h",
        "src/include/liburing/barrier.h",
        "src/include/liburing/io_uring.h",
    ],
    includes = ["src/include"],
    copts = ["-w"],
    visibility = ["//visibility:public"],
)
