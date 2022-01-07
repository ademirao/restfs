
# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_library
cc_library(
    name = "restfs_lib",
    srcs = glob(["*.h"]),
    hdrs = glob(["*.cc"], exclude=["main.cc"]),
    deps = [],
)

# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_binary
cc_binary(
    name = "restfs",
    deps = [":restfs_lib"],
	srcs = ["main.cc"],
)
