//
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
//

#include "trpc/stream/http_sse/http_sse_stream_handler_factory.h"

#include <algorithm>

#include "trpc/util/log/logging.h"

namespace trpc::stream::http_sse {

// Initialize static members
size_t HttpSseStreamHandlerFactory::default_max_streams_ = 1000;
bool HttpSseStreamHandlerFactory::default_auto_cleanup_ = true;
bool HttpSseStreamHandlerFactory::default_fiber_mode_ = true;

HttpSseStreamHandlerPtr HttpSseStreamHandlerFactory::CreateHandler(StreamOptions&& options) {
  try {
    // Apply default options if not set
    if (options.max_streams == 0) {
      options.max_streams = default_max_streams_;
    }
    
    // Create new SSE stream handler
    auto handler = MakeRefCounted<HttpSseStreamHandler>(std::move(options));
    
    // Configure handler with default settings
    handler->SetAutoCleanup(default_auto_cleanup_);
    handler->SetMaxStreams(default_max_streams_);
    
    TRPC_LOG_DEBUG("Created SSE stream handler with options: max_streams=" 
                   << options.max_streams << ", fiber_mode=" << options.fiber_mode);
    
    return handler;
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Failed to create SSE stream handler: " << e.what());
    return nullptr;
  }
}

bool HttpSseStreamHandlerFactory::SupportsProtocol(const std::string& protocol) const {
  // Convert to lowercase for case-insensitive comparison
  std::string lower_protocol = protocol;
  std::transform(lower_protocol.begin(), lower_protocol.end(), lower_protocol.begin(), ::tolower);
  
  // Support HTTP SSE protocol
  return lower_protocol == "http_sse" || 
         lower_protocol == "sse" || 
         lower_protocol == "text/event-stream";
}

StreamOptions HttpSseStreamHandlerFactory::GetDefaultOptions() {
  StreamOptions options;
  options.max_streams = default_max_streams_;
  options.fiber_mode = default_fiber_mode_;
  options.auto_cleanup = default_auto_cleanup_;
  
  // Set SSE-specific defaults
  options.stream_id = 0;
  options.connection_id = 0;
  
  return options;
}

void HttpSseStreamHandlerFactory::SetDefaultConfig(size_t max_streams, bool auto_cleanup, bool fiber_mode) {
  default_max_streams_ = max_streams;
  default_auto_cleanup_ = auto_cleanup;
  default_fiber_mode_ = fiber_mode;
  
  TRPC_LOG_INFO("Updated SSE stream handler default config: max_streams=" 
                << max_streams << ", auto_cleanup=" << auto_cleanup 
                << ", fiber_mode=" << fiber_mode);
}

}  // namespace trpc::stream::http_sse
