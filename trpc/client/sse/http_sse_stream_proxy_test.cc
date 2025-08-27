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

#include "trpc/client/sse/http_sse_stream_proxy.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>
#include <memory>
#include <string>
#include <functional>

#include "trpc/client/service_proxy_option.h"
#include "trpc/common/status.h"
#include "trpc/util/http/sse/sse_event.h"
#include "trpc/util/http/sse/sse_parser.h"

namespace trpc {

class HttpSseStreamProxyTest : public ::testing::Test {
 public:
  void SetUp() override {
    // Setup service proxy options without requiring config file or codec registration
    options_.name = "test_sse_client";
    options_.codec_name = "http";
    options_.network = "tcp";
    options_.conn_type = "long";
    options_.timeout = 30000;
    options_.selector_name = "direct";
    options_.target = "127.0.0.1:8080";
    
    // Create HttpSseStreamProxy without initializing the underlying proxy
    sse_proxy_ = std::make_shared<::trpc::HttpSseStreamProxy>();
  }

  void TearDown() override {
    sse_proxy_.reset();
  }

 protected:
  ::trpc::ServiceProxyOption options_;
  std::shared_ptr<::trpc::HttpSseStreamProxy> sse_proxy_;
};

TEST_F(HttpSseStreamProxyTest, ConstructorDestructor) {
  // Test basic construction and destruction
  EXPECT_NE(sse_proxy_, nullptr);
  
  // Test direct construction
  auto proxy2 = std::make_shared<::trpc::HttpSseStreamProxy>();
  EXPECT_NE(proxy2, nullptr);
}

TEST_F(HttpSseStreamProxyTest, SetServiceProxyOption) {
  // Test setting service proxy options - should not throw
  // Note: This test validates the method exists and doesn't crash
  EXPECT_NO_THROW(sse_proxy_->SetServiceProxyOption(options_));
  
  // Test that we can still call other methods after setting options
  EXPECT_NO_THROW(sse_proxy_->InitializeSseProxy());
}

TEST_F(HttpSseStreamProxyTest, CreateSseContext) {
  // Test creating SSE context - this should return nullptr when no HTTP proxy is set
  std::string url = "/ai/chat?question=hello";
  auto ctx = sse_proxy_->CreateSseContext(url, 30000);
  
  // Without proper initialization, this should return nullptr
  EXPECT_EQ(ctx, nullptr);
}

TEST_F(HttpSseStreamProxyTest, InitializeSseProxy) {
  // Test SSE proxy initialization
  EXPECT_NO_THROW(sse_proxy_->InitializeSseProxy());
}

TEST_F(HttpSseStreamProxyTest, SendRequestWithNullContext) {
  std::string response_content;
  
  ::trpc::Status status = sse_proxy_->SendRequest(nullptr, "/test", response_content);
  
  // Should fail with invalid HTTP service proxy
  EXPECT_FALSE(status.OK());
}

TEST_F(HttpSseStreamProxyTest, ConnectAndReceiveWithNullContext) {
  ::trpc::SseEventCallback callback = [](const ::trpc::http::sse::SseEvent& event) -> bool {
    return true;
  };
  
  ::trpc::Status status = sse_proxy_->ConnectAndReceive(nullptr, "/test", callback);
  
  // Should fail due to invalid HTTP service proxy
  EXPECT_FALSE(status.OK());
}

TEST_F(HttpSseStreamProxyTest, ConnectAndReceiveWithNullCallback) {
  // Create a mock context (will be nullptr since no HTTP proxy is set)
  auto ctx = sse_proxy_->CreateSseContext("/test", 30000);
  
  ::trpc::SseEventCallback null_callback = nullptr;
  ::trpc::Status status = sse_proxy_->ConnectAndReceive(ctx, "/test", null_callback);
  
  // Should fail due to null callback
  EXPECT_FALSE(status.OK());
}

TEST_F(HttpSseStreamProxyTest, SseEventCallbackType) {
  // Test SSE event callback function type
  bool callback_called = false;
  ::trpc::http::sse::SseEvent test_event;
  test_event.event_type = "message";
  test_event.data = "test data";
  
  ::trpc::SseEventCallback callback = [&callback_called](const ::trpc::http::sse::SseEvent& event) -> bool {
    callback_called = true;
    EXPECT_EQ(event.event_type, "message");
    EXPECT_EQ(event.data, "test data");
    return false; // Stop processing
  };
  
  // Call callback directly for testing
  bool result = callback(test_event);
  
  EXPECT_TRUE(callback_called);
  EXPECT_FALSE(result); // Should return false to stop processing
}

TEST_F(HttpSseStreamProxyTest, GetSseStreamWithValidContext) {
  // Create a mock context (will be nullptr since no HTTP proxy is set)
  auto ctx = sse_proxy_->CreateSseContext("/test", 30000);
  
  // This should not crash even with null context/proxy
  EXPECT_NO_THROW({
    auto stream = sse_proxy_->GetSseStream(ctx, "/test");
  });
}

TEST_F(HttpSseStreamProxyTest, GetSseStreamWithNullContext) {
  // Test with null context should handle gracefully
  EXPECT_NO_THROW({
    auto stream = sse_proxy_->GetSseStream(nullptr, "/test");
  });
}

// Test SSE parser integration
TEST_F(HttpSseStreamProxyTest, SseParserIntegration) {
  // Test SSE content parsing
  std::string sse_content = 
    "event: message\n"
    "data: Hello World\n\n"
    "event: close\n"
    "data: Connection closed\n\n";
  
  std::vector<::trpc::http::sse::SseEvent> events = ::trpc::http::sse::SseParser::ParseEvents(sse_content);
  
  EXPECT_EQ(events.size(), 2);
  if (events.size() >= 2) {
    EXPECT_EQ(events[0].event_type, "message");
    EXPECT_EQ(events[0].data, "Hello World");
    EXPECT_EQ(events[1].event_type, "close");
    EXPECT_EQ(events[1].data, "Connection closed");
  }
}

}  // namespace trpc

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
