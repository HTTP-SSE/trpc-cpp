#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <memory>
#include <functional>

// ==== 模拟 tRPC 里用到的类型 ====
namespace trpc {
namespace stream {

// 最小化 HttpStatusLine
struct HttpStatusLine {
    int status_code{0};
    std::string version;
};

// 最小化 HttpHeader
class HttpHeader {
public:
    void SetHeader(const std::string& key, const std::string& value) {
        headers_.push_back({key, value});
    }
    std::string GetHeader(const std::string& key) const {
        for (auto& kv : headers_) {
            if (kv.first == key) return kv.second;
        }
        return {};
    }
private:
    std::vector<std::pair<std::string, std::string>> headers_;
};

// 模拟 NoncontiguousBuffer
class NoncontiguousBuffer {
public:
    void Append(const char* data, size_t size) {
        data_.append(data, size);
    }
    std::string ToString() const { return data_; }
private:
    std::string data_;
};

// 模拟 HttpServerAsyncStreamReaderWriter 接口
class HttpServerAsyncStreamReaderWriter {
public:
    virtual ~HttpServerAsyncStreamReaderWriter() = default;
    virtual void WriteStatusLine(HttpStatusLine&&) = 0;
    virtual void WriteHeader(HttpHeader&&) = 0;
    virtual void WriteData(NoncontiguousBuffer&&) = 0;
    virtual void WriteDone() = 0;
};

} // namespace stream
} // namespace trpc

using namespace trpc::stream;

// ==== 我们的 Fake 实现 ====
class FakeStreamRW : public HttpServerAsyncStreamReaderWriter {
public:
    HttpStatusLine status;
    HttpHeader header;
    std::vector<std::string> chunks;
    bool done = false;

    void WriteStatusLine(HttpStatusLine&& s) override { status = std::move(s); }
    void WriteHeader(HttpHeader&& h) override { header = std::move(h); }
    void WriteData(NoncontiguousBuffer&& buf) override { chunks.push_back(buf.ToString()); }
    void WriteDone() override { done = true; }
};

// ==== 模拟 SSE 服务逻辑 ====
void HandleSse(HttpServerAsyncStreamReaderWriter* rw) {
    HttpStatusLine status_line;
    status_line.status_code = 200;
    status_line.version = "HTTP/1.1";
    rw->WriteStatusLine(std::move(status_line));

    HttpHeader header;
    header.SetHeader("Content-Type", "text/event-stream");
    header.SetHeader("Cache-Control", "no-cache");
    header.SetHeader("Connection", "keep-alive");
    rw->WriteHeader(std::move(header));

    NoncontiguousBuffer buf;
    std::string event = "data: hello\n\n";
    buf.Append(event.data(), event.size());
    rw->WriteData(std::move(buf));

    rw->WriteDone();
}

// ==== 测试 ====
TEST(HttpSseStreamProxyTest, BasicSseFlow_NoDeps) {
    auto fake_rw = std::make_unique<FakeStreamRW>();
    HandleSse(fake_rw.get());

    EXPECT_EQ(fake_rw->status.status_code, 200);
    EXPECT_EQ(fake_rw->header.GetHeader("Content-Type"), "text/event-stream");
    ASSERT_FALSE(fake_rw->chunks.empty());
    EXPECT_EQ(fake_rw->chunks[0], "data: hello\n\n");
    EXPECT_TRUE(fake_rw->done);
}

