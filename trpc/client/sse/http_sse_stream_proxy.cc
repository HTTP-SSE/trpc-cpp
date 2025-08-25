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

#include <iostream>
#include <sstream>

#include "trpc/client/trpc_client.h"
#include "trpc/codec/client_codec_factory.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/http/util.h"
#include "trpc/util/log/logging.h"

namespace trpc {

// ===== HttpSseStreamProxy Implementation =====

void HttpSseStreamProxy::SetServiceProxyOption(const ::trpc::ServiceProxyOption& options) {
  options_ = options;
  TRPC_LOG_DEBUG("Set service proxy options for HttpSseStreamProxy: " << options.name);
}

::trpc::stream::HttpClientStreamReaderWriter HttpSseStreamProxy::GetSseStream(const ::trpc::ClientContextPtr& ctx, 
                                                                      const std::string& url) {
  auto proxy = GetHttpProxy();
  if (!proxy) {
    TRPC_LOG_ERROR("Failed to get HTTP proxy for SSE stream");
    // Create a null stream provider and return it
    ::trpc::stream::HttpClientStreamPtr null_provider = nullptr;
    return ::trpc::stream::HttpClientStreamReaderWriter(null_provider);
  }

  // Setup SSE-specific headers
  SetupSseHeaders(ctx);

  // Use HttpServiceProxy::Get() directly for streaming
  return proxy->Get(ctx, url);
}

::trpc::Status HttpSseStreamProxy::ConnectAndReceive(const ::trpc::ClientContextPtr& ctx,
                                           const std::string& url,
                                           const SseEventCallback& callback) {
  if (!callback) {
    return ::trpc::Status(static_cast<int>(::trpc::codec::ClientRetCode::ENCODE_ERROR), 0, "Invalid SSE event callback");
  }

  auto proxy = GetHttpProxy();
  if (!proxy) {
    return ::trpc::Status(static_cast<int>(::trpc::codec::ClientRetCode::ENCODE_ERROR), 0, "Failed to get HTTP proxy");
  }

  // Use the same direct streaming approach as the working original client
  auto stream_rw = proxy->Get(ctx, url);
  
  // Check stream status
  if (!stream_rw.GetStatus().OK()) {
    TRPC_LOG_ERROR("Failed to create SSE stream: " << stream_rw.GetStatus().ToString());
    return stream_rw.GetStatus();
  }
  
  TRPC_LOG_INFO("SSE stream established, reading events...");
  
  // Process SSE events from stream using the same approach as working client
  return ProcessSseStream(stream_rw, callback);
}

::trpc::Status HttpSseStreamProxy::SendRequest(const ::trpc::ClientContextPtr& ctx,
                                     const std::string& url,
                                     std::string& response_content) {
  auto proxy = GetHttpProxy();
  if (!proxy) {
    return ::trpc::Status(static_cast<int>(::trpc::codec::ClientRetCode::ENCODE_ERROR), 0, "Invalid HTTP service proxy");
  }

  // Setup SSE-specific headers
  SetupSseHeaders(ctx);

  // Send SSE request using Get2 method to get HttpResponse
  ::trpc::http::HttpResponse response;
  ::trpc::Status status = proxy->Get2(ctx, url, &response);
  
  if (!status.OK()) {
    TRPC_LOG_ERROR("Failed to send SSE request: " << status.ToString());
    return status;
  }

  // Get response content
  response_content = response.GetContent();
  
  TRPC_LOG_INFO("Received response: " << response_content.length() << " bytes");
  TRPC_LOG_INFO("Response status: " << response.GetStatus());
  TRPC_LOG_INFO("Content-Type: " << response.GetHeader("Content-Type"));

  return ::trpc::Status();
}

::trpc::ClientContextPtr HttpSseStreamProxy::CreateSseContext(const std::string& url, uint32_t timeout_ms) {
  auto proxy = GetHttpProxy();
  if (!proxy) {
    TRPC_LOG_ERROR("Failed to get HTTP proxy for creating SSE context");
    return nullptr;
  }

  // Create client context using the same approach as working original client
  auto ctx = ::trpc::MakeClientContext(proxy);
  ctx->SetTimeout(timeout_ms);
  
  // Setup SSE-specific headers (same as working original client)
  ctx->SetHttpHeader("Accept", "text/event-stream");
  ctx->SetHttpHeader("Cache-Control", "no-cache");
  ctx->SetHttpHeader("Connection", "keep-alive");
  
  TRPC_LOG_INFO("Created SSE context for URL: " << url << " with timeout: " << timeout_ms << "ms");
  
  return ctx;
}

void HttpSseStreamProxy::InitializeSseProxy() {
  // Register SSE codec if not already registered
  auto* codec_factory = ::trpc::ClientCodecFactory::GetInstance();
  if (codec_factory && !codec_factory->Get("http_sse")) {
    codec_factory->Register(std::make_shared<::trpc::HttpSseClientCodec>());
    TRPC_LOG_INFO("Registered HTTP SSE codec");
  }
  
  TRPC_LOG_INFO("HttpSseStreamProxy initialized");
}

void HttpSseStreamProxy::SetupSseHeaders(const ::trpc::ClientContextPtr& ctx) {
  if (!ctx) {
    return;
  }

  // Set standard SSE headers
  ctx->SetHttpHeader("Accept", "text/event-stream");
  ctx->SetHttpHeader("Cache-Control", "no-cache");
  ctx->SetHttpHeader("Connection", "keep-alive");
  
  TRPC_LOG_DEBUG("SSE headers configured");
}

bool HttpSseStreamProxy::ParseSseContent(const std::string& content, const SseEventCallback& callback) {
  if (content.empty() || !callback) {
    return false;
  }

  try {
    // Use tRPC's SSE parser
    std::vector<::trpc::http::sse::SseEvent> events = ::trpc::http::sse::SseParser::ParseEvents(content);
    
    TRPC_LOG_DEBUG("Parsed " << events.size() << " SSE events");
    
    // Invoke callback for each event
    for (const auto& event : events) {
      TRPC_LOG_TRACE("SSE Event - Type: " << event.event_type << ", Data: " << event.data);
      
      // Call user callback, stop if it returns false
      if (!callback(event)) {
        TRPC_LOG_DEBUG("SSE event processing stopped by callback");
        break;
      }
    }
    
    return true;
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Failed to parse SSE content: " << e.what());
    return false;
  }
}

std::shared_ptr<::trpc::HttpRequestProtocol> HttpSseStreamProxy::CreateSseRequestProtocol(
    const ::trpc::ClientContextPtr& ctx, const std::string& url) {
  if (!ctx) {
    return nullptr;
  }

  auto request = std::make_shared<::trpc::HttpRequestProtocol>();
  if (!request) {
    return nullptr;
  }

  // Set request method and URL
  request->request->SetMethod("GET");
  request->request->SetUrl(url);
  
  // Set SSE-specific headers
  request->request->SetHeader("Accept", "text/event-stream");
  request->request->SetHeader("Cache-Control", "no-cache");
  request->request->SetHeader("Connection", "keep-alive");
  
  TRPC_LOG_DEBUG("Created SSE request protocol for URL: " << url);
  
  return request;
}

::trpc::Status HttpSseStreamProxy::ProcessSseStream(::trpc::stream::HttpClientStreamReaderWriter& stream_rw,
                                          const SseEventCallback& callback) {
  if (!callback) {
    return ::trpc::Status(static_cast<int>(::trpc::codec::ClientRetCode::ENCODE_ERROR), 0, "Invalid SSE event callback");
  }

  try {
    ::trpc::NoncontiguousBuffer buffer;
    std::string accumulated_data;
    
    // Read from stream and accumulate data
    while (true) {
      auto status = stream_rw.Read(buffer, 5000);  // 5 second timeout per read
      
      if (!status.OK()) {
        // Check if it's end of stream
        if (status.GetFrameworkRetCode() == static_cast<int>(::trpc::stream::StreamStatus::kStreamEof)) {
          TRPC_LOG_INFO("SSE stream ended normally (EOF)");
          break;
        } else {
          TRPC_LOG_ERROR("Failed to read from SSE stream: " << status.ToString());
          return ::trpc::Status(static_cast<int>(::trpc::codec::ClientRetCode::NETWORK_ERROR), 0, 
                       "SSE stream read error: " + status.ToString());
        }
      }
      
      // Convert buffer to string and accumulate
      std::string chunk = ::trpc::FlattenSlow(buffer);
      if (!chunk.empty()) {
        accumulated_data += chunk;
        
        // Try to parse complete SSE events from accumulated data
        size_t last_event_end = 0;
        size_t pos = 0;
        
        // Look for complete SSE events (ending with \n\n)
        while ((pos = accumulated_data.find("\n\n", last_event_end)) != std::string::npos) {
          std::string event_data = accumulated_data.substr(last_event_end, pos - last_event_end + 2);
          
          // Parse and process this complete event
          if (!ParseSseContent(event_data, callback)) {
            TRPC_LOG_WARN("Failed to parse SSE event data");
          }
          
          last_event_end = pos + 2;  // Move past \n\n
        }
        
        // Keep unprocessed data for next iteration
        if (last_event_end > 0) {
          accumulated_data = accumulated_data.substr(last_event_end);
        }
      }
      
      // Clear buffer for next read
      buffer.Clear();
    }
    
    // Process any remaining data
    if (!accumulated_data.empty()) {
      ParseSseContent(accumulated_data, callback);
    }
    
    return ::trpc::Status();
    
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Exception in SSE stream processing: " << e.what());
    return ::trpc::Status(static_cast<int>(::trpc::codec::ClientRetCode::DECODE_ERROR), 0, 
                 "SSE stream processing exception: " + std::string(e.what()));
  }
}

std::shared_ptr<::trpc::http::HttpServiceProxy> HttpSseStreamProxy::GetHttpProxy() {
  if (!http_proxy_) {
    // Use GetTrpcClient()->GetProxy to create the HTTP service proxy (same as working original client)
    http_proxy_ = ::trpc::GetTrpcClient()->GetProxy<::trpc::http::HttpServiceProxy>(options_.name, options_);
    
    if (http_proxy_) {
      TRPC_LOG_DEBUG("Created HTTP service proxy for SSE using GetTrpcClient()->GetProxy");
    } else {
      TRPC_LOG_ERROR("Failed to create HTTP service proxy using GetTrpcClient()->GetProxy");
    }
  }
  
  return http_proxy_;
}

// ===== Factory Function =====

std::shared_ptr<HttpSseStreamProxy> CreateHttpSseStreamProxy(const ::trpc::ServiceProxyOption& options) {
  auto proxy = std::make_shared<HttpSseStreamProxy>();
  if (proxy) {
    proxy->SetServiceProxyOption(options);
    proxy->InitializeSseProxy();
    TRPC_LOG_INFO("Created HttpSseStreamProxy with service: " << options.name);
  } else {
    TRPC_LOG_ERROR("Failed to create HttpSseStreamProxy");
  }
  
  return proxy;
}

}  // namespace trpc