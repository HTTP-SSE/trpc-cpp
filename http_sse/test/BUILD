#trpc/server/http_sse/test
cc_test(
    name = "ssestreamwriter_test1",
    srcs = ["ssestreamwriter_test1.cc"],
    deps = [
    	"//trpc/server/http_sse:ssestreamwriter",
        "//trpc/codec/http_sse:http_sse_codec",     
        "//trpc/util/http/sse:http_sse",
        "//trpc/server:server_context",
        "//trpc/util:ref_ptr",
        "//trpc/filter:server_filter_controller_h",
        "//trpc/util/buffer:noncontiguous_buffer", 
        "//trpc/util/http/sse:http_sse_parser", 
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)
cc_test(
    name = "HttpSseService_test",
    srcs = ["HttpSseService_test.cc"],
    deps = [
        "//trpc/server/http_sse:HttpSseService",
        "//trpc/server/http_sse:ssestreamwriter",
        "//trpc/server:server_context",
        "//trpc/filter:server_filter_controller_h",
        "//trpc/util:ref_ptr",
        "//trpc/util/http/sse:http_sse",
        "//trpc/util/http/sse:http_sse_parser",
        "//trpc/util/log:logging",
        "//trpc/util/http:request",
        "//trpc/util/http:response",
        "@com_google_googletest//:gtest",
        "@com_google_googletest//:gtest_main",
    ],
)

