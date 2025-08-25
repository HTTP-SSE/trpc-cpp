load("@rules_cc//cc:defs.bzl", "cc_binary")

# Stock Price SSE Server (tRPC HTTP-SSE)
cc_binary(
    name = "stock_price_sse_server",
    srcs = ["stock_price_sse_server.cc"],
    deps = [
        "@trpc_cpp//trpc/server:trpc_server",
        "@trpc_cpp//trpc/server:http_service",
        "@trpc_cpp//trpc/server/http_sse:HttpSseService",
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/common:runtime_manager",
        "@trpc_cpp//trpc/common/config:trpc_config",
        "@trpc_cpp//trpc/codec:codec_manager",
        "@trpc_cpp//trpc/runtime:runtime",
        "@trpc_cpp//trpc/naming:trpc_naming_registry",
        "@trpc_cpp//trpc/util/http:http_parser",
        "@trpc_cpp//trpc/util/http:response",
        "@trpc_cpp//trpc/util/http:request",
        "@trpc_cpp//trpc/util/http:http_handler",
        "@trpc_cpp//trpc/util/http:routes",
        "@trpc_cpp//trpc/util/http/sse:http_sse",
        "@trpc_cpp//trpc/util/http/sse:http_sse_parser",
        "@trpc_cpp//trpc/util/buffer:noncontiguous_buffer",
        "@trpc_cpp//trpc/common:status",
        "@trpc_cpp//trpc/log:logging",
    ],
    data = ["trpc_merge.yaml"],
)

# Stock Price SSE Client (tRPC HTTP-SSE)
cc_binary(
    name = "stock_price_sse_client",
    srcs = ["stock_price_sse_client.cc"],
    deps = [
        "//trpc/client/http:http_service_proxy",
        "//trpc/client:trpc_client",
        "//trpc/client:make_client_context",
        "//trpc/client:client_context",
        "//trpc/client:service_proxy_option",

        "//trpc/common/config:trpc_config",
        "//trpc/codec:codec_manager",
        "//trpc/runtime:runtime",
        "//trpc/common:runtime_manager",
        "//trpc/naming:trpc_naming_registry",
        "//trpc/util/buffer:noncontiguous_buffer",
        "//trpc/common:status",
        "//trpc/common:trpc_app",

    ],
    data = ["trpc_merge.yaml"],
)


