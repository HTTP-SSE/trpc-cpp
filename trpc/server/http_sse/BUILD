# trpc/server/http_sse/BUILD
cc_library(
    name = "ssestreamwriter",
    hdrs = [
        "ssestreamwriter.h",
    ],
    visibility = ["//visibility:public"],
    deps = [
         "//trpc/codec/http_sse:http_sse_codec",
         "//trpc/util/http/sse:http_sse",
         "//trpc/util:ref_ptr",
          "//trpc/util/http/sse:http_sse_parser", 
         "//trpc/server:server_context_h",
         "//trpc/util/buffer:noncontiguous_buffer",
    ],
)

cc_library(
    name = "HttpSseService",
    hdrs = [
        "HttpSseService.h",
    ],
    srcs = [
        "HttpSseService.cc",
    ],
    visibility = ["//visibility:public"],
    deps = [
        "//trpc/server:server_context_h",
        "//trpc/server/http_sse:ssestreamwriter",
        "//trpc/util:ref_ptr",
        "//trpc/util/http/sse:http_sse",
        "//trpc/util/log:logging",
    ],
)
