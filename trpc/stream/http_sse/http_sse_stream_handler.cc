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

#include "trpc/stream/http_sse/http_sse_stream_handler.h"

#include <algorithm>
#include <chrono>

#include "trpc/util/log/logging.h"
#include "trpc/coroutine/fiber.h"

namespace trpc::stream::http_sse {

HttpSseStreamHandler::HttpSseStreamHandler(StreamOptions&& options)
    : options_(std::move(options)) {
  // Initialize with default settings
  auto_cleanup_ = true;
  max_streams_ = 1000;
}

bool HttpSseStreamHandler::Init() {
  if (initialized_) {
    TRPC_LOG_WARN("SSE stream handler already initialized");
    return true;
  }

  try {
    // Initialize parent handler
    if (!http::HttpStreamHandler::Init()) {
      TRPC_LOG_ERROR("Failed to initialize parent HTTP stream handler");
      return false;
    }

    // Set up stream cleanup if enabled
    if (auto_cleanup_) {
      // Start cleanup fiber in fiber mode
      if (options_.fiber_mode) {
        Fiber::Start([this]() {
          while (!conn_closed_) {
            CleanupInactiveStreams();
            Fiber::SleepFor(std::chrono::seconds(30)); // Cleanup every 30 seconds
          }
        });
      }
    }

    initialized_ = true;
    TRPC_LOG_INFO("SSE stream handler initialized successfully");
    return true;
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Failed to initialize SSE stream handler: " << e.what());
    return false;
  }
}

void HttpSseStreamHandler::Stop() {
  if (!initialized_) {
    return;
  }

  try {
    conn_closed_ = true;
    
    // Stop all streams
    CriticalSection([this]() {
      for (auto& [stream_id, stream] : streams_) {
        if (stream) {
          stream->Close(Status(-1, "Handler stopped"));
        }
      }
      streams_.clear();
    });

    // Stop parent handler
    http::HttpStreamHandler::Stop();
    
    TRPC_LOG_INFO("SSE stream handler stopped");
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Error stopping SSE stream handler: " << e.what());
  }
}

void HttpSseStreamHandler::Join() {
  if (!initialized_) {
    return;
  }

  try {
    // Wait for all streams to finish
    CriticalSection([this]() {
      for (auto& [stream_id, stream] : streams_) {
        if (stream) {
          stream->Join();
        }
      }
    });

    // Join parent handler
    http::HttpStreamHandler::Join();
    
    TRPC_LOG_INFO("SSE stream handler joined");
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Error joining SSE stream handler: " << e.what());
  }
}

int HttpSseStreamHandler::RemoveStream(uint32_t stream_id) {
  return CriticalSection([this, stream_id]() {
    return RemoveStreamInner(stream_id);
  });
}

bool HttpSseStreamHandler::IsNewStream(uint32_t stream_id, uint32_t frame_type) {
  return CriticalSection([this, stream_id, frame_type]() {
    // Check if stream exists
    auto it = streams_.find(stream_id);
    if (it == streams_.end()) {
      // Check if we can create a new stream
      if (streams_.size() >= max_streams_) {
        TRPC_LOG_WARN("Maximum number of streams reached: " << max_streams_);
        return false;
      }
      return true;
    }
    return false;
  });
}

void HttpSseStreamHandler::PushMessage(std::any&& message, ProtocolMessageMetadata&& metadata) {
  try {
    uint32_t stream_id = metadata.stream_id;
    
    CriticalSection([this, &message, &metadata, stream_id]() {
      // Get or create stream
      auto it = streams_.find(stream_id);
      if (it == streams_.end()) {
        // Create new stream
        if (streams_.size() >= max_streams_) {
          TRPC_LOG_WARN("Maximum number of streams reached, dropping message for stream: " << stream_id);
          return;
        }
        
        auto stream = CreateStream(stream_id);
        if (!stream) {
          TRPC_LOG_ERROR("Failed to create stream for ID: " << stream_id);
          return;
        }
        
        it = streams_.emplace(stream_id, std::move(stream)).first;
      }
      
      // Push message to stream
      if (it->second) {
        it->second->PushMessage(std::move(message), std::move(metadata));
      }
    });
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Error pushing message to SSE stream: " << e.what());
  }
}

HttpSseStreamPtr HttpSseStreamHandler::CreateStream(uint32_t stream_id) {
  try {
    // Create stream options
    StreamOptions stream_options = options_;
    stream_options.stream_id = stream_id;
    
    // Create new SSE stream
    auto stream = MakeRefCounted<HttpSseStream>(std::move(stream_options));
    
    // Initialize stream
    if (!stream->Init()) {
      TRPC_LOG_ERROR("Failed to initialize SSE stream: " << stream_id);
      return nullptr;
    }
    
    TRPC_LOG_DEBUG("Created SSE stream: " << stream_id);
    return stream;
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Error creating SSE stream: " << e.what());
    return nullptr;
  }
}

HttpSseStreamPtr HttpSseStreamHandler::GetStream(uint32_t stream_id) {
  return CriticalSection([this, stream_id]() -> HttpSseStreamPtr {
    auto it = streams_.find(stream_id);
    if (it != streams_.end()) {
      return it->second;
    }
    return nullptr;
  });
}

std::vector<HttpSseStreamPtr> HttpSseStreamHandler::GetAllStreams() {
  return CriticalSection([this]() -> std::vector<HttpSseStreamPtr> {
    std::vector<HttpSseStreamPtr> result;
    result.reserve(streams_.size());
    
    for (auto& [stream_id, stream] : streams_) {
      if (stream) {
        result.push_back(stream);
      }
    }
    
    return result;
  });
}

size_t HttpSseStreamHandler::GetStreamCount() const {
  return CriticalSection([this]() -> size_t {
    return streams_.size();
  });
}

void HttpSseStreamHandler::SetStreamOptions(const StreamOptions& options) {
  CriticalSection([this, &options]() {
    options_ = options;
  });
}

void HttpSseStreamHandler::SetAutoCleanup(bool enable) {
  auto_cleanup_ = enable;
}

void HttpSseStreamHandler::SetMaxStreams(size_t max_streams) {
  max_streams_ = max_streams;
}

bool HttpSseStreamHandler::HandleStreamCreation(uint32_t stream_id) {
  try {
    TRPC_LOG_DEBUG("Handling SSE stream creation: " << stream_id);
    return true;
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Error handling stream creation: " << e.what());
    return false;
  }
}

bool HttpSseStreamHandler::HandleStreamRemoval(uint32_t stream_id) {
  try {
    TRPC_LOG_DEBUG("Handling SSE stream removal: " << stream_id);
    return true;
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Error handling stream removal: " << e.what());
    return false;
  }
}

void HttpSseStreamHandler::CleanupInactiveStreams() {
  try {
    CriticalSection([this]() {
      auto it = streams_.begin();
      while (it != streams_.end()) {
        if (!it->second || !it->second->IsReady()) {
          TRPC_LOG_DEBUG("Cleaning up inactive stream: " << it->first);
          it = streams_.erase(it);
        } else {
          ++it;
        }
      }
    });
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Error during stream cleanup: " << e.what());
  }
}

int HttpSseStreamHandler::RemoveStreamInner(uint32_t stream_id) {
  try {
    auto it = streams_.find(stream_id);
    if (it == streams_.end()) {
      TRPC_LOG_WARN("Stream not found for removal: " << stream_id);
      return -1;
    }
    
    // Close and remove stream
    if (it->second) {
      it->second->Close(Status(-1, "Stream removed"));
      it->second->Join();
    }
    
    streams_.erase(it);
    
    TRPC_LOG_DEBUG("Removed SSE stream: " << stream_id);
    return 0;
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Error removing stream: " << e.what());
    return -1;
  }
}

}  // namespace trpc::stream::http_sse
