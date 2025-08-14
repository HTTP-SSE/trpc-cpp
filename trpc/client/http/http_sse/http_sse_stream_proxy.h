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

#pragma once

#include "trpc/client/http/http_stream_proxy.h"
#include "trpc/codec/http/http_protocol.h"

namespace trpc::stream {

/// @brief HTTP SSE (Server-Sent Events) 客户端代理（同步风格，基于 Future::Wait 挂起/等待）。
/// 返回类型为 HttpClientAsyncStreamReaderWriterPtr，调用方可以像同步接口一样使用（在 Fiber 中不会阻塞物理线程）。
class HttpSseStreamProxy : public HttpStreamProxy {
 public:
  /// @brief 获取 SSE 同步流读写器（阻塞直到创建完成）。
  /// @param ctx 客户端上下文（包含请求、超时等）
  /// @return 已创建的 HttpClientAsyncStreamReaderWriterPtr，失败时返回 nullptr 并在 ctx 中设置状态
  HttpClientAsyncStreamReaderWriterPtr GetStreamReaderWriter(const ClientContextPtr& ctx);

 protected:
  TransInfo ProxyOptionToTransInfo() override {
    TransInfo trans_info = HttpStreamProxy::ProxyOptionToTransInfo();
    trans_info.is_stream_proxy = true;
    return trans_info;
  }

 private:
  /// @brief 按 SSE 规范补齐请求方法与头部（GET + text/event-stream 等）。
  static void PrepareSseRequest(HttpRequestProtocol* req);
};

}  // namespace trpc::stream
