cc_test(
    name = "http_sse_codec_test",
    srcs = ["http_sse_codec_test.cc"],
    deps = [
        "//trpc/codec/http_sse:http_sse_codec",
        "//trpc/codec/http:http_protocol",
        "//trpc/client:client_context",
        "//trpc/server:server_context",
        "//trpc/util/http/sse:http_sse",
        "//trpc/util/http/sse:http_sse_parser",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)