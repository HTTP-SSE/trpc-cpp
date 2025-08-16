package(default_visibility = ["//visibility:public"])

cc_library(
    name = "http_sse_stream",
    srcs = [
        "http_sse_stream.cc",
        "http_sse_stream_handler.cc",
        "http_sse_stream_handler_factory.cc",
    ],
    hdrs = [
        "http_sse_stream.h",
        "http_sse_stream_handler.h",
        "http_sse_stream_handler_factory.h",
    ],
    deps = [
        "//trpc/stream/http:http_stream",
        "//trpc/stream/http:http_stream_handler",
        "//trpc/stream:stream_handler_factory",
        "//trpc/util/http/sse:http_sse",
        "//trpc/util/http/sse:http_sse_parser",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/util/log:trpc_log",
        "//trpc/common:status",
        "//trpc/coroutine:fiber",
        "//trpc/coroutine:fiber_mutex",
        "//trpc/coroutine:fiber_condition_variable",
    ],
)

cc_test(
    name = "http_sse_stream_test",
    srcs = [
        "test/http_sse_stream_test.cc",
        "test/http_sse_stream_handler_test.cc",
    ],
    deps = [
        ":http_sse_stream",
        "//trpc/stream/http:http_stream",
        "//trpc/stream/http:http_stream_handler",
        "//trpc/util/http/sse:http_sse",
        "//trpc/util/http/sse:http_sse_parser",
        "//trpc/common:status",
        "//trpc/coroutine:fiber",
        "//trpc/coroutine:fiber_mutex",
        "//trpc/coroutine:fiber_condition_variable",
        "@com_google_googletest//:gtest_main",
    ],
    size = "small",
    timeout = "short",
)
