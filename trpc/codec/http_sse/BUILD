load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "http_sse_codec",
    srcs = [
        "http_sse_codec.cc",
        "http_sse_proto_checker_impl.cc",
    ],
    hdrs = [
        "http_sse_codec.h",
        "http_sse_proto_checker.h",
    ],
    deps = [
        "//trpc/codec/http:http_client_codec",
        "//trpc/codec/http:http_server_codec",
        "//trpc/codec/http:http_protocol",
        "//trpc/codec/http:http_client_proto_checker_impl",
        "//trpc/codec/http:http_server_proto_checker_impl",
        "//trpc/util/http/sse:http_sse",
        "//trpc/util/http/sse:http_sse_parser",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/common:status",
        "//trpc/log:trpc_log",
    ],
)


