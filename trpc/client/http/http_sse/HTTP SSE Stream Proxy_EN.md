# HTTP SSE Stream Proxy

This directory contains the **HTTP Server-Sent Events (SSE) synchronous stream client proxy** implementation for tRPC-Cpp.
 It is built on top of the HTTP streaming interface and provides a convenient way to create and manage SSE client streams.

## Overview

`HttpSseStreamProxy` extends `HttpStreamProxy` to provide a **synchronous-style (Fiber suspend/wait)** interface for SSE stream creation.
 Key features:

- **SSE Protocol Support**: Automatically sets `GET` + `text/event-stream` headers according to the SSE specification
- **Synchronous Experience**: Uses `Future::Wait` to suspend the current Fiber without blocking a physical thread
- **Filter Integration**: Fully compatible with tRPC's client filter mechanism
- **Easy Integration**: Returns a standard `HttpClientAsyncStreamReaderWriterPtr` for direct streaming read/write

## Components

### `HttpSseStreamProxy`

#### Header File

```
#include "trpc/client/http/http_sse/http_sse_stream_proxy.h"
```

#### Class Definition

```
namespace trpc::stream {

class HttpSseStreamProxy : public HttpStreamProxy {
 public:
  HttpClientAsyncStreamReaderWriterPtr GetStreamReaderWriter(const ClientContextPtr& ctx);

 protected:
  TransInfo ProxyOptionToTransInfo() override;

 private:
  static void PrepareSseRequest(HttpRequestProtocol* req);
};

} // namespace trpc::stream
```

#### Key Methods

| Method                   | Parameters                    | Return                                 | Description                                                  |
| ------------------------ | ----------------------------- | -------------------------------------- | ------------------------------------------------------------ |
| `GetStreamReaderWriter`  | `const ClientContextPtr& ctx` | `HttpClientAsyncStreamReaderWriterPtr` | Creates and returns an SSE stream reader/writer, waiting synchronously until the stream is ready |
| `PrepareSseRequest`      | `HttpRequestProtocol* req`    | `void`                                 | Adds HTTP method and SSE-specific headers                    |
| `ProxyOptionToTransInfo` | —                             | `TransInfo`                            | Marks the proxy as a stream proxy, inherits base settings    |

### Automatic Request Header Setup

`PrepareSseRequest` sets the following headers for every SSE request:

- Method: `GET`
- `Accept: text/event-stream`
- `Cache-Control: no-cache`
- `Connection: keep-alive`
- `Accept-Encoding: identity` (disables compression to keep SSE event boundaries predictable)

## Usage Example

```
#include "trpc/client/http/http_sse/http_sse_stream_proxy.h"
#include "trpc/client/service_proxy.h"

using namespace trpc;
using namespace trpc::stream;

void ExampleSseClient() {
  auto sse_proxy = std::make_shared<HttpSseStreamProxy>();
  ClientContextPtr ctx = std::make_shared<ClientContext>();

  // Configure request (e.g., URL, timeout)
  ctx->SetTimeout(5000);

  auto stream = sse_proxy->GetStreamReaderWriter(ctx);
  if (!stream) {
    TRPC_LOG_ERROR("Failed to create SSE stream: " << ctx->GetStatus().ErrorMessage());
    return;
  }

  // Use stream->Read() / stream->Write() to exchange SSE data
}
```

## Building

### Prerequisites

- Bazel build system
- C++17 or later
- tRPC-Cpp framework

### Dependencies

- `//trpc/client/http:http_stream_proxy`
- `//trpc/runtime/threadmodel/merge:merge_thread_model`

### Build Commands

```
# Build SSE stream proxy library
bazel build //trpc/client/http/http_sse:http_sse_stream_proxy

# Build and run tests
bazel test //trpc/client/http/http_sse:http_sse_stream_proxy_test --test_output=all
```

## Testing

The test suite (`http_sse_stream_proxy_test.cc`) covers:

- ✅ SSE response status line and header writing
- ✅ Data chunk writing (`data: ...` format)
- ✅ Completion signal (`WriteDone`)

Run tests:

```
bazel test //trpc/client/http/http_sse:http_sse_stream_proxy_test --test_output=all
```

## Notes

- **Synchronous Waiting**: `GetStreamReaderWriter` suspends the current Fiber; in a non-Fiber environment it will block the thread
- **Error Handling**: Returns `nullptr` on failure and sets an appropriate status in the `ctx`
- **Compression Control**: By default, disables compression (`Accept-Encoding: identity`) to prevent breaking SSE event boundaries
- **Thread Safety**: Safe for use in multi-Fiber/multi-threaded environments

## Related Documentation

- [tRPC-Cpp Framework](https://github.com/trpc-group/trpc-cpp)
- [W3C SSE Specification](https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events)