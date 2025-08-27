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

#pragma once

#include <memory>
#include <string>
#include <functional>

#include "trpc/client/http/http_service_proxy.h"
#include "trpc/client/make_client_context.h"
#include "trpc/client/service_proxy_option.h"
#include "trpc/client/client_context.h"
#include "trpc/codec/http/http_protocol.h"
#include "trpc/codec/http_sse/http_sse_codec.h"
#include "trpc/common/status.h"
#include "trpc/stream/http/http_client_stream.h"
#include "trpc/util/http/sse/sse_event.h"
#include "trpc/util/http/sse/sse_parser.h"

namespace trpc {

/// @brief SSE Event callback function type
/// @param event The SSE event received
/// @return true to continue reading, false to stop
using SseEventCallback = std::function<bool(const ::trpc::http::sse::SseEvent& event)>;

/// @brief HTTP SSE Stream Proxy - A specialized proxy for Server-Sent Events
/// @note Integrates with tRPC architecture while providing SSE-specific functionality
class HttpSseStreamProxy {
 public:
  HttpSseStreamProxy() = default;
  ~HttpSseStreamProxy() = default;

  /// @brief Initialize the SSE proxy with service proxy options
  /// @param options Service proxy configuration options
  void SetServiceProxyOption(const ::trpc::ServiceProxyOption& options);

  /// @brief Get streaming connection for SSE
  /// @param ctx Client context containing request configuration
  /// @param url SSE endpoint URL (e.g., "/ai/chat?question=hello")
  /// @return HttpClientStreamReaderWriter for reading SSE events
  ::trpc::stream::HttpClientStreamReaderWriter GetSseStream(const ::trpc::ClientContextPtr& ctx, const std::string& url);

  /// @brief Connect to SSE endpoint and process events with callback
  /// @param ctx Client context containing request configuration
  /// @param url SSE endpoint URL (e.g., "/ai/chat?question=hello")
  /// @param callback Function to call for each received SSE event
  /// @return Status indicating success or failure
  ::trpc::Status ConnectAndReceive(const ::trpc::ClientContextPtr& ctx, 
                          const std::string& url,
                          const SseEventCallback& callback);

  /// @brief Send SSE request and get complete response (non-streaming)
  /// @param ctx Client context containing request configuration
  /// @param url SSE endpoint URL (e.g., "/ai/chat?question=hello")
  /// @param response_content Output parameter to store complete response
  /// @return Status indicating success or failure
  ::trpc::Status SendRequest(const ::trpc::ClientContextPtr& ctx,
                    const std::string& url,
                    std::string& response_content);

  /// @brief Create and configure SSE-specific client context
  /// @param url SSE endpoint URL
  /// @param timeout_ms Connection timeout in milliseconds (default: 60000)
  /// @return Configured client context for SSE requests
  ::trpc::ClientContextPtr CreateSseContext(const std::string& url, uint32_t timeout_ms = 60000);

  /// @brief Initialize SSE-specific proxy settings
  void InitializeSseProxy();

 protected:
  /// @brief Setup SSE-specific headers for HTTP request
  /// @param ctx Client context to configure
  void SetupSseHeaders(const ::trpc::ClientContextPtr& ctx);

  /// @brief Parse SSE content and invoke callback for each event
  /// @param content Raw SSE content from server
  /// @param callback Function to call for each parsed event
  /// @return true if parsing was successful, false otherwise
  bool ParseSseContent(const std::string& content, const SseEventCallback& callback);

  /// @brief Create HTTP request protocol for SSE
  /// @param ctx Client context
  /// @param url Request URL
  /// @return HTTP request protocol
  std::shared_ptr<::trpc::HttpRequestProtocol> CreateSseRequestProtocol(const ::trpc::ClientContextPtr& ctx, 
                                                                     const std::string& url);

  /// @brief Process streaming SSE data from HttpClientStreamReaderWriter
  /// @param stream_rw The stream reader/writer
  /// @param callback Function to call for each parsed event
  /// @return Status indicating success or failure
  ::trpc::Status ProcessSseStream(::trpc::stream::HttpClientStreamReaderWriter& stream_rw, 
                         const SseEventCallback& callback);

  /// @brief Get service proxy options
  /// @return Current service proxy options
  const ::trpc::ServiceProxyOption& GetServiceProxyOption() const { return options_; }

 private:
  /// @brief Get or create underlying HTTP service proxy
  /// @return HTTP service proxy for actual network communication
  std::shared_ptr<::trpc::http::HttpServiceProxy> GetHttpProxy();

  ::trpc::ServiceProxyOption options_;                              ///< Service proxy configuration options
  std::shared_ptr<::trpc::http::HttpServiceProxy> http_proxy_;     ///< Underlying HTTP proxy for communication
};

/// @brief Factory function to create HttpSseStreamProxy
/// @param options Service proxy options
/// @return Shared pointer to HttpSseStreamProxy
std::shared_ptr<HttpSseStreamProxy> CreateHttpSseStreamProxy(const ::trpc::ServiceProxyOption& options);

}  // namespace trpc
