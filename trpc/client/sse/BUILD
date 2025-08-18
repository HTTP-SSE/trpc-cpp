cc_library(
    name = "http_sse_stream_proxy",
    srcs = ["http_sse_stream_proxy.cc"],
    hdrs = ["http_sse_stream_proxy.h"],
    deps = [
        "//trpc/client:service_proxy",
        "//trpc/codec/http_sse:http_sse_codec",
        "//trpc/stream/http:http_client_stream",
        "//trpc/common:status",
        "//trpc/log:logging",
        "//trpc/util/http:method",
    ],
    visibility = ["//visibility:public"],
)
cc_test(
    name = "http_sse_stream_proxy_test",
    srcs = ["http_sse_stream_proxy_test.cc"],
    deps = [
        "//trpc/client/http_sse:http_sse_stream_proxy",
        "//trpc/client:make_client_context",
        "//trpc/client:service_proxy_option_setter",
        "//trpc/codec:codec_manager",
        "//trpc/codec/http:http_client_codec",
        "//trpc/common/config:trpc_config",
        "//trpc/future:future_utility",
        "//trpc/runtime:merge_runtime",
        "//trpc/runtime/threadmodel/merge:merge_thread_model",
        "//trpc/serialization:trpc_serialization",
        "//trpc/stream/http:http_client_stream",
        "//trpc/stream/testing:mock_stream_handler",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
