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
#include <vector>
#include <optional>

#include "trpc/stream/http/http_stream.h"
#include "trpc/util/http/sse/sse_event.h"
#include "trpc/common/status.h"

namespace trpc::stream::http_sse {

/// @brief HTTP SSE stream for handling Server-Sent Events over HTTP streams.
/// @note This class extends the existing HTTP stream functionality to provide
///       SSE-specific streaming capabilities.
class HttpSseStream : public http::HttpStream {
 public:
  explicit HttpSseStream(StreamOptions&& options);
  ~HttpSseStream() override = default;

  /// @brief Send a single SSE event to the client.
  /// @param event The SSE event to send.
  /// @return Status of the operation.
  Status SendEvent(const http::sse::SseEvent& event);

  /// @brief Send multiple SSE events to the client.
  /// @param events Vector of SSE events to send.
  /// @return Status of the operation.
  Status SendEvents(const std::vector<http::sse::SseEvent>& events);

  /// @brief Send a comment line to the client.
  /// @param comment The comment to send.
  /// @return Status of the operation.
  Status SendComment(const std::string& comment);

  /// @brief Send a retry instruction to the client.
  /// @param retry_ms Retry timeout in milliseconds.
  /// @return Status of the operation.
  Status SendRetry(uint32_t retry_ms);

  /// @brief Check if the stream is ready to send events.
  /// @return true if ready, false otherwise.
  bool IsReady() const;

  /// @brief Get the last received event from the client (if any).
  /// @return Optional SSE event from the client.
  std::optional<http::sse::SseEvent> GetLastReceivedEvent() const;

  /// @brief Set a custom event ID generator function.
  /// @param generator Function that generates event IDs.
  void SetEventIdGenerator(std::function<std::string()> generator);

  /// @brief Enable or disable automatic event ID generation.
  /// @param enable true to enable, false to disable.
  void SetAutoEventId(bool enable);

  /// @brief Set the default event type for data-only events.
  /// @param event_type Default event type.
  void SetDefaultEventType(const std::string& event_type);

 protected:
  /// @brief Override to handle SSE-specific message processing.
  RetCode HandleData(StreamRecvMessage&& msg) override;

  /// @brief Override to handle SSE-specific message sending.
  RetCode SendData(StreamSendMessage&& msg) override;

  /// @brief Override to handle SSE-specific stream initialization.
  RetCode SendInit(StreamSendMessage&& msg) override;

  /// @brief Override to handle SSE-specific stream closure.
  RetCode SendClose(StreamSendMessage&& msg) override;

 private:
  /// @brief Generate a unique event ID.
  /// @return Generated event ID.
  std::string GenerateEventId();

  /// @brief Format SSE message for transmission.
  /// @param event The SSE event to format.
  /// @return Formatted SSE message.
  std::string FormatSseMessage(const http::sse::SseEvent& event);

  /// @brief Send raw SSE data over the stream.
  /// @param data The SSE data to send.
  /// @return Status of the operation.
  Status SendSseData(const std::string& data);

 private:
  // SSE-specific configuration
  std::function<std::string()> event_id_generator_;
  bool auto_event_id_{true};
  std::string default_event_type_{"message"};
  
  // SSE state
  std::optional<http::sse::SseEvent> last_received_event_;
  uint32_t event_counter_{0};
  
  // Stream state
  bool stream_ready_{false};
};

using HttpSseStreamPtr = RefPtr<HttpSseStream>;

}  // namespace trpc::stream::http_sse
