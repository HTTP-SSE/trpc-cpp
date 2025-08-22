# trpc/codec/http_sse/BUILD
cc_library(
    name = "http_sse_codec",
    srcs = [
        "http_sse_codec.cc",
       
    ],
    hdrs = [
        "http_sse_codec.h",
        
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//trpc/codec/http:http_client_codec",
        "//trpc/codec/http:http_server_codec",
        "//trpc/codec/http:http_protocol",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/common:status",
        "//trpc/log:trpc_log",
        "//trpc/util/http/sse:http_sse",
    ],
)
