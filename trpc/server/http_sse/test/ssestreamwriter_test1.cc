#include <iostream>
#include <string>
#include <memory>

#include "trpc/server/http_sse/ssestreamwriter.h"
#include "trpc/util/http/sse/sse_event.h"
#include "trpc/server/server_context.h"
#include "trpc/filter/server_filter_controller.h"
#include "trpc/util/ref_ptr.h"  // RefPtr

namespace trpc {

// MockServerContext 继承 ServerContext，用于测试
class MockServerContext : public ServerContext {
public:
    MockServerContext() {
        // 初始化 ServerFilterController
        server_filter_controller_ = std::make_unique<ServerFilterController>();
        // 绑定到 ServerContext
        SetFilterController(server_filter_controller_.get());
    }

      // 注意：签名必须和 ServerContext 中声明的一致（返回 Status）
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

  // 名称要和基类一致：CloseConnection
    void CloseConnection()  {
      std::cout << "[MockServerContext] CloseConnection called" << std::endl;
    }
private:
    // ⚠️ 这里必须声明成员变量
    std::unique_ptr<ServerFilterController> server_filter_controller_;
};

}  // namespace trpc


int main() {
  using namespace trpc;
  using namespace trpc::http::sse;

  // 1. 创建 MockServerContext
  auto ctx = new MockServerContext();
  RefPtr<ServerContext> ctx_ref(adopt_ptr, ctx);
  std::cout << "[INFO] Creating SseStreamWriter..." << std::endl;
  std::cout << "[INFO] HasFilterController = " << ctx_ref->HasFilterController() << std::endl;
  auto writer = std::make_shared<SseStreamWriter>(ctx_ref,false);

  SseEvent test_event{};
  test_event.event_type = "message";
  test_event.data       = "Hello World";
  test_event.id         = "123";
  std::cout << "[INFO] Writing SSE event..." << std::endl;
  bool ok = writer->Write(test_event);
  std::cout << "[INFO] Write result = " << (ok ? "success" : "fail") << std::endl;

  std::cout << "[INFO] Closing writer..." << std::endl;
  writer->Close();
}

