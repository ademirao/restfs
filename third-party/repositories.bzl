load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository", "new_git_repository")

def load_all_repositories():
    """ Nada por enquanto
    """

    new_git_repository(
        name = "com_github_curl_curl",
        build_file = "//third-party:libcurl.BUILD",
        commit = "781864bedbc57e2e2532bde7cf64db9af7b80d05",
        remote = "git://github.com/curl/curl.git",
        shallow_since = "1620211916 +0200",
    )

    git_repository(
        name = "com_google_absl",
        commit = "143a27800eb35f4568b9be51647726281916aac9",  # current as of 2021/02/17
        remote = "git://github.com/abseil/abseil-cpp.git",
        shallow_since = "1613186346 -0500",
    )

    http_archive(
        name = "com_github_open_source_parsers_jsoncpp",
        build_file = "//third-party:jsoncpp.BUILD",
        sha256 =
            "c49deac9e0933bcb7044f08516861a2d560988540b23de2ac1ad443b219afdb6",
        strip_prefix = "jsoncpp-1.8.4",
        urls = ["https://github.com/open-source-parsers/jsoncpp/archive/1.8.4.tar.gz"],
    )

    new_git_repository(
        name = "com_gitlab_libidn_libidn2",
        build_file = "//third-party:libidn2.BUILD",
        commit = "49fe79c77f6013356920b46bd07f95bcb609f541",  # current as of 2021/11/01
        recursive_init_submodules = True,
        remote = "https://gitlab.com/libidn/libidn2.git",
        shallow_since = "1632651379 +0200",
    )

    http_archive(
        name = "zlib",
        build_file = "//third-party:zlib.BUILD",
        sha256 = "c3e5e9fdd5004dcb542feda5ee4f0ff0744628baf8ed2dd5d66f8ca1197cb1a1",
        strip_prefix = "zlib-1.2.11",
        urls = [
            "https://mirror.bazel.build/zlib.net/zlib-1.2.11.tar.gz",
            "https://zlib.net/zlib-1.2.11.tar.gz",
        ],
    )

    new_git_repository(
        name = "com_github_openssl_openssl",
        #recursive_init_submodules = True,
        build_file = "//third-party:openssl.BUILD",
        commit = "7ca3bf792a4a085e6f2426ad51a41fca4d0b1b8c",  # current as of 2021/12/17
        remote = "https://github.com/openssl/openssl.git",
        shallow_since = "1639760376 +0100",
    )
