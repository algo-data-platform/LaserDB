package(
    default_visibility = ["//transform:__subpackages__"],
)

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

java_library(
    name = "core",
    srcs = glob([
        "com/weibo/ad/adcore/transform/core/*.java",
    ]),
    visibility = ["//control/batch_update_manager:__pkg__"],
    deps = [
        ":lombok",
        "@maven//:commons_codec_commons_codec",
        "@maven//:commons_collections_commons_collections",
        "@maven//:org_apache_hadoop_hadoop_common",
        "@maven//:org_apache_hadoop_hadoop_hdfs",
        "@maven//:org_apache_hadoop_hadoop_mapreduce_client_core",
        "@maven//:org_apache_hadoop_hadoop_mapreduce_client_jobclient",
        "@maven//:org_slf4j_slf4j_api",
        "@maven//:org_xerial_snappy_snappy_java",
    ],
)

java_binary(
    name = "job",
    srcs = glob([
        "com/weibo/ad/adcore/transform/job/*.java",
    ]),
    main_class = "com.weibo.ad.adcore.transform.job.LaserDataTransform",
    resources = [
        "resources/log4j.properties",
    ],
    deps = [
        ":core",
        ":lombok",
        "@maven//:log4j_log4j",
        "@maven//:org_apache_hadoop_hadoop_common",
        "@maven//:org_apache_hadoop_hadoop_hdfs",
        "@maven//:org_apache_hadoop_hadoop_mapreduce_client_core",
        "@maven//:org_apache_hadoop_hadoop_mapreduce_client_jobclient",
        "@maven//:org_slf4j_slf4j_api",
        "@maven//:org_xerial_snappy_snappy_java",
    ],
)
