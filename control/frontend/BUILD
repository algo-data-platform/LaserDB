filegroup(
    name = "srcs",
    srcs = glob(["**/**"]),
    visibility = ["//visibility:public"],
)

genrule(
    name = "build",
    srcs = [
        ":srcs",
    ],
    outs = ["dist"],
    cmd = """
        export PATH=$$PWD/$$(dirname $(location @nodejs//:node_bin)):$$PATH
        vue_cli_service=$$PWD/$(location //control/frontend:node_modules/.bin/vue-cli-service) outs=$$PWD/$@
        cd control/frontend && $$vue_cli_service build --dest $$outs
    """,
    tools = [
        "//control/frontend:node_modules/.bin/vue-cli-service",
        "@nodejs//:node_bin",
    ],
    visibility = ["//visibility:public"],
)
