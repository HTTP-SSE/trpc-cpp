#include "trpc/server/http_sse/HttpSseService.h"

#include "trpc/util/log/logging.h"
#include "trpc/util/ref_ptr.h"  // RefPtr

namespace trpc {

HttpSseService::~HttpSseService() { Shutdown(); }

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
    writer = std::make_shared<SseStreamWriter>(ctx, false);
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

/**
 * @brief Handles an incoming Server-Sent Events (SSE) request.
 * @param ctx The server context associated with the incoming request.
 * @return true if the connection was successfully accepted and registered (non-zero connection ID),false otherwise.
 */
bool HttpSseService::HandleSseRequest(const ServerContextPtr& ctx) {
  // This helper is a convenience: call it from your route handler when you detect SSE request.
  // It just accepts and registers the connection. The actual streaming will be done by our SendToClient/Broadcast APIs.
  uint64_t cid = AcceptConnection(ctx);
  return cid != 0;
}

/**
 * @brief Registers a new SSE (Server-Sent Events) connection.
 * @param ctx The server context associated with the connection.
 * @param writer A shared pointer to the SSE stream writer for sending data to the client.
 * @return The unique client ID assigned to the newly registered connection.
 * @note This function is thread-safe and uses a mutex to protect access to the internal connection map.
 */
uint64_t HttpSseService::RegisterConnection(const ServerContextPtr& ctx,
                                            const std::shared_ptr<SseStreamWriter>& writer) {
  auto conn = std::make_shared<Connection>();
  conn->ctx = ctx;
  conn->writer = writer;
  conn->client_id = next_client_id_.fetch_add(1);

  std::lock_guard<std::mutex> lk(mu_);
  connections_.emplace(conn->client_id, conn);
  return conn->client_id;
}

/**
 * @brief Unregisters a client connection identified by its client ID.
 * @param client_id The unique identifier of the client connection to unregister.
 */
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

/**
 * @brief Sends an SSE (Server-Sent Event) to a specific client identified by its ID.
 * @param client_id The unique identifier of the client to send the event to.
 * @param event The SSE event to be sent to the client.
 * @return true if the event was successfully sent to the client; false otherwise.
 */
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

/**
 * @brief Broadcasts an SSE (Server-Sent Event) to all connected clients.
 * @param event The SSE event to be broadcasted to all connections.
 * @return The number of connections that successfully received the event.
 */
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

/**
 * @brief Closes the connection for a specific client identified by its ID.
 * @param client_id The unique identifier of the client whose connection should be closed.
 */
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

/**
 * @brief Shuts down the HttpSseService by closing all active connections.
 *
 * This method performs the following steps:
 * 1. Acquires a lock to safely access the internal connections map.
 * 2. Creates a snapshot of all active connections and clears the internal map.
 * 3. Iterates over the snapshot and closes each connection gracefully.
 *    - If the connection has a writer, it calls the `Close` method on the writer.
 *    - If the connection has a context, it calls the `CloseConnection` method on the context.
 *
 * Any exceptions thrown during the closing of individual connections are caught and ignored.
 * This ensures that the shutdown process continues for all connections without interruption.
 */
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
