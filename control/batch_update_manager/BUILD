load("@bazel_springboot_rule//tools/springboot:springboot.bzl", "springboot")
load("@rules_java//java:defs.bzl", "java_library", "java_plugin")

java_plugin(
    name = "lombok_plugin",
    processor_class = "lombok.launch.AnnotationProcessorHider$AnnotationProcessor",
    deps = [
        "@lombok//jar",
    ],
)

java_library(
    name = "lombok",
    exported_plugins = [
        ":lombok_plugin",
    ],
    exports = [
        "@lombok//jar",
    ],
)

springboot_deps = [
    "@maven//:org_springframework_boot_spring_boot_loader",
    "@maven//:org_springframework_boot_spring_boot_starter_web",
    "@maven//:org_springframework_boot_spring_boot_starter_log4j2",
    "@maven//:org_springframework_boot_spring_boot_starter_test",
    "@maven//:org_springframework_boot_spring_boot_autoconfigure",
    "@maven//:org_springframework_boot_spring_boot",
    "@maven//:org_springframework_spring_beans",
    "@maven//:org_springframework_spring_context",
    "@maven//:org_springframework_spring_web",
]

lib_deps = [
    "//transform:core",
    ":lombok",
    "@maven//:org_apache_hadoop_hadoop_common",
    "@maven//:org_apache_hadoop_hadoop_hdfs",
    "@maven//:org_apache_hadoop_hadoop_client",
    "@maven//:org_slf4j_slf4j_api",
    "@maven//:com_alibaba_fastjson",
    "@maven//:com_squareup_okhttp3_okhttp",
    "@maven//:com_fasterxml_jackson_core_jackson_annotations",
    "@maven//:commons_lang_commons_lang",
    "@maven//:commons_codec_commons_codec",
    "@maven//:jakarta_annotation_jakarta_annotation_api",
]

java_library(
    name = "batch_update_manager_lib",
    srcs = glob(
        [
            "src/main/java/com/weibo/ad/adcore/batch_update_manager/**/*.java",
        ],
        allow_empty = False,
    ),
    deps = springboot_deps + lib_deps,
)

springboot(
    name = "laser_batch_update_manager",
    boot_app_class = "com.weibo.ad.adcore.batch_update_manager.BatchUpdateManagerApplication",
    java_library = ":batch_update_manager_lib",
    visibility = ["//visibility:public"],
)
