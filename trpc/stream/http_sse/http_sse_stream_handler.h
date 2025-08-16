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
#include <unordered_map>
#include <mutex>

#include "trpc/stream/http/http_stream_handler.h"
#include "trpc/stream/http_sse/http_sse_stream.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/coroutine/fiber_mutex.h"
#include "trpc/coroutine/fiber_condition_variable.h"

namespace trpc::stream::http_sse {

/// @brief HTTP SSE stream handler for managing multiple SSE streams.
/// @note This class extends the existing HTTP stream handler to provide
///       SSE-specific stream management capabilities.
class HttpSseStreamHandler : public http::HttpStreamHandler {
 public:
  explicit HttpSseStreamHandler(StreamOptions&& options);
  ~HttpSseStreamHandler() override = default;

  /// @brief Initialize the SSE stream handler.
  /// @return true if successful, false otherwise.
  bool Init() override;

  /// @brief Stop the SSE stream handler.
  void Stop() override;

  /// @brief Wait for all streams to finish.
  void Join() override;

  /// @brief Remove a stream by its ID.
  /// @param stream_id The ID of the stream to remove.
  /// @return 0 on success, negative value on failure.
  int RemoveStream(uint32_t stream_id) override;

  /// @brief Check if a stream ID represents a new stream.
  /// @param stream_id The stream ID to check.
  /// @param frame_type The frame type.
  /// @return true if it's a new stream, false otherwise.
  bool IsNewStream(uint32_t stream_id, uint32_t frame_type) override;

  /// @brief Push a message to the appropriate stream.
  /// @param message The message to push.
  /// @param metadata The message metadata.
  void PushMessage(std::any&& message, ProtocolMessageMetadata&& metadata) override;

  /// @brief Get mutable stream options.
  /// @return Pointer to stream options.
  StreamOptions* GetMutableStreamOptions() override { return &options_; }

  /// @brief Encode transport message.
  /// @param msg The message to encode.
  /// @return 0 on success, negative value on failure.
  int EncodeTransportMessage(IoMessage* msg) override { return 0; }

  /// @brief Create a new SSE stream.
  /// @param stream_id The ID for the new stream.
  /// @return Pointer to the created stream, or nullptr on failure.
  HttpSseStreamPtr CreateStream(uint32_t stream_id);

  /// @brief Get an existing stream by ID.
  /// @param stream_id The stream ID.
  /// @return Pointer to the stream, or nullptr if not found.
  HttpSseStreamPtr GetStream(uint32_t stream_id);

  /// @brief Get all active streams.
  /// @return Vector of all active stream pointers.
  std::vector<HttpSseStreamPtr> GetAllStreams();

  /// @brief Get the count of active streams.
  /// @return Number of active streams.
  size_t GetStreamCount() const;

  /// @brief Set stream options for new streams.
  /// @param options The stream options to set.
  void SetStreamOptions(const StreamOptions& options);

  /// @brief Enable or disable automatic stream cleanup.
  /// @param enable true to enable, false to disable.
  void SetAutoCleanup(bool enable);

  /// @brief Set the maximum number of concurrent streams.
  /// @param max_streams Maximum number of streams.
  void SetMaxStreams(size_t max_streams);

 protected:
  /// @brief Handle stream creation.
  /// @param stream_id The stream ID.
  /// @return true if successful, false otherwise.
  virtual bool HandleStreamCreation(uint32_t stream_id);

  /// @brief Handle stream removal.
  /// @param stream_id The stream ID.
  /// @return true if successful, false otherwise.
  virtual bool HandleStreamRemoval(uint32_t stream_id);

  /// @brief Clean up inactive streams.
  void CleanupInactiveStreams();

  /// @brief Get network error code.
  /// @return Network error code.
  virtual int GetNetworkErrorCode() { return -1; }

 private:
  /// @brief Remove stream internally (thread-safe).
  /// @param stream_id The stream ID.
  /// @return 0 on success, negative value on failure.
  int RemoveStreamInner(uint32_t stream_id);

  /// @brief Critical section execution (fiber-aware).
  /// @param cb The callback to execute.
  /// @return The result of the callback.
  template <class T>
  T CriticalSection(std::function<T()>&& cb) const {
    if (options_.fiber_mode) {
      std::unique_lock<FiberMutex> _(mutex_);
      return cb();
    } else {
      return cb();
    }
  }

 private:
  // Stream options
  StreamOptions options_;
  
  // Stream management
  std::unordered_map<uint32_t, HttpSseStreamPtr> streams_;
  
  // Threading and synchronization
  mutable FiberMutex mutex_;
  FiberConditionVariable stream_cond_;
  
  // Configuration
  bool auto_cleanup_{true};
  size_t max_streams_{1000};
  
  // State
  bool conn_closed_{false};
  bool initialized_{false};
};

using HttpSseStreamHandlerPtr = RefPtr<HttpSseStreamHandler>;

}  // namespace trpc::stream::http_sse
