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

#include <gtest/gtest.h>

#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/runtime/iomodel/reactor/common/connection.h"

namespace trpc::test {

class HttpSseProtoCheckerTest : public ::testing::Test {
 protected:
  void SetUp() override {}
  void TearDown() override {}
};

// Test IsValidSseRequest function
TEST_F(HttpSseProtoCheckerTest, IsValidSseRequest_ValidRequest) {
  auto request = std::make_shared<http::Request>();
  request->SetMethod("GET");
  request->SetHeader("Accept", "text/event-stream");
  
  bool is_valid = trpc::IsValidSseRequest(request.get());
  EXPECT_TRUE(is_valid);
}

TEST_F(HttpSseProtoCheckerTest, IsValidSseRequest_InvalidMethod) {
  auto request = std::make_shared<http::Request>();
  request->SetMethod("POST");
  request->SetHeader("Accept", "text/event-stream");
  
  bool is_valid = trpc::IsValidSseRequest(request.get());
  EXPECT_FALSE(is_valid);
}

TEST_F(HttpSseProtoCheckerTest, IsValidSseRequest_InvalidAcceptHeader) {
  auto request = std::make_shared<http::Request>();
  request->SetMethod("GET");
  request->SetHeader("Accept", "application/json");
  
  bool is_valid = trpc::IsValidSseRequest(request.get());
  EXPECT_FALSE(is_valid);
}

TEST_F(HttpSseProtoCheckerTest, IsValidSseRequest_MissingAcceptHeader) {
  auto request = std::make_shared<http::Request>();
  request->SetMethod("GET");
  
  bool is_valid = trpc::IsValidSseRequest(request.get());
  EXPECT_FALSE(is_valid);
}

TEST_F(HttpSseProtoCheckerTest, IsValidSseRequest_NullRequest) {
  bool is_valid = trpc::IsValidSseRequest(nullptr);
  EXPECT_FALSE(is_valid);
}

TEST_F(HttpSseProtoCheckerTest, IsValidSseRequest_AcceptHeaderWithMultipleTypes) {
  auto request = std::make_shared<http::Request>();
  request->SetMethod("GET");
  request->SetHeader("Accept", "text/html,text/event-stream,application/json");
  
  bool is_valid = trpc::IsValidSseRequest(request.get());
  EXPECT_TRUE(is_valid);
}

// Test IsValidSseResponse function
TEST_F(HttpSseProtoCheckerTest, IsValidSseResponse_ValidResponse) {
  auto response = std::make_shared<http::Response>();
  response->SetContentType("text/event-stream");
  response->SetHeader("Cache-Control", "no-cache");
  
  bool is_valid = trpc::IsValidSseResponse(response.get());
  EXPECT_TRUE(is_valid);
}

TEST_F(HttpSseProtoCheckerTest, IsValidSseResponse_InvalidContentType) {
  auto response = std::make_shared<http::Response>();
  response->SetContentType("application/json");
  response->SetHeader("Cache-Control", "no-cache");
  
  bool is_valid = trpc::IsValidSseResponse(response.get());
  EXPECT_FALSE(is_valid);
}

TEST_F(HttpSseProtoCheckerTest, IsValidSseResponse_MissingContentType) {
  auto response = std::make_shared<http::Response>();
  response->SetHeader("Cache-Control", "no-cache");
  
  bool is_valid = trpc::IsValidSseResponse(response.get());
  EXPECT_FALSE(is_valid);
}

TEST_F(HttpSseProtoCheckerTest, IsValidSseResponse_InvalidCacheControl) {
  auto response = std::make_shared<http::Response>();
  response->SetContentType("text/event-stream");
  response->SetHeader("Cache-Control", "max-age=3600");
  
  bool is_valid = trpc::IsValidSseResponse(response.get());
  EXPECT_FALSE(is_valid);
}

TEST_F(HttpSseProtoCheckerTest, IsValidSseResponse_MissingCacheControl) {
  auto response = std::make_shared<http::Response>();
  response->SetContentType("text/event-stream");
  
  bool is_valid = trpc::IsValidSseResponse(response.get());
  EXPECT_FALSE(is_valid);
}

TEST_F(HttpSseProtoCheckerTest, IsValidSseResponse_NullResponse) {
  bool is_valid = trpc::IsValidSseResponse(nullptr);
  EXPECT_FALSE(is_valid);
}

TEST_F(HttpSseProtoCheckerTest, IsValidSseResponse_CacheControlWithMultipleValues) {
  auto response = std::make_shared<http::Response>();
  response->SetContentType("text/event-stream");
  response->SetHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  
  bool is_valid = trpc::IsValidSseResponse(response.get());
  EXPECT_TRUE(is_valid);
}

// Test HttpSseZeroCopyCheckRequest function
TEST_F(HttpSseProtoCheckerTest, HttpSseZeroCopyCheckRequest_EmptyBuffer) {
  // Mock connection
  auto conn = trpc::MakeRefCounted<trpc::Connection>();
  NoncontiguousBuffer in;
  std::deque<std::any> out;
  
  int result = trpc::HttpSseZeroCopyCheckRequest(conn, in, out);
  EXPECT_EQ(result, trpc::kPacketLess);
  EXPECT_TRUE(out.empty());
}

TEST_F(HttpSseProtoCheckerTest, HttpSseZeroCopyCheckRequest_InvalidHttpRequest) {
  // Mock connection
  auto conn = trpc::MakeRefCounted<trpc::Connection>();
  NoncontiguousBuffer in;
  
  // Add invalid HTTP request data
  std::string invalid_request = "INVALID HTTP REQUEST DATA\r\n\r\n";
  NoncontiguousBufferBuilder builder;
  builder.Append(invalid_request.data(), invalid_request.size());
  in = builder.DestructiveGet();
  
  std::deque<std::any> out;
  int result = trpc::HttpSseZeroCopyCheckRequest(conn, in, out);
  EXPECT_EQ(result, trpc::kPacketError);
}

TEST_F(HttpSseProtoCheckerTest, HttpSseZeroCopyCheckRequest_ValidHttpRequest) {
  // Mock connection
  auto conn = trpc::MakeRefCounted<trpc::Connection>();
  NoncontiguousBuffer in;
  
  // Add valid HTTP request data
  std::string valid_request = 
    "GET /events HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "Accept: text/event-stream\r\n"
    "Cache-Control: no-cache\r\n"
    "Connection: keep-alive\r\n"
    "\r\n";
  
  NoncontiguousBufferBuilder builder;
  builder.Append(valid_request.data(), valid_request.size());
  in = builder.DestructiveGet();
  
  std::deque<std::any> out;
  int result = trpc::HttpSseZeroCopyCheckRequest(conn, in, out);
  EXPECT_EQ(result, trpc::kPacketFull);
  EXPECT_FALSE(out.empty());
}

// Test HttpSseZeroCopyCheckResponse function
TEST_F(HttpSseProtoCheckerTest, HttpSseZeroCopyCheckResponse_EmptyBuffer) {
  // Mock connection
  auto conn = trpc::MakeRefCounted<trpc::Connection>();
  NoncontiguousBuffer in;
  std::deque<std::any> out;
  
  int result = trpc::HttpSseZeroCopyCheckResponse(conn, in, out);
  EXPECT_EQ(result, trpc::kPacketLess);
  EXPECT_TRUE(out.empty());
}

TEST_F(HttpSseProtoCheckerTest, HttpSseZeroCopyCheckResponse_InvalidHttpResponse) {
  // Mock connection
  auto conn = trpc::MakeRefCounted<trpc::Connection>();
  NoncontiguousBuffer in;
  
  // Add invalid HTTP response data
  std::string invalid_response = "INVALID HTTP RESPONSE DATA\r\n\r\n";
  NoncontiguousBufferBuilder builder;
  builder.Append(invalid_response.data(), invalid_response.size());
  in = builder.DestructiveGet();
  
  std::deque<std::any> out;
  int result = trpc::HttpSseZeroCopyCheckResponse(conn, in, out);
  EXPECT_EQ(result, trpc::kPacketError);
}

TEST_F(HttpSseProtoCheckerTest, HttpSseZeroCopyCheckResponse_ValidHttpResponse) {
  // Mock connection
  auto conn = trpc::MakeRefCounted<trpc::Connection>();
  NoncontiguousBuffer in;
  
  // Add valid HTTP response data
  std::string valid_response = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/event-stream\r\n"
    "Cache-Control: no-cache\r\n"
    "Connection: keep-alive\r\n"
    "\r\n"
    "data: Hello World\r\n\r\n";
  
  NoncontiguousBufferBuilder builder;
  builder.Append(valid_response.data(), valid_response.size());
  in = builder.DestructiveGet();
  
  std::deque<std::any> out;
  int result = trpc::HttpSseZeroCopyCheckResponse(conn, in, out);
  EXPECT_EQ(result, trpc::kPacketFull);
  EXPECT_FALSE(out.empty());
}

// Test edge cases
TEST_F(HttpSseProtoCheckerTest, EdgeCase_RequestWithExtraHeaders) {
  auto request = std::make_shared<http::Request>();
  request->SetMethod("GET");
  request->SetHeader("Accept", "text/event-stream");
  request->SetHeader("User-Agent", "Mozilla/5.0");
  request->SetHeader("Authorization", "Bearer token123");
  
  bool is_valid = trpc::IsValidSseRequest(request.get());
  EXPECT_TRUE(is_valid);
}

TEST_F(HttpSseProtoCheckerTest, EdgeCase_ResponseWithExtraHeaders) {
  auto response = std::make_shared<http::Response>();
  response->SetContentType("text/event-stream");
  response->SetHeader("Cache-Control", "no-cache");
  response->SetHeader("Access-Control-Allow-Origin", "*");
  response->SetHeader("X-Custom-Header", "value");
  
  bool is_valid = trpc::IsValidSseResponse(response.get());
  EXPECT_TRUE(is_valid);
}

TEST_F(HttpSseProtoCheckerTest, EdgeCase_CaseInsensitiveHeaders) {
  auto request = std::make_shared<http::Request>();
  request->SetMethod("GET");
  request->SetHeader("accept", "TEXT/EVENT-STREAM"); // Different case
  
  bool is_valid = trpc::IsValidSseRequest(request.get());
  EXPECT_TRUE(is_valid);
}

TEST_F(HttpSseProtoCheckerTest, EdgeCase_WhitespaceInHeaders) {
  auto request = std::make_shared<http::Request>();
  request->SetMethod("GET");
  request->SetHeader("Accept", "  text/event-stream  "); // With whitespace
  
  bool is_valid = trpc::IsValidSseRequest(request.get());
  EXPECT_TRUE(is_valid);
}

}  // namespace trpc::test
