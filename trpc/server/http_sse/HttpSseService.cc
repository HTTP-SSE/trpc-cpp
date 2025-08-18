#include "trpc/server/http_sse/HttpSseService.h"

#include "trpc/util/log/logging.h"
#include "trpc/util/ref_ptr.h"  // RefPtr

namespace trpc {

HttpSseService::~HttpSseService() {
  Shutdown();
}

uint64_t HttpSseService::AcceptConnection(const ServerContextPtr& ctx) {
  if (!ctx) return 0;

  // Ensure filter controller etc. are present as needed by SseStreamWriter / codec
  if (!ctx->HasFilterController()) {
    TRPC_LOG_WARN("AcceptConnection: ctx has no FilterController set");
    // you may choose to reject, here we continue but log
  }

  // Create SseStreamWriter. It will send initial headers in ctor.
  std::shared_ptr<SseStreamWriter> writer;
  try {
    writer = std::make_shared<SseStreamWriter>(ctx,false);
  } catch (const std::exception& e) {
    TRPC_LOG_ERROR("AcceptConnection: failed to create SseStreamWriter: " << e.what());
    return 0;
  }

  uint64_t cid = RegisterConnection(ctx, writer);

  // Save client id into ctx->user_data if you want later mapping
  ctx->SetUserData(cid);

  TRPC_LOG_INFO("AcceptConnection: client_id=" << cid);
  return cid;
}

bool HttpSseService::HandleSseRequest(const ServerContextPtr& ctx) {
  // This helper is a convenience: call it from your route handler when you detect SSE request.
  // It just accepts and registers the connection. The actual streaming will be done by our SendToClient/Broadcast APIs.
  uint64_t cid = AcceptConnection(ctx);
  return cid != 0;
}

uint64_t HttpSseService::RegisterConnection(const ServerContextPtr& ctx, const std::shared_ptr<SseStreamWriter>& writer) {
  auto conn = std::make_shared<Connection>();
  conn->ctx = ctx;
  conn->writer = writer;
  conn->client_id = next_client_id_.fetch_add(1);

  std::lock_guard<std::mutex> lk(mu_);
  connections_.emplace(conn->client_id, conn);
  return conn->client_id;
}

void HttpSseService::UnregisterConnection(uint64_t client_id) {
  std::lock_guard<std::mutex> lk(mu_);
  auto it = connections_.find(client_id);
  if (it != connections_.end()) {
    // ensure writer closed and ctx cleared
    try {
      if (it->second->writer) it->second->writer->Close();
    } catch (...) {
    }
    it->second->ctx = RefPtr<ServerContext>();  // reset
    connections_.erase(it);
    TRPC_LOG_INFO("UnregisterConnection: client_id=" << client_id);
  }
}

bool HttpSseService::SendToClient(uint64_t client_id, const http::sse::SseEvent& event) {
  std::shared_ptr<Connection> conn;
  {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = connections_.find(client_id);
    if (it == connections_.end()) return false;
    conn = it->second;
  }

  if (!conn || !conn->open.load(std::memory_order_acquire) || !conn->writer) return false;

  bool ok = conn->writer->Write(event);
  if (!ok) {
    // failed to write -> unregister
    TRPC_LOG_WARN("SendToClient: write failed for client " << client_id << ", unregistering");
    UnregisterConnection(client_id);
  }
  return ok;
}
//broadcast
size_t HttpSseService::Broadcast(const http::sse::SseEvent& event) {
  std::vector<std::shared_ptr<Connection>> snapshot;
  {
    std::lock_guard<std::mutex> lk(mu_);
    snapshot.reserve(connections_.size());
    for (auto& kv : connections_) snapshot.push_back(kv.second);
  }

  size_t succ = 0;
  for (auto& c : snapshot) {
    if (!c) continue;
    if (!c->open.load(std::memory_order_acquire) || !c->writer) continue;
    if (c->writer->Write(event)) {
      ++succ;
    } else {
      // best-effort cleanup for failed writers: schedule unregister
      UnregisterConnection(c->client_id);
    }
  }
  return succ;
}

void HttpSseService::CloseClient(uint64_t client_id) {
  std::shared_ptr<Connection> conn;
  {
    std::lock_guard<std::mutex> lk(mu_);
    auto it = connections_.find(client_id);
    if (it == connections_.end()) return;
    conn = it->second;
  }
  if (conn && conn->writer) {
    conn->writer->Close();
  }
  UnregisterConnection(client_id);
}

void HttpSseService::Shutdown() {
  std::vector<std::shared_ptr<Connection>> snapshot;
  {
    std::lock_guard<std::mutex> lk(mu_);
    snapshot.reserve(connections_.size());
    for (auto& kv : connections_) snapshot.push_back(kv.second);
    connections_.clear();
  }

  for (auto& c : snapshot) {
    if (!c) continue;
    try {
      if (c->writer) c->writer->Close();
      if (c->ctx) c->ctx->CloseConnection();
    } catch (...) {
    }
  }
}

}  // namespace trpc

