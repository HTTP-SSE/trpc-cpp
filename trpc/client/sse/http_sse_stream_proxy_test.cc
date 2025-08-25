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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "trpc/client/make_client_context.h"
#include "trpc/client/service_proxy_option_setter.h"
#include "trpc/client/client_context.h"
#include "trpc/codec/codec_helper.h"
#include "trpc/codec/http/http_protocol.h"
#include "trpc/common/status.h"
#include "trpc/util/ref_ptr.h"

namespace trpc::testing {

using namespace stream;

class HttpSseStreamProxyTest : public ::testing::Test {
 public:
  void SetUp() override {
    // Create a simple proxy instance for basic testing
    proxy_ = std::make_shared<HttpSseStreamProxy>();
  }

  void TearDown() override {
    proxy_.reset();
  }

 protected:
  std::shared_ptr<HttpSseStreamProxy> proxy_{nullptr};
};

TEST_F(HttpSseStreamProxyTest, TestCreateSseRequestProtocol) {
  // Test that we can create a proper SSE request protocol
  auto protocol = proxy_->CreateSseRequestProtocol();
  
  ASSERT_NE(protocol, nullptr);
  
  // Cast to HTTP request protocol
  auto* http_protocol = dynamic_cast<HttpRequestProtocol*>(protocol.get());
  ASSERT_NE(http_protocol, nullptr);
  
  // Verify default values
  EXPECT_EQ(http_protocol->request->GetMethod(), "GET");
  EXPECT_EQ(http_protocol->request->GetUrl(), "/");
}

TEST_F(HttpSseStreamProxyTest, TestSetupSseParametersWithValidContext) {
  // Create a client context manually instead of using MakeClientContext
  auto ctx = MakeRefCounted<ClientContext>();
  ASSERT_NE(ctx, nullptr);
  
  // Setup SSE parameters
  bool result = proxy_->SetupSseParameters(ctx);
  EXPECT_TRUE(result);
  
  // Verify request was created
  ASSERT_NE(ctx->GetRequest(), nullptr);
  
  // Cast to HTTP request protocol
  auto* http_protocol = dynamic_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  ASSERT_NE(http_protocol, nullptr);
  
  // Verify SSE-specific headers are set
  auto& request = http_protocol->request;
  EXPECT_EQ(request->GetHeader("Accept"), "text/event-stream");
  EXPECT_EQ(request->GetHeader("Cache-Control"), "no-cache");
  EXPECT_EQ(request->GetHeader("Connection"), "keep-alive");
  
  // Verify timeout is set
  EXPECT_EQ(ctx->GetTimeout(), 60000);  // 60 seconds
}

TEST_F(HttpSseStreamProxyTest, TestSetupSseParametersWithNullContext) {
  // Test that SetupSseParameters handles null context gracefully
  bool result = proxy_->SetupSseParameters(nullptr);
  EXPECT_FALSE(result);
}

TEST_F(HttpSseStreamProxyTest, TestGetSyncStreamReaderWriterWithNullContext) {
  // Test that GetSyncStreamReaderWriter handles null context properly
  auto result = proxy_->GetSyncStreamReaderWriter(nullptr);
  
  // Should return an error stream with appropriate status
  EXPECT_FALSE(result.GetStatus().OK());
  EXPECT_EQ(result.GetStatus().GetFrameworkRetCode(), static_cast<int>(codec::ClientRetCode::ENCODE_ERROR));
}

TEST_F(HttpSseStreamProxyTest, TestSseHeadersAreCorrect) {
  // Test that all required SSE headers are set correctly
  auto ctx = MakeRefCounted<ClientContext>();
  bool result = proxy_->SetupSseParameters(ctx);
  EXPECT_TRUE(result);
  
  auto* http_protocol = dynamic_cast<HttpRequestProtocol*>(ctx->GetRequest().get());
  auto& request = http_protocol->request;
  
  // Required SSE headers according to specification
  EXPECT_EQ(request->GetHeader("Accept"), "text/event-stream");
  EXPECT_EQ(request->GetHeader("Cache-Control"), "no-cache");
  EXPECT_EQ(request->GetHeader("Connection"), "keep-alive");
}

TEST_F(HttpSseStreamProxyTest, TestTimeoutConfiguration) {
  // Test that SSE timeout is configured appropriately for long-lived connections
  auto ctx = MakeRefCounted<ClientContext>();
  bool result = proxy_->SetupSseParameters(ctx);
  EXPECT_TRUE(result);
  
  // SSE connections should have a longer timeout than regular HTTP requests
  EXPECT_EQ(ctx->GetTimeout(), 60000);  // 60 seconds
  
  // Verify this is different from the default timeout in the option
  EXPECT_GT(ctx->GetTimeout(), 1000);  // Should be longer than the 1-second default
}

TEST_F(HttpSseStreamProxyTest, TestBasicFunctionality) {
  // Test basic functionality without complex initialization
  auto ctx = MakeRefCounted<ClientContext>();
  
  // Test SetupSseParameters
  bool setup_result = proxy_->SetupSseParameters(ctx);
  EXPECT_TRUE(setup_result);
  
  // Test CreateSseRequestProtocol
  auto protocol = proxy_->CreateSseRequestProtocol();
  EXPECT_NE(protocol, nullptr);
  
  // Test that we can create a context and it's valid
  EXPECT_NE(ctx, nullptr);
  EXPECT_TRUE(setup_result);
}

}  // namespace trpc::testing
