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

#include "trpc/stream/http_sse/http_sse_stream.h"

#include <sstream>
#include <chrono>
#include <random>

#include "trpc/util/log/logging.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/util/http/sse/sse_parser.h"

namespace trpc::stream::http_sse {

HttpSseStream::HttpSseStream(StreamOptions&& options) 
    : http::HttpStream(std::move(options)) {
  // Set default event ID generator
  event_id_generator_ = [this]() { return GenerateEventId(); };
}

Status HttpSseStream::SendEvent(const http::sse::SseEvent& event) {
  if (!IsReady()) {
    return Status(-1, "Stream is not ready");
  }

  try {
    std::string sse_data = FormatSseMessage(event);
    return SendSseData(sse_data);
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Failed to send SSE event: " << e.what());
    return Status(-1, "Failed to send SSE event");
  }
}

Status HttpSseStream::SendEvents(const std::vector<http::sse::SseEvent>& events) {
  if (!IsReady()) {
    return Status(-1, "Stream is not ready");
  }

  try {
    std::stringstream sse_data;
    for (const auto& event : events) {
      sse_data << FormatSseMessage(event);
    }
    return SendSseData(sse_data.str());
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Failed to send SSE events: " << e.what());
    return Status(-1, "Failed to send SSE events");
  }
}

Status HttpSseStream::SendComment(const std::string& comment) {
  if (!IsReady()) {
    return Status(-1, "Stream is not ready");
  }

  try {
    std::string sse_data = ":" + comment + "\n\n";
    return SendSseData(sse_data);
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Failed to send SSE comment: " << e.what());
    return Status(-1, "Failed to send SSE comment");
  }
}

Status HttpSseStream::SendRetry(uint32_t retry_ms) {
  if (!IsReady()) {
    return Status(-1, "Stream is not ready");
  }

  try {
    std::string sse_data = "retry: " + std::to_string(retry_ms) + "\n\n";
    return SendSseData(sse_data);
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Failed to send SSE retry: " << e.what());
    return Status(-1, "Failed to send SSE retry");
  }
}

bool HttpSseStream::IsReady() const {
  return stream_ready_ && http::HttpStream::IsReady();
}

std::optional<http::sse::SseEvent> HttpSseStream::GetLastReceivedEvent() const {
  return last_received_event_;
}

void HttpSseStream::SetEventIdGenerator(std::function<std::string()> generator) {
  if (generator) {
    event_id_generator_ = std::move(generator);
  }
}

void HttpSseStream::SetAutoEventId(bool enable) {
  auto_event_id_ = enable;
}

void HttpSseStream::SetDefaultEventType(const std::string& event_type) {
  default_event_type_ = event_type;
}

RetCode HttpSseStream::HandleData(StreamRecvMessage&& msg) {
  try {
    // Parse received data as SSE event
    if (msg.data && !msg.data->IsEmpty()) {
      std::string received_data = msg.data->ToString();
      auto event = http::sse::SseParser::ParseEvent(received_data);
      last_received_event_ = event;
      
      TRPC_LOG_DEBUG("Received SSE event: " << event.data);
    }
    
    // Call parent implementation
    return http::HttpStream::HandleData(std::move(msg));
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Failed to handle SSE data: " << e.what());
    return RetCode::kError;
  }
}

RetCode HttpSseStream::SendData(StreamSendMessage&& msg) {
  try {
    // Mark stream as ready when first data is sent
    if (!stream_ready_) {
      stream_ready_ = true;
    }
    
    // Call parent implementation
    return http::HttpStream::SendData(std::move(msg));
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Failed to send SSE data: " << e.what());
    return RetCode::kError;
  }
}

RetCode HttpSseStream::SendInit(StreamSendMessage&& msg) {
  try {
    // Mark stream as ready
    stream_ready_ = true;
    
    // Call parent implementation
    return http::HttpStream::SendInit(std::move(msg));
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Failed to send SSE init: " << e.what());
    return RetCode::kError;
  }
}

RetCode HttpSseStream::SendClose(StreamSendMessage&& msg) {
  try {
    // Mark stream as not ready
    stream_ready_ = false;
    
    // Call parent implementation
    return http::HttpStream::SendClose(std::move(msg));
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Failed to send SSE close: " << e.what());
    return RetCode::kError;
  }
}

std::string HttpSseStream::GenerateEventId() {
  if (event_id_generator_) {
    return event_id_generator_();
  }
  
  // Default event ID generation
  auto now = std::chrono::system_clock::now();
  auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
      now.time_since_epoch()).count();
  
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(1000, 9999);
  
  return std::to_string(timestamp) + "_" + std::to_string(dis(gen));
}

std::string HttpSseStream::FormatSseMessage(const http::sse::SseEvent& event) {
  http::sse::SseEvent formatted_event = event;
  
  // Auto-generate event ID if enabled and not provided
  if (auto_event_id_ && !formatted_event.id.has_value()) {
    formatted_event.id = GenerateEventId();
  }
  
  // Set default event type if not provided
  if (formatted_event.event_type.empty() && !formatted_event.data.empty()) {
    formatted_event.event_type = default_event_type_;
  }
  
  // Use the existing ToString method from SseEvent
  return formatted_event.ToString();
}

Status HttpSseStream::SendSseData(const std::string& data) {
  try {
    // Create NoncontiguousBuffer from string data
    NoncontiguousBuffer buffer;
    buffer.Append(data.data(), data.size());
    
    // Send data through the stream
    StreamSendMessage msg;
    msg.data = std::move(buffer);
    
    auto ret = SendData(std::move(msg));
    if (ret == RetCode::kSuccess) {
      return Status::OK();
    } else {
      return Status(-1, "Failed to send SSE data");
    }
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("Failed to send SSE data: " << e.what());
    return Status(-1, "Failed to send SSE data");
  }
}

}  // namespace trpc::stream::http_sse
