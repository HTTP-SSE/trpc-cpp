load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

package(default_visibility = ["//visibility:public"])

cc_library(
    name = "http_sse_stream_proxy",
    srcs = ["http_sse_stream_proxy.cc"],
    hdrs = ["http_sse_stream_proxy.h"],
    deps = [
       
     
    
     	"//trpc/runtime/threadmodel/merge:merge_thread_model",
     
     	"//trpc/client/http:http_stream_proxy",
     
     	
	
    ],
)
cc_test(
    name = "http_sse_stream_proxy_test",
    srcs = ["http_sse_stream_proxy_test.cc"],
    deps = [
        ":http_sse_stream_proxy",
        "//trpc/server/http:http_async_stream_service",
        "//trpc/common/future:future",
        "//trpc/util/http:request",
        "//trpc/util/http:response",
        "@com_google_googletest//:gtest_main",
    ],
   
)

