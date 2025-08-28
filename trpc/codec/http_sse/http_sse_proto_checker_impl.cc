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

#include "trpc/codec/http_sse/http_sse_proto_checker.h"

#include "trpc/codec/http/http_proto_checker.h"
#include "trpc/util/log/logging.h"

namespace trpc {

int HttpSseZeroCopyCheckRequest(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
  // Use the base HTTP request checker
  int result = HttpZeroCopyCheckRequest(conn, in, out);
  
  if (result == kPacketFull) {
    // Validate that the requests are valid SSE requests
    for (auto& request_any : out) {
      try {
        auto request_ptr = std::any_cast<http::RequestPtr>(request_any);
        if (!IsValidSseRequest(request_ptr.get())) {
          TRPC_LOG_WARN("Invalid SSE request detected, but continuing processing");
          // Don't fail here, just log a warning
        }
      } catch (const std::exception& e) {
        TRPC_LOG_ERROR("Failed to validate SSE request: " << e.what());
        return kPacketError;
      }
    }
  }
  
  return result;
}

int HttpSseZeroCopyCheckResponse(const ConnectionPtr& conn, NoncontiguousBuffer& in, std::deque<std::any>& out) {
  // Use the base HTTP response checker
  int result = HttpZeroCopyCheckResponse(conn, in, out);
  
  if (result == kPacketFull) {
    // Validate that the responses are valid SSE responses
    for (auto& response_any : out) {
      try {
        auto response = std::any_cast<http::Response>(response_any);
        if (!IsValidSseResponse(&response)) {
          TRPC_LOG_WARN("Invalid SSE response detected, but continuing processing");
          // Don't fail here, just log a warning
        }
      } catch (const std::exception& e) {
        TRPC_LOG_ERROR("Failed to validate SSE response: " << e.what());
        return kPacketError;
      }
    }
  }
  
  return result;
}

bool IsValidSseRequest(const http::Request* request) {
  if (!request) {
    return false;
  }

  // Check if Accept header includes text/event-stream
  std::string accept = request->GetHeader("Accept");
  if (accept.find("text/event-stream") == std::string::npos) {
    return false;
  }

  // SSE requests are typically GET requests
  if (request->GetMethod() != "GET") {
    return false;
  }

  return true;
}

bool IsValidSseResponse(const http::Response* response) {
  if (!response) {
    return false;
  }

  // Check if Content-Type is text/event-stream
  std::string content_type = response->GetHeader("Content-Type");
  if (content_type.find("text/event-stream") == std::string::npos) {
    return false;
  }

  // Check if Cache-Control is set to no-cache
  std::string cache_control = response->GetHeader("Cache-Control");
  if (cache_control.find("no-cache") == std::string::npos) {
    return false;
  }

  return true;
}

}  // namespace trpc
