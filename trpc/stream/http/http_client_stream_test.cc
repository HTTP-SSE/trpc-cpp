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

#include "trpc/stream/http/http_client_stream.h"

#include "gtest/gtest.h"

#include "trpc/coroutine/fiber_latch.h"
#include "trpc/coroutine/testing/fiber_runtime.h"
#include "trpc/stream/http/http_client_stream_handler.h"

namespace trpc::testing {

namespace {

stream::HttpClientStreamPtr GetClientStream() {
  stream::StreamOptions handler_options;
  handler_options.send = [](IoMessage&& message) { return 0; };
  auto handler = MakeRefCounted<stream::HttpClientStreamHandler>(std::move(handler_options));

  stream::StreamOptions stream_options;
  stream_options.stream_handler = handler;
  ClientContextPtr client_context = MakeRefCounted<ClientContext>();
  client_context->SetTimeout(1);
  stream_options.context.context = client_context;
  stream_options.callbacks.on_close_cb = [](int reason) {};
  return MakeRefCounted<stream::HttpClientStream>(std::move(stream_options));
}

}  // namespace

TEST(HttpClientStreamTest, TestProvider) {
  RunAsFiber([&]() {
    stream::HttpClientStreamPtr stream = GetClientStream();
    ASSERT_TRUE(std::any_cast<const ClientContextPtr>(stream->GetMutableStreamOptions()->context.context));

    // No data.
    size_t capacity = 1000;
    stream->SetCapacity(capacity);
    ASSERT_EQ(capacity, stream->Capacity());
    ASSERT_EQ(0, stream->Size());
    int code = 0;
    http::HttpHeader http_header;
    ASSERT_EQ(stream::kStreamStatusClientReadTimeout.GetFrameworkRetCode(),
              stream->ReadHeaders(code, http_header).GetFrameworkRetCode());
    NoncontiguousBuffer out;
    ASSERT_EQ(stream::kStreamStatusClientReadTimeout.GetFrameworkRetCode(),
              stream->Read(out, 100).GetFrameworkRetCode());

    // Sends HTTP request header.
    HttpRequestProtocol protocol{std::make_shared<http::Request>()};
    protocol.request->SetHeader(http::kHeaderContentLength, "5");
    stream->SetHttpRequestProtocol(&protocol);
    stream->SetMethod(http::OperationType::PUT);
    ASSERT_TRUE(stream->SendRequestHeader().OK());

    // Receives content.
    http::HttpResponse http_response;
    http_response.SetStatus(200);
    http_response.AddHeader("Content-Type", "application/json");
    stream->PushRecvMessage(std::move(http_response));
    NoncontiguousBuffer in = CreateBufferSlow("hello");
    stream->PushDataToRecvQueue(std::move(in));
    ASSERT_EQ(5, stream->Size());
    in = CreateBufferSlow("world");
    stream->PushDataToRecvQueue(std::move(in));
    ASSERT_EQ(10, stream->Size());

    ASSERT_TRUE(stream->ReadHeaders(code, http_header).OK());
    ASSERT_EQ(200, code);
    ASSERT_EQ("application/json", http_header.Get("Content-Type"));
    ASSERT_TRUE(stream->Read(out, 6).OK());
    ASSERT_EQ("hellow", FlattenSlow(out));
    ASSERT_EQ(4, stream->Size());

    // Receives EOF.
    stream->PushEofToRecvQueue();
    ASSERT_TRUE(stream->ReadAll(out).OK());
    ASSERT_EQ("orld", FlattenSlow(out));
    ASSERT_EQ(stream::kStreamStatusReadEof.GetFrameworkRetCode(), stream->Read(out, 100).GetFrameworkRetCode());

    // Sends content.
    in = CreateBufferSlow("hello");
    ASSERT_TRUE(stream->Write(std::move(in)).OK());
    ASSERT_TRUE(stream->WriteDone().OK());

    stream->Close();
  });
}

TEST(HttpClientStreamTest, TestProviderClose) {
  RunAsFiber([&]() {
    stream::HttpClientStreamPtr stream = GetClientStream();
    ASSERT_TRUE(std::any_cast<const ClientContextPtr>(stream->GetMutableStreamOptions()->context.context));

    // Sends HTTP request header. The inner state will transfer to kReading
    size_t capacity = 1000;
    stream->SetCapacity(capacity);
    HttpRequestProtocol protocol{std::make_shared<http::Request>()};
    protocol.request->SetHeader(http::kHeaderContentLength, "10");
    stream->SetHttpRequestProtocol(&protocol);
    stream->SetMethod(http::OperationType::PUT);
    ASSERT_TRUE(stream->SendRequestHeader().OK());

    // Receives EOF.
    NoncontiguousBuffer in = CreateBufferSlow("helloworld");
    stream->PushDataToRecvQueue(std::move(in));
    ASSERT_EQ(10, stream->Size());
    stream->PushEofToRecvQueue();

    // Stream not closed, reading is normal.
    NoncontiguousBuffer out1;
    ASSERT_TRUE(stream->Read(out1, 5).OK());
    ASSERT_EQ("hello", FlattenSlow(out1));
    ASSERT_EQ(5, stream->Size());

    // Stream closed, reading should still be normal.
    NoncontiguousBuffer out2;
    stream->Close();
    ASSERT_TRUE(stream->Read(out2, 5).OK());
    ASSERT_EQ("world", FlattenSlow(out2));
    ASSERT_EQ(0, stream->Size());
  });
}

TEST(HttpClientStreamTest, CreateStreamReaderWriter) {
  RunAsFiber([&]() {
    bool closing = true;
    stream::HttpClientStreamReaderWriter StreamReaderWriter =
        Create(MakeRefCounted<stream::HttpClientStream>(stream::kStreamStatusClientNetworkError, closing));

    int code;
    http::HttpHeader http_header;
    ASSERT_EQ(stream::kStreamStatusClientNetworkError.GetFrameworkRetCode(),
              StreamReaderWriter.ReadHeaders(code, http_header).GetFrameworkRetCode());

    NoncontiguousBuffer out;
    ASSERT_EQ(stream::kStreamStatusClientNetworkError.GetFrameworkRetCode(),
              StreamReaderWriter.Read(out, 100).GetFrameworkRetCode());

    ASSERT_EQ(stream::kStreamStatusClientNetworkError.GetFrameworkRetCode(),
              StreamReaderWriter.ReadAll(out).GetFrameworkRetCode());

    NoncontiguousBuffer in;
    ASSERT_EQ(stream::kStreamStatusClientNetworkError.GetFrameworkRetCode(),
              StreamReaderWriter.Write(std::move(in)).GetFrameworkRetCode());

    ASSERT_EQ(stream::kStreamStatusClientNetworkError.GetFrameworkRetCode(),
              StreamReaderWriter.WriteDone().GetFrameworkRetCode());

    StreamReaderWriter.Close();
  });
}

// SSE-specific tests for HttpClientStream
TEST(HttpClientStreamTest, ConfigureSseMode) {
  RunAsFiber([&]() {
    stream::HttpClientStreamPtr stream = GetClientStream();
    
    // Test that SSE mode is not enabled initially
    ASSERT_FALSE(stream->IsSseMode());

    // Set up HTTP request protocol (required for ConfigureSseMode)
    HttpRequestProtocol protocol{std::make_shared<http::Request>()};
    stream->SetHttpRequestProtocol(&protocol);

    // Configure SSE mode
    Status status = stream->ConfigureSseMode();
    ASSERT_TRUE(status.OK());
    ASSERT_TRUE(stream->IsSseMode());

    // Test that configuring again doesn't fail
    status = stream->ConfigureSseMode();
    ASSERT_TRUE(status.OK());
    ASSERT_TRUE(stream->IsSseMode());

    stream->Close();
  });
}

TEST(HttpClientStreamTest, ParseSseEvents) {
  RunAsFiber([&]() {
    stream::HttpClientStreamPtr stream = GetClientStream();
    
    // Test SSE event parsing
    std::string sse_data = "event: test\n"
                           "data: Hello, SSE World!\n"
                           "id: 123\n"
                           "retry: 5000\n\n";
    
    NoncontiguousBuffer buffer;
    buffer.Append(CreateBufferSlow(sse_data));
    
    std::vector<trpc::http::sse::SseEvent> events;
    bool success = stream->ParseSseEvents(buffer, events);
    ASSERT_TRUE(success);
    ASSERT_EQ(events.size(), 1);
    
    const auto& event = events[0];
    ASSERT_EQ(event.event_type, "test");
    ASSERT_EQ(event.data, "Hello, SSE World!");
    ASSERT_EQ(event.id.value(), "123");
    ASSERT_EQ(event.retry.value(), 5000);

    stream->Close();
  });
}

TEST(HttpClientStreamTest, ParseMultipleSseEvents) {
  RunAsFiber([&]() {
    stream::HttpClientStreamPtr stream = GetClientStream();
    
    // Test parsing multiple SSE events
    std::string sse_data = "event: first\n"
                           "data: First event\n"
                           "id: 1\n\n"
                           "event: second\n"
                           "data: Second event\n"
                           "id: 2\n\n";
    
    NoncontiguousBuffer buffer;
    buffer.Append(CreateBufferSlow(sse_data));
    
    std::vector<trpc::http::sse::SseEvent> events;
    bool success = stream->ParseSseEvents(buffer, events);
    ASSERT_TRUE(success);
    ASSERT_EQ(events.size(), 2);
    
    ASSERT_EQ(events[0].event_type, "first");
    ASSERT_EQ(events[0].data, "First event");
    ASSERT_EQ(events[0].id.value(), "1");
    
    ASSERT_EQ(events[1].event_type, "second");
    ASSERT_EQ(events[1].data, "Second event");
    ASSERT_EQ(events[1].id.value(), "2");

    stream->Close();
  });
}

TEST(HttpClientStreamTest, ParseSseEventWithOnlyData) {
  RunAsFiber([&]() {
    stream::HttpClientStreamPtr stream = GetClientStream();
    
    // Test parsing SSE event with only data
    std::string sse_data = "data: Simple message\n\n";
    
    NoncontiguousBuffer buffer;
    buffer.Append(CreateBufferSlow(sse_data));
    
    std::vector<trpc::http::sse::SseEvent> events;
    bool success = stream->ParseSseEvents(buffer, events);
    ASSERT_TRUE(success);
    ASSERT_EQ(events.size(), 1);
    
    const auto& event = events[0];
    ASSERT_TRUE(event.event_type.empty());
    ASSERT_EQ(event.data, "Simple message");
    ASSERT_FALSE(event.id.has_value());
    ASSERT_FALSE(event.retry.has_value());

    stream->Close();
  });
}

TEST(HttpClientStreamTest, SseEventSerialization) {
  RunAsFiber([&]() {
    // Test that SSE events can be serialized correctly
    trpc::http::sse::SseEvent event;
    event.event_type = "test";
    event.data = "Test message";
    event.id = "123";
    event.retry = 5000;
    
    std::string serialized = event.ToString();
    std::string expected = "event: test\n"
                           "data: Test message\n"
                           "id: 123\n"
                           "retry: 5000\n\n";
    
    ASSERT_EQ(serialized, expected);
  });
}

}  // namespace trpc::testing
