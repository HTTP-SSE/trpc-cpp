
//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//
#include "trpc/client/http/http_sse/http_sse_stream_proxy.h"

#include <tuple>

#include "trpc/client/service_proxy.h"
#include "trpc/runtime/threadmodel/merge/merge_thread_model.h"

namespace trpc::stream {

void HttpSseStreamProxy::PrepareSseRequest(HttpRequestProtocol* req) {
  TRPC_ASSERT(req && "unmatched codec for SSE");
  // Http::Request::SetMethod 接受 string/string_view，这里直接传 "GET"
  req->request->SetMethod("GET");
  req->request->AddHeader("Accept", "text/event-stream");
  req->request->AddHeader("Cache-Control", "no-cache");
  req->request->AddHeader("Connection", "keep-alive");
  // 禁用编码以避免压缩导致事件边界不可控（可按需移除）
  req->request->AddHeader("Accept-Encoding", "identity");
}

HttpClientAsyncStreamReaderWriterPtr HttpSseStreamProxy::GetStreamReaderWriter(const ClientContextPtr& ctx) {
  // 如果没有 request，则创建一个（与 HttpStreamProxy 行为一致）
  if (!ctx->GetRequest()) {
    ctx->SetRequest(codec_->CreateRequestPtr());
  }

  // 在进入过滤器前，补齐 SSE 头部，方便 filter 看到真实头部
  if (auto* req = dynamic_cast<HttpRequestProtocol*>(ctx->GetRequest().get())) {
    PrepareSseRequest(req);
  }

  // 运行前置过滤器（与异步版本一致）
  ctx->SetServiceProxyOption(ServiceProxy::GetMutableServiceProxyOption());
  auto filter_status = filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_PRE_RPC_INVOKE, ctx);
  if (filter_status == FilterStatus::REJECT) {
    Status status{TRPC_STREAM_UNKNOWN_ERR, 0, "filter PRE_RPC_INVOKE execute failed."};
    TRPC_FMT_ERROR(status.ErrorMessage());
    ctx->SetStatus(status);
    return nullptr;
  }

  filter_status = filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_PRE_SEND_MSG, ctx);
  if (filter_status == FilterStatus::REJECT) {
    Status status{TRPC_STREAM_UNKNOWN_ERR, 0, "filter PRE_SEND_MSG execute failed."};
    TRPC_FMT_ERROR(status.ErrorMessage());
    ctx->SetStatus(status);
    filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, ctx);
    return nullptr;
  }

  // 调用基类异步创建流的函数，得到 Future
  auto fut = HttpStreamProxy::GetAsyncStreamReaderWriter(ctx);

  // 同步等待（在 Fiber 环境下 Wait 会挂起当前 Fiber）
  if (!fut.Wait()) {
    TRPC_FMT_ERROR("SSE stream creation wait timeout or interrupted");
    // 设置状态（可根据需要细化）
    ctx->SetStatus(Status{TRPC_STREAM_CLIENT_NETWORK_ERR, 0, "SSE stream creation wait failed."});
    return nullptr;
  }

  if (fut.IsFailed()) {
    // 获取异常并记录
    auto ex = fut.GetException();
    TRPC_FMT_ERROR("SSE stream creation failed: {}", ex.what());
    ctx->SetStatus(Status{TRPC_STREAM_CLIENT_NETWORK_ERR, 0, std::string("SSE stream creation failed: ") + ex.what()});
    return nullptr;
  }

  // GetValue 返回 std::tuple<T>
  auto tuple_val = fut.GetValue();
  auto result = std::get<0>(tuple_val);

  // 如果需要，仍可在此运行后置过滤器（与异步版本一致）
  // filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RECV_MSG, ctx);
  // filter_controller_.RunMessageClientFilters(FilterPoint::CLIENT_POST_RPC_INVOKE, ctx);

  return result;
}

}  // namespace trpc::stream

