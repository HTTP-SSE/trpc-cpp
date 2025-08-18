#include <iostream>
#include <memory>
#include <string>
#include "trpc/server/http_sse/HttpSseService.h"
#include "trpc/util/http/sse/sse_event.h"
#include "trpc/util/http/request.h"
#include "trpc/util/http/response.h"
#include "trpc/util/ref_ptr.h"
#include "trpc/filter/server_filter_controller.h"

using namespace trpc;

// Mock 一个最简 ServerContext
class MockServerContext : public ServerContext {
public:
  MockServerContext() {
        server_filter_controller_ = std::make_unique<ServerFilterController>();
        SetFilterController(server_filter_controller_.get());  // ⚠️关键，绑定到 ServerContext
        std::cout << "[MockServerContext] Constructor: FilterController set" << std::endl;
    }

  explicit MockServerContext(const http::RequestPtr& req) : req_(req), user_data_(0) {
        server_filter_controller_ = std::make_unique<ServerFilterController>();
        SetFilterController(server_filter_controller_.get());
        std::cout << "[MockServerContext] Constructor with request: FilterController set" << std::endl;
  }

  void CloseConnection() override {
    std::cout << "[MockServerContext] CloseConnection called" << std::endl;
  }

  bool HasFilterController() const  { 
    bool has = server_filter_controller_ != nullptr;
    std::cout << "[MockServerContext] HasFilterController = " << (has ? "true" : "false") << std::endl;
    return has;
  }
  Status SendResponse(NoncontiguousBuffer&& buf) {
    // 打印 block 数和总字节数，方便调试
     std::cout << "[MockServerContext::SendResponse] blocks = " << buf.size()
                << ", bytes = " << buf.ByteSize() << std::endl;

    // 把内容拼成字符串打印（测试用）
     std::string output;
     for (auto& block : buf) {
       output.append(block.data(), block.size());
     }
     std::cout << "[MockServerContext] payload:\n" << output << std::endl;

      // 返回一个默认 Status，测试中足够。如果你需要模拟失败可以返回非 OK 的 Status。
     return Status{};
  }

  void SetUserData(uint64_t id) { 
      user_data_ = id; 
      std::cout << "[MockServerContext] SetUserData: " << id << std::endl;
  }
  std::any GetUserData() const  { 
      std::cout << "[MockServerContext] GetUserData: " << user_data_ << std::endl;
      return user_data_; 
  }

  http::RequestPtr GetRequest() const { return req_; }

private:
  http::RequestPtr req_;
  uint64_t user_data_;
  std::unique_ptr<ServerFilterController> server_filter_controller_;
};

// 一个简单的 handler，接收到 GET 请求后走 SSE 建立连接
bool MySseHandler(const RefPtr<ServerContext>& ctx, HttpSseService& service) {
  auto mock_ctx = dynamic_cast<MockServerContext*>(ctx.get());
  if (!mock_ctx) {
      std::cerr << "[Handler] dynamic_cast failed!" << std::endl;
      return false;
  }

  auto req = mock_ctx->GetRequest();
  if (!req) {
      std::cerr << "[Handler] Request is null!" << std::endl;
      return false;
  }

  if (req->GetMethod() == "GET" && req->GetUrl() == "/sse") {
    std::cout << "[Handler] SSE request detected" << std::endl;
    bool result = service.HandleSseRequest(ctx);
    std::cout << "[Handler] HandleSseRequest returned " << (result ? "true" : "false") << std::endl;
    return result;
  }

  std::cout << "[Handler] Not a SSE request" << std::endl;
  return false;
}

int main() {
  HttpSseService service;

  // 1. 模拟一个 HTTP GET /sse 请求
  auto req = std::make_shared<http::Request>();
  req->SetMethod("GET");
  req->SetUrl("/sse");
  auto ctx = new MockServerContext(req);
  RefPtr<ServerContext> ctx_ref(adopt_ptr, ctx);

  std::cout << "[Main] Created MockServerContext and RefPtr" << std::endl;

  // 2. 进入路由 → handler
  bool ok = MySseHandler(ctx_ref, service);
  if (!ok) {
    std::cerr << "[Main] Handler failed" << std::endl;
    return 1;
  }

  uint64_t client_id = std::any_cast<uint64_t>(ctx_ref->GetUserData());
  std::cout << "[Main] Client connected, id=" << client_id << std::endl;

  // 3. 服务端发送一条消息给客户端
  http::sse::SseEvent ev;
  ev.event_type = "welcome";
  ev.data       = "hello from server";
  std::cout << "[Main] Sending SSE event to client..." << std::endl;
  bool sent = service.SendToClient(client_id, ev);
  std::cout << "[Main] SendToClient result=" << sent << std::endl;

  // 4. 广播一条消息
  http::sse::SseEvent ev2;
  ev2.event_type = "broadcast";
  ev2.data       = "hi all clients";
  std::cout << "[Main] Broadcasting SSE event..." << std::endl;
  size_t succ = service.Broadcast(ev2);
  std::cout << "[Main] Broadcast result=" << succ << std::endl;

  // 5. 断开客户端连接
  std::cout << "[Main] Closing client..." << std::endl;
  service.CloseClient(client_id);
  std::cout << "[Main] Closed client " << client_id << std::endl;

  // 6. 服务关闭
  std::cout << "[Main] Shutting down service..." << std::endl;
  service.Shutdown();
  std::cout << "[Main] Service shutdown complete" << std::endl;

  return 0;
}

