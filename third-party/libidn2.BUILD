# Building this may need one or more of the following:
# sudo apt-get install gengetopt
# sudo apt-get install autotools-dev
# sudo apt-get install autoconf
# sudo apt-get install libtool
# sudo apt-get install autopoint
# sudo apt-get install libpthread-stubs0-dev

load("@rules_foreign_cc//foreign_cc:configure.bzl", "configure_make")

filegroup(
    name = "all",
    srcs = glob(["**"]),
)

configure_make(
    name = "libidn2",
    autogen = True,
    autogen_command = "bootstrap",
    autogen_options = [
        "--no-git",
        "--gnulib-srcdir=gnulib",
        "--skip-po",
        "--no-bootstrap-sync",
    ],
    env = {
        "CFLAGS": "-Wno-error",
    },
    configure_in_place = True,
    configure_options = [
        #"--help",
        "--disable-doc",
        "--enable-shared=no",
    ],
    lib_source = "@com_gitlab_libidn_libidn2//:all",
    visibility = ["//visibility:public"],
)