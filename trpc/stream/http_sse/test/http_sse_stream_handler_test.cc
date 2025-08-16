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

#include <gtest/gtest.h>
#include <thread>
#include <chrono>

#include "trpc/stream/stream_options.h"
#include "trpc/stream/protocol_message_metadata.h"

namespace trpc::stream::http_sse {

class HttpSseStreamHandlerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create stream options
    StreamOptions options;
    options.connection_id = 1;
    options.fiber_mode = true;
    options.max_streams = 10;
    
    // Create SSE stream handler
    handler_ = std::make_unique<HttpSseStreamHandler>(std::move(options));
  }

  void TearDown() override {
    if (handler_) {
      handler_->Stop();
      handler_->Join();
    }
  }

  std::unique_ptr<HttpSseStreamHandler> handler_;
};

TEST_F(HttpSseStreamHandlerTest, Constructor) {
  EXPECT_NE(handler_, nullptr);
  EXPECT_EQ(handler_->GetStreamCount(), 0);
}

TEST_F(HttpSseStreamHandlerTest, Init) {
  // Initialize handler
  EXPECT_TRUE(handler_->Init());
  
  // Check initialization state
  EXPECT_EQ(handler_->GetStreamCount(), 0);
}

TEST_F(HttpSseStreamHandlerTest, CreateStream) {
  // Initialize handler
  EXPECT_TRUE(handler_->Init());
  
  // Create stream
  auto stream = handler_->CreateStream(1);
  EXPECT_NE(stream, nullptr);
  
  // Check stream count
  EXPECT_EQ(handler_->GetStreamCount(), 1);
  
  // Get stream
  auto retrieved_stream = handler_->GetStream(1);
  EXPECT_EQ(retrieved_stream, stream);
}

TEST_F(HttpSseStreamHandlerTest, CreateMultipleStreams) {
  // Initialize handler
  EXPECT_TRUE(handler_->Init());
  
  // Create multiple streams
  for (uint32_t i = 1; i <= 5; ++i) {
    auto stream = handler_->CreateStream(i);
    EXPECT_NE(stream, nullptr);
  }
  
  // Check stream count
  EXPECT_EQ(handler_->GetStreamCount(), 5);
  
  // Get all streams
  auto all_streams = handler_->GetAllStreams();
  EXPECT_EQ(all_streams.size(), 5);
}

TEST_F(HttpSseStreamHandlerTest, RemoveStream) {
  // Initialize handler
  EXPECT_TRUE(handler_->Init());
  
  // Create stream
  auto stream = handler_->CreateStream(1);
  EXPECT_NE(stream, nullptr);
  EXPECT_EQ(handler_->GetStreamCount(), 1);
  
  // Remove stream
  int result = handler_->RemoveStream(1);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(handler_->GetStreamCount(), 0);
  
  // Try to get removed stream
  auto retrieved_stream = handler_->GetStream(1);
  EXPECT_EQ(retrieved_stream, nullptr);
}

TEST_F(HttpSseStreamHandlerTest, RemoveNonExistentStream) {
  // Initialize handler
  EXPECT_TRUE(handler_->Init());
  
  // Try to remove non-existent stream
  int result = handler_->RemoveStream(999);
  EXPECT_EQ(result, -1);
  EXPECT_EQ(handler_->GetStreamCount(), 0);
}

TEST_F(HttpSseStreamHandlerTest, IsNewStream) {
  // Initialize handler
  EXPECT_TRUE(handler_->Init());
  
  // Check new stream
  EXPECT_TRUE(handler_->IsNewStream(1, 0));
  
  // Create stream
  auto stream = handler_->CreateStream(1);
  EXPECT_NE(stream, nullptr);
  
  // Check existing stream
  EXPECT_FALSE(handler_->IsNewStream(1, 0));
  
  // Check another new stream
  EXPECT_TRUE(handler_->IsNewStream(2, 0));
}

TEST_F(HttpSseStreamHandlerTest, MaxStreamsLimit) {
  // Initialize handler
  EXPECT_TRUE(handler_->Init());
  
  // Set low max streams limit
  handler_->SetMaxStreams(3);
  
  // Create streams up to limit
  for (uint32_t i = 1; i <= 3; ++i) {
    auto stream = handler_->CreateStream(i);
    EXPECT_NE(stream, nullptr);
  }
  
  // Try to create more streams
  auto stream = handler_->CreateStream(4);
  EXPECT_EQ(stream, nullptr);
  
  // Check stream count
  EXPECT_EQ(handler_->GetStreamCount(), 3);
}

TEST_F(HttpSseStreamHandlerTest, PushMessage) {
  // Initialize handler
  EXPECT_TRUE(handler_->Init());
  
  // Create stream
  auto stream = handler_->CreateStream(1);
  EXPECT_NE(stream, nullptr);
  
  // Create message metadata
  ProtocolMessageMetadata metadata;
  metadata.stream_id = 1;
  metadata.frame_type = 0;
  
  // Push message
  std::any message = std::string("test message");
  handler_->PushMessage(std::move(message), std::move(metadata));
  
  // Message should be routed to the stream
  // (Note: This is a basic test - actual message routing depends on stream implementation)
}

TEST_F(HttpSseStreamHandlerTest, StreamOptions) {
  // Initialize handler
  EXPECT_TRUE(handler_->Init());
  
  // Get mutable options
  auto options = handler_->GetMutableStreamOptions();
  EXPECT_NE(options, nullptr);
  
  // Modify options
  options->max_streams = 50;
  
  // Set options
  StreamOptions new_options;
  new_options.max_streams = 100;
  new_options.fiber_mode = false;
  handler_->SetStreamOptions(new_options);
  
  // Check if options were updated
  auto updated_options = handler_->GetMutableStreamOptions();
  EXPECT_EQ(updated_options->max_streams, 100);
  EXPECT_EQ(updated_options->fiber_mode, false);
}

TEST_F(HttpSseStreamHandlerTest, AutoCleanup) {
  // Initialize handler
  EXPECT_TRUE(handler_->Init());
  
  // Enable auto cleanup
  handler_->SetAutoCleanup(true);
  
  // Create some streams
  for (uint32_t i = 1; i <= 3; ++i) {
    auto stream = handler_->CreateStream(i);
    EXPECT_NE(stream, nullptr);
  }
  
  // Check initial count
  EXPECT_EQ(handler_->GetStreamCount(), 3);
  
  // Auto cleanup should run periodically
  // (Note: This test doesn't wait for cleanup - it's just checking the setting)
}

TEST_F(HttpSseStreamHandlerTest, StopAndJoin) {
  // Initialize handler
  EXPECT_TRUE(handler_->Init());
  
  // Create some streams
  for (uint32_t i = 1; i <= 3; ++i) {
    auto stream = handler_->CreateStream(i);
    EXPECT_NE(stream, nullptr);
  }
  
  // Stop handler
  handler_->Stop();
  
  // Join handler
  handler_->Join();
  
  // All streams should be cleaned up
  EXPECT_EQ(handler_->GetStreamCount(), 0);
}

}  // namespace trpc::stream::http_sse
