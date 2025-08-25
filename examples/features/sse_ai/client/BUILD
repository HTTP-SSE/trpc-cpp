licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "client",
    srcs = ["client.cc"],
    data = [
        "trpc_cpp_fiber.yaml",
        "trpc_cpp_fiber_quiet.yaml",
    ],
    linkopts = ["-ldl"],
    deps = [
        "@com_github_gflags_gflags//:gflags",
        "@trpc_cpp//trpc/client/http:http_service_proxy",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/codec/http_sse:http_sse_codec",
        "@trpc_cpp//trpc/codec:client_codec_factory",
        "@trpc_cpp//trpc/common/config:trpc_config",
        "@trpc_cpp//trpc/common:runtime_manager",
        "@trpc_cpp//trpc/common:status",
        "@trpc_cpp//trpc/common:trpc_plugin",
        "@trpc_cpp//trpc/coroutine:fiber",
        "@trpc_cpp//trpc/stream/http:http_client_stream",
        "@trpc_cpp//trpc/util/http/sse:http_sse",
        "@trpc_cpp//trpc/util/http/sse:http_sse_parser",
        "@trpc_cpp//trpc/util/http:util",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)

cc_binary(
    name = "sse_proxy_demo",
    srcs = ["sse_proxy_demo.cc"],
    data = [
        "trpc_cpp_fiber.yaml",
        "trpc_cpp_fiber_quiet.yaml",
    ],
    linkopts = ["-ldl"],
    deps = [
        "@com_github_gflags_gflags//:gflags",
        "@trpc_cpp//trpc/client/http:http_service_proxy",
        "@trpc_cpp//trpc/client:make_client_context",
        "@trpc_cpp//trpc/client:trpc_client",
        "@trpc_cpp//trpc/client/sse:http_sse_stream_proxy",
        "@trpc_cpp//trpc/codec/http_sse:http_sse_codec",
        "@trpc_cpp//trpc/codec:client_codec_factory",
        "@trpc_cpp//trpc/common/config:trpc_config",
        "@trpc_cpp//trpc/common:runtime_manager",
        "@trpc_cpp//trpc/common:status",
        "@trpc_cpp//trpc/common:trpc_plugin",
        "@trpc_cpp//trpc/coroutine:fiber",
        "@trpc_cpp//trpc/stream/http:http_client_stream",
        "@trpc_cpp//trpc/util/http/sse:http_sse",
        "@trpc_cpp//trpc/util/http/sse:http_sse_parser",
        "@trpc_cpp//trpc/util/http:util",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)