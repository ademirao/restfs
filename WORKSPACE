workspace(name = "restfs")

load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "rules_foreign_cc",
    commit = "b136e6c52da63da300b0f588c8a214d97b0d15cd",  # current as of 2021/05/06
    remote = "git://github.com/bazelbuild/rules_foreign_cc.git",
    shallow_since = "1620262045 -0700",
)

load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")

rules_foreign_cc_dependencies([])

load("@restfs//third-party:repositories.bzl", "load_all_repositories")

load_all_repositories()
