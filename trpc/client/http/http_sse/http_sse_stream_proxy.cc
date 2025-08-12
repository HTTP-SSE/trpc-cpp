
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

//#include "http_sse_stream_proxy.h"


#include "trpc/client/http/http_sse/http_sse_stream_proxy.h"
#include "trpc/codec/http/http_protocol.h" 
namespace trpc::stream {

Future<HttpClientAsyncStreamReaderWriterPtr> HttpSseStreamProxy::GetAsyncStreamReaderWriter(const ClientContextPtr& ctx) {
  SetupSseParameters(ctx);
  return HttpStreamProxy::GetAsyncStreamReaderWriter(ctx);
}

void HttpSseStreamProxy::SetupSseParameters(const ClientContextPtr& ctx) {
  if (!ctx->GetRequest()) {
    ctx->SetRequest(codec_->CreateRequestPtr());
  }
  
  // 使用完全限定的类型名
  auto* req = dynamic_cast<trpc::HttpRequestProtocol*>(ctx->GetRequest().get());
  TRPC_ASSERT(req && "Invalid HTTP request protocol");
  
  // 获取HTTP请求对象指针
  auto http_request = req->request;
  
  // 设置SSE必需的头
  http_request->SetHeader("Accept", "text/event-stream");
  http_request->SetHeader("Cache-Control", "no-cache");
  http_request->SetHeader("Connection", "keep-alive");
  
  // 设置长连接超时（可选）
  ctx->SetTimeout(60000);  // 60秒超时
}

}  // namespace trpc::stream
