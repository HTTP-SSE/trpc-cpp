#include "gtest/gtest.h"

#include "trpc/codec/codec_manager.h"
#include "trpc/serialization/trpc_serialization.h"
#include "trpc/server/http_service.h"
#include "trpc/server/testing/server_context_testing.h"
#include "trpc/transport/server/testing/server_transport_testing.h"
#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"
#include "trpc/util/http/sse_event.h"
#include "trpc/stream/http/http_stream.h"

namespace trpc::testing {

class SseStreamWriterTest : public ::testing::Test {
 public:
  static void SetUpTestCase() {
    codec::Init();
    serialization::Init();
  }

  static void TearDownTestCase() {
    codec::Destroy();
    serialization::Destroy();
  }
};

TEST_F(SseStreamWriterTest, WriteHeader) {
  http::RequestPtr request = std::make_shared<http::Request>(1000, false);
  std::shared_ptr<HttpService> service = std::make_shared<HttpService>();
  std::shared_ptr<TestServerTransport> transport = std::make_shared<TestServerTransport>();
  service->SetServerTransport(transport.get());
  ServerContextPtr context = MakeTestServerContext("http", service.get(), std::move(request));
  Connection conn;
  context->SetReserved(&conn);

  stream::SseStreamWriter writer(context.get());

  ASSERT_TRUE(writer.WriteHeader().OK());
  // 因为 header 是直接通过 context_->SendResponse 写出的，所以这里只能检查成功返回
  // 具体 header 是否正确，可以在 codec 层的 UT 验证（SseResponseProtocol/HttpSseServerCodec）
}

TEST_F(SseStreamWriterTest, WriteEvent) {
  http::RequestPtr request = std::make_shared<http::Request>(1000, false);
  std::shared_ptr<HttpService> service = std::make_shared<HttpService>();
  std::shared_ptr<TestServerTransport> transport = std::make_shared<TestServerTransport>();
  service->SetServerTransport(transport.get());
  ServerContextPtr context = MakeTestServerContext("http", service.get(), std::move(request));
  Connection conn;
  context->SetReserved(&conn);

  stream::SseStreamWriter writer(context.get());

  http::sse::SseEvent ev;
  ev.id = "1";
  ev.event_type = "message";
  ev.data = "hello world";

  ASSERT_TRUE(writer.WriteEvent(ev).OK());
  ASSERT_TRUE(writer.WriteDone().OK());
}

TEST_F(SseStreamWriterTest, WriteBuffer) {
  http::RequestPtr request = std::make_shared<http::Request>(1000, false);
  std::shared_ptr<HttpService> service = std::make_shared<HttpService>();
  std::shared_ptr<TestServerTransport> transport = std::make_shared<TestServerTransport>();
  service->SetServerTransport(transport.get());
  ServerContextPtr context = MakeTestServerContext("http", service.get(), std::move(request));
  Connection conn;
  context->SetReserved(&conn);

  stream::SseStreamWriter writer(context.get());

  // 直接构造一个已经序列化好的 SSE payload
  std::string payload = "id: 99\n"
                        "event: notice\n"
                        "data: pre-serialized\n\n";
  NoncontiguousBuffer buf = CreateBufferSlow(payload);

  ASSERT_TRUE(writer.WriteBuffer(std::move(buf)).OK());
  ASSERT_TRUE(writer.WriteDone().OK());
}

TEST_F(SseStreamWriterTest, Close) {
  http::RequestPtr request = std::make_shared<http::Request>(1000, false);
  std::shared_ptr<HttpService> service = std::make_shared<HttpService>();
  std::shared_ptr<TestServerTransport> transport = std::make_shared<TestServerTransport>();
  service->SetServerTransport(transport.get());
  ServerContextPtr context = MakeTestServerContext("http", service.get(), std::move(request));
  Connection conn;
  context->SetReserved(&conn);

  stream::SseStreamWriter writer(context.get());
  http::sse::SseEvent ev;
  ev.data = "bye";
  ASSERT_TRUE(writer.WriteEvent(ev).OK());

  // Close 内部会调用 WriteDone + CloseConnection
  writer.Close();
}

}  // namespace trpc::testing

