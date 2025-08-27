cc_library(
    name = "http_sse_stream_proxy",
    srcs = ["http_sse_stream_proxy.cc"],
    hdrs = ["http_sse_stream_proxy.h"],
    deps = [
        "//trpc/client/http:http_service_proxy",
        "//trpc/client:client_context",
        "//trpc/client:make_client_context",
        "//trpc/client:service_proxy_option",
        "//trpc/client:trpc_client",
        "//trpc/codec/http:http_protocol",
        "//trpc/codec/http_sse:http_sse_codec",
        "//trpc/codec:client_codec_factory",
        "//trpc/common:status",
        "//trpc/stream/http:http_client_stream",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/log:logging",
        "//trpc/util/http:util",
        "//trpc/util/http/sse:http_sse",
        "//trpc/util/http/sse:http_sse_parser",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "http_sse_stream_proxy_test",
    srcs = ["http_sse_stream_proxy_test.cc"],
    deps = [
        ":http_sse_stream_proxy",
        "//trpc/client:client_context",
        "//trpc/client:service_proxy_option",
        "//trpc/common:status",
        "//trpc/util/http/sse:http_sse",
        "//trpc/util/http/sse:http_sse_parser",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
