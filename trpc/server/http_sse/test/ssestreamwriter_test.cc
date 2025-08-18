// sse_stream_writer_test.cpp
// 单文件测试：SseStreamWriter 的核心发送逻辑（依赖注入 + fake 实现）
// 编译示例： g++ -std=c++17 sse_stream_writer_test.cpp -o sse_test
// 运行： ./sse_test

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <sstream>
#include <cassert>
// 下面这几个头是你之前缺失的（导致错误）：
#include <optional>   // std::optional
#include <future>     // std::future, std::async
#include <cstdint>    // uint32_t

// --------------------
// 简化/最小化的 SseEvent（基于你队友实现的 ToString 语义）
// --------------------
struct SseEvent {
  std::string event_type;
  std::string data;
  std::optional<std::string> id;
  std::optional<uint32_t> retry;

  std::string ToString() const {
    std::string result;
    if (!event_type.empty()) {
      result += "event: " + event_type + "\n";
    }
    if (!data.empty()) {
      // 支持原始实现中把 literal "\n" 当成分行的行为较特殊，这里简化为按真实换行处理
      std::istringstream iss(data);
      std::string line;
      while (std::getline(iss, line)) {
        result += "data: " + line + "\n";
      }
    }
    if (id.has_value() && !id->empty()) {
      result += "id: " + *id + "\n";
    }
    if (retry.has_value()) {
      result += "retry: " + std::to_string(*retry) + "\n";
    }
    result += "\n"; // 事件结束的空行
    return result;
  }
};

// --------------------
// 支持类型：NoncontiguousBuffer（简单用 vector<uint8_t>）
// --------------------
using NoncontiguousBuffer = std::vector<uint8_t>;

// --------------------
// Fake HttpSseResponseProtocol（只存 content）
// --------------------
struct HttpResponseFake {
  std::string content;
  void SetContent(const std::string& s) { content = s; }
  std::string GetContent() const { return content; }
};

// --------------------
// ICodec 接口（抽象 ZeroCopyEncode）
// --------------------
struct ICodec {
  virtual ~ICodec() = default;
  // 模拟签名: bool ZeroCopyEncode(ctx, proto_any, out)
  // proto_any 在测试中我们传的是 HttpResponseFake*
  virtual bool ZeroCopyEncode(void* /*ctx*/, HttpResponseFake* proto, NoncontiguousBuffer& out) = 0;
};

// --------------------
// FakeCodec：把 proto->content 转成 out 并在前面加 "HEADER\n" 以模拟 header 注入
// --------------------
struct FakeCodec : ICodec {
  bool ZeroCopyEncode(void* /*ctx*/, HttpResponseFake* proto, NoncontiguousBuffer& out) override {
    if (!proto) return false;
    std::string header = "HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\n\r\n";
    std::string payload = header + proto->GetContent();
    out.assign(payload.begin(), payload.end());
    return true;
  }
};

// --------------------
// IServerContext 接口（抽象 SendResponse & CloseConnection）
// --------------------
struct IServerContext {
  virtual ~IServerContext() = default;
  virtual void SendResponse(NoncontiguousBuffer&& buf) = 0;
  virtual void CloseConnection() = 0;
};

// --------------------
// FakeServerContext：记录最后一次传入的 buffer，供断言使用
// --------------------
struct FakeServerContext : IServerContext {
  std::mutex mu;
  std::vector<NoncontiguousBuffer> sent_buffers;
  bool closed = false;

  void SendResponse(NoncontiguousBuffer&& buf) override {
    std::lock_guard<std::mutex> lk(mu);
    sent_buffers.emplace_back(std::move(buf));
  }
  void CloseConnection() override {
    std::lock_guard<std::mutex> lk(mu);
    closed = true;
  }
};

// --------------------
// SseStreamWriterTestable：可注入 codec & context 的可测试版本
// 与之前讨论的 SseStreamWriter 行为一致（每次 Write 使用 codec 编码并调用 ctx->SendResponse）
// --------------------
class SseStreamWriterTestable : public std::enable_shared_from_this<SseStreamWriterTestable> {
 public:
  SseStreamWriterTestable(std::shared_ptr<IServerContext> ctx, std::shared_ptr<ICodec> codec)
    : ctx_(std::move(ctx)), codec_(std::move(codec)), open_(true) {}

  bool Write(const SseEvent& event) {
    std::lock_guard<std::mutex> lk(mu_);
    if (!open_.load()) return false;

    // 构造 fake response protocol 等价物
    HttpResponseFake proto;
    proto.SetContent(event.ToString());

    NoncontiguousBuffer out;
    bool ok = codec_->ZeroCopyEncode(ctx_.get(), &proto, out);
    if (!ok) {
      open_.store(false);
      return false;
    }

    // 发送
    try {
      ctx_->SendResponse(std::move(out));
      return true;
    } catch (...) {
      open_.store(false);
      return false;
    }
  }

  std::future<bool> WriteAsync(const SseEvent& event) {
    auto self = shared_from_this();
    return std::async(std::launch::async, [self, event](){ return self->Write(event); });
  }

  void Close() {
    bool expected = true;
    if (!open_.compare_exchange_strong(expected, false)) return;
    ctx_->CloseConnection();
  }

  bool IsOpen() const { return open_.load(); }

 private:
  std::shared_ptr<IServerContext> ctx_;
  std::shared_ptr<ICodec> codec_;
  std::mutex mu_;
  std::atomic<bool> open_;
};

// --------------------
// 测试辅助：将 buffer 转为字符串
// --------------------
std::string BufToString(const NoncontiguousBuffer& b) {
  return std::string(b.begin(), b.end());
}

// --------------------
// 测试用例 1：单次 Write，检查发送数据包含 header + body（由 FakeCodec 生成）
// --------------------
bool TestSingleWrite() {
  auto ctx = std::make_shared<FakeServerContext>();
  auto codec = std::make_shared<FakeCodec>();
  auto writer = std::make_shared<SseStreamWriterTestable>(ctx, codec);

  SseEvent ev;
  ev.event_type = "message";
  ev.data = "hello world";
  ev.id = std::string("42");

  bool ok = writer->Write(ev);
  if (!ok) {
    std::cerr << "[FAIL] Write returned false\n";
    return false;
  }

  // 查看 fake context 是否有一条发送记录
  {
    std::lock_guard<std::mutex> lk(ctx->mu);
    if (ctx->sent_buffers.size() != 1) {
      std::cerr << "[FAIL] expected 1 buffer sent, got " << ctx->sent_buffers.size() << "\n";
      return false;
    }
    std::string s = BufToString(ctx->sent_buffers[0]);
    // 检查 header 与 body 片段
    if (s.find("Content-Type: text/event-stream") == std::string::npos) {
      std::cerr << "[FAIL] header missing\n";
      return false;
    }
    if (s.find("data: hello world") == std::string::npos) {
      std::cerr << "[FAIL] body missing\n";
      return false;
    }
    if (s.find("id: 42") == std::string::npos) {
      std::cerr << "[FAIL] id missing\n";
      return false;
    }
  }

  writer->Close();
  if (!ctx->closed) {
    std::cerr << "[FAIL] expected context closed after Close()\n";
    return false;
  }

  return true;
}

// --------------------
// 测试用例 2：连续两次 Write，检查发送两次数据且各自包含事件内容
// --------------------
bool TestMultipleWrites() {
  auto ctx = std::make_shared<FakeServerContext>();
  auto codec = std::make_shared<FakeCodec>();
  auto writer = std::make_shared<SseStreamWriterTestable>(ctx, codec);

  SseEvent ev1; ev1.event_type = "m"; ev1.data = "one"; ev1.id = std::string("1");
  SseEvent ev2; ev2.event_type = "m"; ev2.data = "two"; ev2.id = std::string("2");

  bool ok1 = writer->Write(ev1);
  bool ok2 = writer->Write(ev2);
  if (!ok1 || !ok2) {
    std::cerr << "[FAIL] one of writes returned false\n";
    return false;
  }

  {
    std::lock_guard<std::mutex> lk(ctx->mu);
    if (ctx->sent_buffers.size() != 2) {
      std::cerr << "[FAIL] expected 2 buffers sent, got " << ctx->sent_buffers.size() << "\n";
      return false;
    }
    std::string s1 = BufToString(ctx->sent_buffers[0]);
    std::string s2 = BufToString(ctx->sent_buffers[1]);
    if (s1.find("data: one") == std::string::npos) { std::cerr << "[FAIL] ev1 body missing\n"; return false; }
    if (s2.find("data: two") == std::string::npos) { std::cerr << "[FAIL] ev2 body missing\n"; return false; }
  }

  writer->Close();
  return true;
}

// --------------------
// main：运行测试并输出结果
// --------------------
int main() {
  bool ok1 = TestSingleWrite();
  std::cout << "TestSingleWrite: " << (ok1 ? "PASS" : "FAIL") << "\n";

  bool ok2 = TestMultipleWrites();
  std::cout << "TestMultipleWrites: " << (ok2 ? "PASS" : "FAIL") << "\n";

  return (ok1 && ok2) ? 0 : 1;
}

