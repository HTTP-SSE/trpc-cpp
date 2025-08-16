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

#include <gtest/gtest.h>
#include <vector>

#include "trpc/stream/stream_options.h"
#include "trpc/util/http/sse/sse_event.h"

namespace trpc::stream::http_sse {

class HttpSseStreamTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create stream options
    StreamOptions options;
    options.stream_id = 1;
    options.connection_id = 1;
    options.fiber_mode = true;
    
    // Create SSE stream
    stream_ = std::make_unique<HttpSseStream>(std::move(options));
  }

  void TearDown() override {
    if (stream_) {
      stream_->Close(Status::OK());
      stream_->Join();
    }
  }

  std::unique_ptr<HttpSseStream> stream_;
};

TEST_F(HttpSseStreamTest, Constructor) {
  EXPECT_NE(stream_, nullptr);
  EXPECT_FALSE(stream_->IsReady());
}

TEST_F(HttpSseStreamTest, SendEvent) {
  // Initialize stream
  EXPECT_TRUE(stream_->Init());
  
  // Create SSE event
  http::sse::SseEvent event;
  event.event_type = "message";
  event.data = "Hello World";
  event.id = "123";
  
  // Send event
  auto status = stream_->SendEvent(event);
  EXPECT_TRUE(status.OK());
}

TEST_F(HttpSseStreamTest, SendEvents) {
  // Initialize stream
  EXPECT_TRUE(stream_->Init());
  
  // Create multiple SSE events
  std::vector<http::sse::SseEvent> events;
  
  http::sse::SseEvent event1;
  event1.event_type = "message";
  event1.data = "Event 1";
  event1.id = "1";
  events.push_back(event1);
  
  http::sse::SseEvent event2;
  event2.event_type = "update";
  event2.data = "Event 2";
  event2.id = "2";
  events.push_back(event2);
  
  // Send events
  auto status = stream_->SendEvents(events);
  EXPECT_TRUE(status.OK());
}

TEST_F(HttpSseStreamTest, SendComment) {
  // Initialize stream
  EXPECT_TRUE(stream_->Init());
  
  // Send comment
  auto status = stream_->SendComment("This is a comment");
  EXPECT_TRUE(status.OK());
}

TEST_F(HttpSseStreamTest, SendRetry) {
  // Initialize stream
  EXPECT_TRUE(stream_->Init());
  
  // Send retry instruction
  auto status = stream_->SendRetry(5000);
  EXPECT_TRUE(status.OK());
}

TEST_F(HttpSseStreamTest, EventIdGenerator) {
  // Set custom event ID generator
  std::string custom_id = "custom_123";
  stream_->SetEventIdGenerator([custom_id]() { return custom_id; });
  
  // Initialize stream
  EXPECT_TRUE(stream_->Init());
  
  // Create event without ID
  http::sse::SseEvent event;
  event.event_type = "message";
  event.data = "Test data";
  
  // Send event (should use custom ID generator)
  auto status = stream_->SendEvent(event);
  EXPECT_TRUE(status.OK());
}

TEST_F(HttpSseStreamTest, AutoEventId) {
  // Disable auto event ID
  stream_->SetAutoEventId(false);
  
  // Initialize stream
  EXPECT_TRUE(stream_->Init());
  
  // Create event without ID
  http::sse::SseEvent event;
  event.event_type = "message";
  event.data = "Test data";
  
  // Send event (should not generate ID)
  auto status = stream_->SendEvent(event);
  EXPECT_TRUE(status.OK());
}

TEST_F(HttpSseStreamTest, DefaultEventType) {
  // Set default event type
  stream_->SetDefaultEventType("notification");
  
  // Initialize stream
  EXPECT_TRUE(stream_->Init());
  
  // Create event without type
  http::sse::SseEvent event;
  event.data = "Test data";
  
  // Send event (should use default type)
  auto status = stream_->SendEvent(event);
  EXPECT_TRUE(status.OK());
}

TEST_F(HttpSseStreamTest, StreamState) {
  // Initially not ready
  EXPECT_FALSE(stream_->IsReady());
  
  // Initialize stream
  EXPECT_TRUE(stream_->Init());
  
  // Still not ready until first data is sent
  EXPECT_FALSE(stream_->IsReady());
  
  // Send some data to make stream ready
  http::sse::SseEvent event;
  event.data = "Test data";
  auto status = stream_->SendEvent(event);
  EXPECT_TRUE(status.OK());
  
  // Now should be ready
  EXPECT_TRUE(stream_->IsReady());
}

TEST_F(HttpSseStreamTest, LastReceivedEvent) {
  // Initially no received events
  auto last_event = stream_->GetLastReceivedEvent();
  EXPECT_FALSE(last_event.has_value());
  
  // Initialize stream
  EXPECT_TRUE(stream_->Init());
  
  // Still no received events
  last_event = stream_->GetLastReceivedEvent();
  EXPECT_FALSE(last_event.has_value());
}

}  // namespace trpc::stream::http_sse
