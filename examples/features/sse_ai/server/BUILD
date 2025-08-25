licenses(["notice"])

package(default_visibility = ["//visibility:public"])

cc_binary(
    name = "sse_ai_server",
    srcs = ["sse_ai_server.cc"],
    data = [
        "trpc_cpp_fiber.yaml",
        "trpc_cpp_fiber_quiet.yaml",
    ],
    linkopts = ["-ldl"],
    deps = [
        "@trpc_cpp//trpc/common:trpc_app",
        "@trpc_cpp//trpc/server:http_service",
        "@trpc_cpp//trpc/server/http_sse:HttpSseService",
        "@trpc_cpp//trpc/util/http:function_handlers",
        "@trpc_cpp//trpc/util/http:http_handler",
        "@trpc_cpp//trpc/util/http:routes",
        "@trpc_cpp//trpc/util/http/sse:http_sse",
        "@trpc_cpp//trpc/util/log:logging",
    ],
)