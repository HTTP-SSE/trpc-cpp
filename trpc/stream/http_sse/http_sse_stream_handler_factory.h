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

#pragma once

#include <memory>
#include <string>

#include "trpc/stream/stream_handler_factory.h"
#include "trpc/stream/http_sse/http_sse_stream_handler.h"

namespace trpc::stream::http_sse {

/// @brief Factory for creating HTTP SSE stream handlers.
/// @note This factory extends the existing stream handler factory to provide
///       SSE-specific stream handler creation capabilities.
class HttpSseStreamHandlerFactory : public StreamHandlerFactory {
 public:
  HttpSseStreamHandlerFactory() = default;
  ~HttpSseStreamHandlerFactory() override = default;

  /// @brief Create a new SSE stream handler.
  /// @param options The stream options for the handler.
  /// @return Pointer to the created stream handler, or nullptr on failure.
  HttpSseStreamHandlerPtr CreateHandler(StreamOptions&& options);

  /// @brief Get the name of this factory.
  /// @return Factory name.
  std::string GetFactoryName() const override { return "http_sse"; }

  /// @brief Check if this factory supports the given protocol.
  /// @param protocol The protocol to check.
  /// @return true if supported, false otherwise.
  bool SupportsProtocol(const std::string& protocol) const override;

  /// @brief Get the default stream options for SSE.
  /// @return Default stream options.
  static StreamOptions GetDefaultOptions();

  /// @brief Set default configuration for SSE streams.
  /// @param max_streams Maximum number of concurrent streams.
  /// @param auto_cleanup Whether to enable automatic cleanup.
  /// @param fiber_mode Whether to use fiber mode.
  static void SetDefaultConfig(size_t max_streams, bool auto_cleanup, bool fiber_mode);

 private:
  // Default configuration
  static size_t default_max_streams_;
  static bool default_auto_cleanup_;
  static bool default_fiber_mode_;
};

using HttpSseStreamHandlerFactoryPtr = RefPtr<HttpSseStreamHandlerFactory>;

}  // namespace trpc::stream::http_sse
