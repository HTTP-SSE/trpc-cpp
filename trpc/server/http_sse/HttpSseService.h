#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "trpc/server/server_context.h"
#include "trpc/server/http_sse/ssestreamwriter.h"
#include "trpc/util/ref_ptr.h"
#include "trpc/util/http/sse/sse_event.h"

namespace trpc {

class HttpSseService {
 public:
  HttpSseService() = default;
  ~HttpSseService();

  // Accept a connection and register it. Returns assigned client_id (>0) on success, 0 on failure.
  // ctx should be a valid ServerContextPtr and typically comes from HttpService routing.
  uint64_t AcceptConnection(const ServerContextPtr& ctx);

  // Convenience handler matching typical signature inside an HTTP handler:
  // - ctx: server context
  // - req/rsp are optional; here we only need ctx to register stream
  // Returns true if accepted and streaming will be handled by service.
  bool HandleSseRequest(const ServerContextPtr& ctx);

  // Send to a single client. Returns true on success, false on failure (and will unregister on persistent failures).
  bool SendToClient(uint64_t client_id, const http::sse::SseEvent& event);

  // Broadcast to all connected clients. Returns number of clients that succeeded.
  size_t Broadcast(const http::sse::SseEvent& event);

  // Close a client explicitly.
  void CloseClient(uint64_t client_id);

  // Shutdown service and close all clients.
  void Shutdown();

 private:
  struct Connection {
    uint64_t client_id{0};
    RefPtr<ServerContext> ctx;                       // non-owning wrapper that keeps ctx alive
    std::shared_ptr<SseStreamWriter> writer;         // writer for this connection
    std::atomic<bool> open{true};
  };

  uint64_t RegisterConnection(const ServerContextPtr& ctx, const std::shared_ptr<SseStreamWriter>& writer);
  void UnregisterConnection(uint64_t client_id);

  std::mutex mu_;
  std::unordered_map<uint64_t, std::shared_ptr<Connection>> connections_;
  std::atomic<uint64_t> next_client_id_{1};
};

}  // namespace trpc

