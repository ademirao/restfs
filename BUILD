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
        "@com_google_absl//absl/flags:flag",
        "@com_google_absl//absl/flags:parse",
    ],
)

filegroup(
    name = "examples",
    srcs = glob(["examples/**/openapi.json"]),
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

[
    rule
    for f in glob(["examples/**/openapi.json"])
    for rule in [
        sh_binary(
            name = "run_" + f,
            srcs = [":restfs"],
            data = ["//:%s" % f, ":restfs"],
            args = ["--api_spec_addr=$(location %s)" % f],
        ),
    ]
]

# cc_binary(
#     name = "examples/api-staging.magalu.com",
#     linkopts = [
#         "-lpthread",
#         "-lfuse3",
#     ],
#     args ["-s", "-f", "/mnt/retfs"]
#     deps = [":restfs_lib"],
# )

# sh_(
#     name = "mount_example",
#     srcs = ["mount_example.sh"],
#     args = [""],
#     data = [":restfs"],
#     env = {"": ""},
#     tags = [""],
#     toolchains = [],
#     deps = [],
# )
