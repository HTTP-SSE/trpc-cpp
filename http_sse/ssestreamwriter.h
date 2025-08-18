#pragma once
#include <memory>
#include <string>
#include <mutex>
#include <atomic>
#include <future>
#include <iostream>

#include "trpc/codec/http_sse/http_sse_codec.h" // HttpSseResponseProtocol / HttpSseServerCodec
#include "trpc/util/http/sse/sse_event.h"       // SseEvent
#include "trpc/server/server_context.h"         // ServerContextPtr
#include "trpc/util/ref_ptr.h"                  // RefPtr

namespace trpc {

class SseStreamWriter : public std::enable_shared_from_this<SseStreamWriter> {
 public:
  explicit SseStreamWriter(const ServerContextPtr& ctx,bool send_initial_headers = true)
      : ctx_(ctx), open_(true) {
    if (send_initial_headers) SendInitialHeaders();
  }

  ~SseStreamWriter() { Close(); }

  bool Write(const trpc::http::sse::SseEvent& event) {
    std::lock_guard<std::mutex> lk(mu_);
    std::cout << "[Write] Acquired lock, open_ = " << open_.load() << std::endl;

    if (!open_.load(std::memory_order_acquire)) {
        std::cout << "[Write] Stream closed, returning false" << std::endl;
        return false;
    }

    auto proto = std::make_shared<::trpc::HttpSseResponseProtocol>();
    proto->SetSseEvent(event);
    std::cout << "[Write] SseEvent set in protocol" << std::endl;

    ::trpc::ProtocolPtr p = proto;
    ::trpc::HttpSseServerCodec codec;
    ::trpc::NoncontiguousBuffer out_buf;

    bool ok = codec.ZeroCopyEncode(ctx_, p, out_buf);
    std::cout << "[Write] ZeroCopyEncode returned " << ok << std::endl;
    
    if (!ok) {
        open_.store(false);
       // TRPC_LOG_ERROR("SseStreamWriter: ZeroCopyEncode failed");
        std::cout << "[ERROR] ZeroCopyEncode failed" << std::endl;
        return false;
    }
    std::cout << "[Write] out_buf.blocks() = " << out_buf.size()
          << ", out_buf.ByteSize() = " << out_buf.ByteSize() << std::endl;
    if (!ctx_) {
    std::cout << "[ERROR] ctx_ is null!" << std::endl;
    return false;}
    try {
        ctx_->SendResponse(std::move(out_buf));
        std::cout << "[Write] SendResponse succeeded" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cout << "[Write] Exception in SendResponse: " << e.what() << std::endl;
        open_.store(false);
        return false;
    } catch (...) {
        std::cout << "[Write] Unknown exception in SendResponse" << std::endl;
        open_.store(false);
        return false;
    }
}


//  std::future<bool> WriteAsync(const SseEvent& event) {
  //  auto self = shared_from_this();
  //  return std::async(std::launch::async, [self, event]() {
   //   return self->Write(event);
   // });
//  }

  bool IsOpen() const { return open_.load(std::memory_order_acquire); }

  void Close() {
    bool exp = true;
    if (!open_.compare_exchange_strong(exp, false)) return;
    if (ctx_) ctx_->CloseConnection();
  }

 private:
  void SendInitialHeaders() {
    if (!ctx_) return;
    auto proto = std::make_shared<::trpc::HttpSseResponseProtocol>();
    ::trpc::ProtocolPtr p = proto;
    ::trpc::HttpSseServerCodec codec;
    ::trpc::NoncontiguousBuffer out_buf;
    bool ok = codec.ZeroCopyEncode(ctx_, p, out_buf);
    if (ok) {
      try {
        ctx_->SendResponse(std::move(out_buf));
      } catch (...) {
        open_.store(false);
      }
    } else {
      open_.store(false);
    }
  }

 private:
  ServerContextPtr ctx_; // RefPtr<ServerContext>
  std::mutex mu_;
  std::atomic<bool> open_;
};

}  // namespace trpc

