package(default_visibility = ["//visibility:public"])

cc_test(
    name = "http_sse_stream_test",
    srcs = ["http_sse_stream_test.cc"],
    deps = [
        "//trpc/stream/http_sse:http_sse_stream",
        "//trpc/stream:stream_options",
        "//trpc/util/http/sse:http_sse",
        "@com_google_googletest//:gtest_main",
    ],
    size = "small",
    timeout = "short",
)

cc_test(
    name = "http_sse_stream_handler_test",
    srcs = ["http_sse_stream_handler_test.cc"],
    deps = [
        "//trpc/stream/http_sse:http_sse_stream",
        "//trpc/stream:stream_options",
        "//trpc/stream:protocol_message_metadata",
        "@com_google_googletest//:gtest_main",
    ],
    size = "small",
    timeout = "short",
)
