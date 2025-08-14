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

#include "trpc/client/service_proxy.h"
#include "trpc/codec/http_sse/http_sse_codec.h"
#include "trpc/stream/http/http_client_stream.h"
#include "trpc/common/status.h"

namespace trpc::stream {

/// @brief HTTP SSE Stream Proxy for client-side SSE connections
/// @note Inherits from ServiceProxy to leverage existing tRPC infrastructure
class HttpSseStreamProxy : public ServiceProxy {
 public:
  HttpSseStreamProxy() = default;
  ~HttpSseStreamProxy() override = default;

  /// @brief Get synchronous stream reader/writer for SSE connection
  /// @param ctx Client context containing request/response data
  /// @return HttpClientStreamReaderWriter for reading SSE events
  HttpClientStreamReaderWriter GetSyncStreamReaderWriter(const ClientContextPtr& ctx);

 public:
  /// @brief Setup SSE-specific parameters (headers, timeout, etc.)
  /// @param ctx Client context to configure
  /// @return true if setup was successful, false otherwise
  bool SetupSseParameters(const ClientContextPtr& ctx);

  /// @brief Create SSE-specific request protocol
  /// @return Request protocol pointer
  ProtocolPtr CreateSseRequestProtocol();

 protected:
};

}  // namespace trpc::stream
