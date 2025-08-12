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
#include "trpc/stream/http/async/stream_reader_writer.h"  

namespace trpc::stream {

class HttpSseStreamProxy : public HttpStreamProxy {
 public:
  Future<HttpClientAsyncStreamReaderWriterPtr> GetAsyncStreamReaderWriter(const ClientContextPtr& ctx);

 protected:
  void SetupSseParameters(const ClientContextPtr& ctx);
};

}  // namespace trpc::stream
