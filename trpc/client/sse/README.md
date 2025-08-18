# HTTP SSE Stream Proxy

This directory contains the HTTP Server-Sent Events (SSE) stream proxy implementation for tRPC-Cpp, providing client-side SSE connection management and streaming capabilities.

## Overview

The HTTP SSE Stream Proxy extends the existing tRPC-Cpp infrastructure to handle Server-Sent Events over HTTP connections. It provides:

- **SSE Protocol Compliance**: Full compliance with [W3C Server-Sent Events specification](https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events)
- **Stream Management**: Proper HTTP stream creation and lifecycle management
- **Error Handling**: Comprehensive error handling with appropriate status codes
- **Header Management**: Automatic SSE-specific HTTP header configuration
- **Timeout Configuration**: Optimized timeout settings for long-lived SSE connections

## Components

### HttpSseStreamProxy

The main stream proxy class that inherits from `ServiceProxy` to leverage existing tRPC infrastructure.

#### Header File
```cpp
#include "trpc/client/http_sse/http_sse_stream_proxy.h"
```

#### Class Definition
```cpp
namespace trpc::stream {

class HttpSseStreamProxy : public ServiceProxy {
 public:
  HttpSseStreamProxy() = default;
  ~HttpSseStreamProxy() override = default;

  /// @brief Get synchronous stream reader/writer for SSE connection
  /// @param ctx Client context containing request/response data
  /// @return HttpClientStreamReaderWriter for reading SSE events
  HttpClientStreamReaderWriter GetSyncStreamReaderWriter(const ClientContextPtr& ctx);

  /// @brief Setup SSE-specific parameters (headers, timeout, etc.)
  /// @param ctx Client context to configure
  /// @return true if setup was successful, false otherwise
  bool SetupSseParameters(const ClientContextPtr& ctx);

  /// @brief Create SSE-specific request protocol
  /// @return Request protocol pointer
  ProtocolPtr CreateSseRequestProtocol();

 protected:
};

} // namespace trpc::stream
```

## Public Interfaces

### 1. GetSyncStreamReaderWriter

Creates and returns a synchronous HTTP client stream reader/writer for SSE connections.

#### Signature
```cpp
HttpClientStreamReaderWriter GetSyncStreamReaderWriter(const ClientContextPtr& ctx);
```

#### Parameters
- `ctx`: Client context containing request/response data and configuration

#### Returns
- `HttpClientStreamReaderWriter`: Stream reader/writer for SSE event processing

#### Behavior
- Validates the client context
- Sets up SSE-specific parameters (headers, timeout)
- Creates HTTP request protocol if not present
- Establishes HTTP client stream connection
- Configures stream for SSE operations
- Sends request headers to establish SSE connection

#### Error Handling
- Returns error stream with `ENCODE_ERROR` status for null context
- Returns error stream for failed parameter setup
- Returns error stream for invalid request protocol
- Returns error stream for failed stream provider creation

### 2. SetupSseParameters

Configures SSE-specific parameters for the client context.

#### Signature
```cpp
bool SetupSseParameters(const ClientContextPtr& ctx);
```

#### Parameters
- `ctx`: Client context to configure

#### Returns
- `bool`: `true` if setup was successful, `false` otherwise

#### Configuration Applied
- **Headers**:
  - `Accept: text/event-stream`
  - `Cache-Control: no-cache`
  - `Connection: keep-alive`
- **Timeout**: 60 seconds (optimized for long-lived SSE connections)
- **Request Protocol**: Creates `HttpSseRequestProtocol` if not present

#### Error Handling
- Returns `false` for null context
- Returns `false` for invalid HTTP request protocol
- Returns `false` for invalid HTTP request object

### 3. CreateSseRequestProtocol

Creates a new SSE-specific request protocol.

#### Signature
```cpp
ProtocolPtr CreateSseRequestProtocol();
```

#### Returns
- `ProtocolPtr`: HTTP SSE request protocol

#### Default Configuration
- **HTTP Method**: `GET` (standard for SSE connections)
- **URL**: `/` (if not specified)
- **Protocol Type**: `HttpSseRequestProtocol`

## Usage Examples

### Basic SSE Stream Creation

```cpp
#include "trpc/client/http_sse/http_sse_stream_proxy.h"
#include "trpc/client/make_client_context.h"

// Create SSE stream proxy
auto proxy = std::make_shared<trpc::stream::HttpSseStreamProxy>();

// Create client context
auto ctx = trpc::MakeClientContext(proxy);
ctx->SetTimeout(60000);  // 60 seconds for SSE

// Get stream reader/writer
auto stream = proxy->GetSyncStreamReaderWriter(ctx);

// Check if stream creation was successful
if (!stream.GetStatus().OK()) {
    std::cerr << "Failed to create SSE stream: " << stream.GetStatus().ErrorMessage() << std::endl;
    return;
}

// Read SSE events
trpc::NoncontiguousBuffer buffer;
while (stream.GetStatus().OK()) {
    auto status = stream.Read(buffer, 1024);
    if (status.OK()) {
        // Process SSE event data
        std::string event_data = buffer.ToString();
        std::cout << "Received SSE event: " << event_data << std::endl;
    } else if (status.StreamEof()) {
        // End of stream
        break;
    } else {
        // Error occurred
        std::cerr << "Error reading SSE stream: " << status.ErrorMessage() << std::endl;
        break;
    }
}

// Close the stream
stream.Close();
```

### Custom SSE Configuration

```cpp
#include "trpc/client/http_sse/http_sse_stream_proxy.h"
#include "trpc/client/make_client_context.h"

// Create SSE stream proxy
auto proxy = std::make_shared<trpc::stream::HttpSseStreamProxy>();

// Create client context with custom configuration
auto ctx = trpc::MakeClientContext(proxy);

// Set custom headers
ctx->SetHttpHeader("Authorization", "Bearer your-token");
ctx->SetHttpHeader("User-Agent", "MySSEClient/1.0");

// Setup SSE parameters
if (!proxy->SetupSseParameters(ctx)) {
    std::cerr << "Failed to setup SSE parameters" << std::endl;
    return;
}

// Create custom request protocol
auto protocol = proxy->CreateSseRequestProtocol();
auto* http_protocol = dynamic_cast<trpc::HttpRequestProtocol*>(protocol.get());
if (http_protocol) {
    http_protocol->request->SetUrl("/events/stream");
    http_protocol->request->SetMethod("GET");
}
ctx->SetRequest(protocol);

// Get stream reader/writer
auto stream = proxy->GetSyncStreamReaderWriter(ctx);

// Use the stream for SSE event processing
// ... (same as basic example)
```

### Error Handling Example

```cpp
#include "trpc/client/http_sse/http_sse_stream_proxy.h"
#include "trpc/client/make_client_context.h"

auto proxy = std::make_shared<trpc::stream::HttpSseStreamProxy>();

// Test with null context (should return error stream)
auto error_stream = proxy->GetSyncStreamReaderWriter(nullptr);
if (!error_stream.GetStatus().OK()) {
    std::cout << "Expected error for null context: " << error_stream.GetStatus().ErrorMessage() << std::endl;
    std::cout << "Error code: " << error_stream.GetStatus().GetFrameworkRetCode() << std::endl;
}

// Test with valid context
auto ctx = trpc::MakeClientContext(proxy);
auto stream = proxy->GetSyncStreamReaderWriter(ctx);

if (stream.GetStatus().OK()) {
    std::cout << "Stream created successfully" << std::endl;
} else {
    std::cerr << "Stream creation failed: " << stream.GetStatus().ErrorMessage() << std::endl;
}
```

## Build Commands

### Build the HTTP SSE Stream Proxy

```bash
# Navigate to the project directory
cd /path/to/trpc-cpp

# Build only the HTTP SSE stream proxy
bazel build //trpc/client/http_sse:http_sse_stream_proxy

# Build all HTTP SSE components
bazel build //trpc/client/http_sse/... //trpc/codec/http_sse/...

# Build the entire project
bazel build //trpc/...
```

### Clean Build

```bash
# Clean previous builds
./clean.sh

# Rebuild
bazel build //trpc/client/http_sse/...
```

## Dependencies

### Required Dependencies
- `//trpc/client:service_proxy` - Base service proxy functionality
- `//trpc/codec/http_sse:http_sse_codec` - SSE codec implementation
- `//trpc/stream/http:http_client_stream` - HTTP client stream support
- `//trpc/common:status` - Status handling
- `//trpc/log:logging` - Logging functionality
- `//trpc/util/http:method` - HTTP method utilities


## Error Codes

The HTTP SSE Stream Proxy uses the following error codes:

| Error Code | Value | Description |
|------------|-------|-------------|
| `ENCODE_ERROR` | 4 | General encoding/configuration error |
| `TRPC_STREAM_CLIENT_NETWORK_ERR` | 301 | Network error during stream operations |
| `TRPC_STREAM_CLIENT_READ_TIMEOUT_ERR` | 354 | Read timeout error |
| `TRPC_STREAM_CLIENT_WRITE_TIMEOUT_ERR` | 334 | Write timeout error |

## Configuration Options

### SSE-Specific Headers
The proxy automatically sets the following headers for SSE connections:

- `Accept: text/event-stream` - Indicates SSE support
- `Cache-Control: no-cache` - Prevents caching of SSE events
- `Connection: keep-alive` - Maintains persistent connection

### Timeout Configuration
- **Default SSE Timeout**: 60 seconds
- **Configurable**: Can be overridden via `ClientContext::SetTimeout()`

### HTTP Method
- **Default**: `GET` (standard for SSE connections)
- **Configurable**: Can be changed via request protocol

## Best Practices

### 1. Error Handling
Always check the stream status after creation:
```cpp
auto stream = proxy->GetSyncStreamReaderWriter(ctx);
if (!stream.GetStatus().OK()) {
    // Handle error appropriately
}
```

### 2. Resource Management
Always close streams when done:
```cpp
stream.Close();  // Clean up resources
```

### 3. Timeout Configuration
Use appropriate timeouts for your use case:
```cpp
ctx->SetTimeout(60000);  // 60 seconds for long-lived connections
```

### 4. Custom Headers
Set custom headers before creating the stream:
```cpp
ctx->SetHttpHeader("Authorization", "Bearer token");
ctx->SetHttpHeader("Custom-Header", "value");
```

## Troubleshooting

### Common Issues

1. **Stream Creation Fails**
   - Check if the client context is valid
   - Verify network connectivity
   - Ensure proper service configuration

2. **Timeout Errors**
   - Increase timeout value for long-lived connections
   - Check network stability
   - Verify server-side SSE implementation

3. **Header Issues**
   - Ensure SSE-specific headers are set correctly
   - Check for conflicting headers
   - Verify server accepts SSE connections

### Debug Information

Enable debug logging to troubleshoot issues:
```cpp
// Set log level to debug
trpc::log::SetLogLevel(trpc::log::kDebug);
```

## Related Components

- **HTTP SSE Codec**: `//trpc/codec/http_sse:http_sse_codec`
- **HTTP Client Stream**: `//trpc/stream/http:http_client_stream`
- **SSE Parser**: `//trpc/util/http/sse:sse_parser`

## License

This component is part of tRPC-Cpp and is licensed under the Apache 2.0 License.
