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

#include "trpc/client/sse/http_sse_stream_proxy.h"

#include "trpc/codec/codec_helper.h"
#include "trpc/codec/http_sse/http_sse_codec.h"
#include "trpc/common/status.h"
#include "trpc/log/logging.h"
#include "trpc/stream/http/http_client_stream.h"
#include "trpc/util/http/method.h"

namespace trpc::stream {

HttpClientStreamReaderWriter HttpSseStreamProxy::GetSyncStreamReaderWriter(const ClientContextPtr& ctx) {
  if (!ctx) {
    TRPC_LOG_ERROR("SseStreamProxy: Invalid client context");
    return HttpClientStreamReaderWriter(MakeRefCounted<HttpClientStream>(
        Status(static_cast<int>(codec::ClientRetCode::ENCODE_ERROR), 0, "Invalid client context"), true));
  }

  // Setup SSE-specific parameters
  if (!SetupSseParameters(ctx)) {
    return HttpClientStreamReaderWriter(MakeRefCounted<HttpClientStream>(
        Status(static_cast<int>(codec::ClientRetCode::ENCODE_ERROR), 0, "Failed to setup SSE parameters"), true));
  }

  // Create request if not exists
  if (!ctx->GetRequest()) {
    ctx->SetRequest(CreateSseRequestProtocol());
  }

  // Cast to HTTP request protocol
  auto* req = dynamic_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  if (!req) {
    TRPC_LOG_ERROR("SseStreamProxy: Failed to cast to HttpRequestProtocol");
    return HttpClientStreamReaderWriter(MakeRefCounted<HttpClientStream>(
        Status(static_cast<int>(codec::ClientRetCode::ENCODE_ERROR), 0, "Invalid request protocol"), true));
  }

  // Fill client context with service proxy options
  FillClientContext(ctx);

  // Select stream provider using base ServiceProxy functionality
  auto stream_provider = ServiceProxy::SelectStreamProvider(ctx, nullptr);
  if (!stream_provider->GetStatus().OK()) {
    return HttpClientStreamReaderWriter(MakeRefCounted<HttpClientStream>(
        stream_provider->GetStatus(), true));
  }

  // Cast to HTTP client stream
  auto http_stream_provider = static_pointer_cast<HttpClientStream>(stream_provider);
  
  // Configure stream for SSE
  http_stream_provider->SetCapacity(GetServiceProxyOption()->max_packet_size > 0
                                       ? GetServiceProxyOption()->max_packet_size
                                       : std::numeric_limits<size_t>::max());
  http_stream_provider->SetMethod(http::MethodNameToMethodType(req->request->GetMethod()));
  http_stream_provider->SetHttpRequestProtocol(req);
  
  // Send request header to establish SSE connection
  http_stream_provider->SendRequestHeader();

  return HttpClientStreamReaderWriter(std::move(http_stream_provider));
}

bool HttpSseStreamProxy::SetupSseParameters(const ClientContextPtr& ctx) {
  if (!ctx) {
    TRPC_LOG_ERROR("SseStreamProxy: Invalid client context");
    return false;
  }

  // Create request if not exists
  if (!ctx->GetRequest()) {
    ctx->SetRequest(CreateSseRequestProtocol());
  }
  
  // Cast to HTTP request protocol
  auto* req = dynamic_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  if (!req) {
    TRPC_LOG_ERROR("SseStreamProxy: Failed to cast to HttpRequestProtocol");
    return false;
  }
  
  // Get HTTP request object
  auto http_request = req->request;
  if (!http_request) {
    TRPC_LOG_ERROR("SseStreamProxy: Invalid HTTP request object");
    return false;
  }
  
  // Set SSE-specific headers
  http_request->SetHeader("Accept", "text/event-stream");
  http_request->SetHeader("Cache-Control", "no-cache");
  http_request->SetHeader("Connection", "keep-alive");
  
  // Set SSE timeout (60 seconds for long-lived connections)
  ctx->SetTimeout(60000);
  
  return true;
}

ProtocolPtr HttpSseStreamProxy::CreateSseRequestProtocol() {
  // Create HTTP SSE request protocol using SSE codec
  auto protocol = std::make_shared<HttpSseRequestProtocol>();
  
  // Set default HTTP method for SSE (usually GET)
  protocol->request->SetMethod("GET");
  
  // Set default path if not specified
  if (protocol->request->GetUrl().empty()) {
    protocol->request->SetUrl("/");
  }
  
  return protocol;
}

}  // namespace trpc::stream
