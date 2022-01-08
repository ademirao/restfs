cc_library(
    name = "path",
    srcs = ["path.cc"],
    hdrs = ["path.h"],
    deps = [":logger"],
)

cc_library(
    name = "logger",
    srcs = ["logger.cc"],
    hdrs = ["logger.h"],
    deps = [],
)

cc_library(
    name = "rest",
    srcs = ["rest.cc"],
    hdrs = ["rest.h"],
    deps = [":logger"],
)

cc_library(
    name = "http",
    srcs = ["http.cc"],
    hdrs = ["http.h"],
    deps = [":rest"],
)

cc_library(
    name = "openapi",
    srcs = ["openapi.cc"],
    hdrs = ["openapi.h"],
    deps = [
        ":path",
        ":rest",
    ],
)

# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_library
cc_library(
    name = "restfs_lib",
    srcs = ["main.cc"],
    deps = [
        ":http",
        ":logger",
        ":openapi",
        ":rest",
        "@com_github_curl_curl//:curl",
        "@com_github_open_source_parsers_jsoncpp//:jsoncpp",
    ],
)

# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_binary
cc_binary(
    name = "restfs",
    linkopts = [
        "-lpthread",
        "-lfuse3",
    ],
    deps = [":restfs_lib"],
)

sh_binary(
    name = "mount_example",
    deps = [],
    srcs = ["mount_example.sh"],
    data = [],
    args = [""],
    env = {"": ""},
    tags = [""],
    toolchains = [],
)
