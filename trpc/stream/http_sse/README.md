# HTTP SSE Stream Module

This directory contains the HTTP Server-Sent Events (SSE) stream implementation for tRPC-Cpp, providing streaming capabilities for SSE over HTTP.

## Overview

The HTTP SSE stream module extends the existing HTTP stream infrastructure to provide SSE-specific streaming support:

- **SSE Stream Support**: Full support for SSE streaming over HTTP
- **Fiber Coroutine Mode**: Optimized for tRPC-Cpp's fiber coroutine mode
- **Stream Management**: Comprehensive stream lifecycle management
- **Event Handling**: Rich SSE event handling capabilities
- **Performance Optimized**: Zero-copy design and efficient memory management

## Components

### 1. HttpSseStream

The core SSE stream class that handles individual SSE connections.

#### Header File
```cpp
#include "trpc/stream/http_sse/http_sse_stream.h"
```

#### Key Features
- **Event Sending**: Send individual or multiple SSE events
- **Comment Support**: Send comment lines for debugging
- **Retry Instructions**: Send retry timeout instructions
- **Auto ID Generation**: Automatic event ID generation
- **Custom Event Types**: Configurable default event types

#### Usage Example
```cpp
#include "trpc/stream/http_sse/http_sse_stream.h"

using namespace trpc::stream::http_sse;

// Create stream options
StreamOptions options;
options.stream_id = 1;
options.connection_id = 1;
options.fiber_mode = true;

// Create SSE stream
auto stream = std::make_unique<HttpSseStream>(std::move(options));

// Initialize stream
stream->Init();

// Send SSE event
http::sse::SseEvent event;
event.event_type = "message";
event.data = "Hello World";
event.id = "123";

auto status = stream->SendEvent(event);
if (status.OK()) {
    std::cout << "Event sent successfully" << std::endl;
}

// Send multiple events
std::vector<http::sse::SseEvent> events = {
    {"update", "Status changed", "1"},
    {"notification", "User logged in", "2"}
};

status = stream->SendEvents(events);

// Send comment
stream->SendComment("Debug information");

// Send retry instruction
stream->SendRetry(5000);

// Close stream
stream->Close(Status::OK());
stream->Join();
```

### 2. HttpSseStreamHandler

Manages multiple SSE streams and handles stream lifecycle.

#### Header File
```cpp
#include "trpc/stream/http_sse/http_sse_stream_handler.h"
```

#### Key Features
- **Stream Management**: Create, retrieve, and remove streams
- **Concurrent Support**: Handle multiple concurrent SSE streams
- **Auto Cleanup**: Automatic cleanup of inactive streams
- **Resource Limits**: Configurable maximum stream limits
- **Thread Safety**: Fiber-aware synchronization

#### Usage Example
```cpp
#include "trpc/stream/http_sse/http_sse_stream_handler.h"

using namespace trpc::stream::http_sse;

// Create stream options
StreamOptions options;
options.connection_id = 1;
options.fiber_mode = true;
options.max_streams = 100;

// Create stream handler
auto handler = std::make_unique<HttpSseStreamHandler>(std::move(options));

// Initialize handler
handler->Init();

// Create new stream
auto stream = handler->CreateStream(1);
if (stream) {
    stream->Init();
    
    // Send events through the stream
    http::sse::SseEvent event{"message", "Hello from stream 1"};
    stream->SendEvent(event);
}

// Get existing stream
auto existing_stream = handler->GetStream(1);

// Get stream count
size_t count = handler->GetStreamCount();

// Get all streams
auto all_streams = handler->GetAllStreams();

// Remove stream
handler->RemoveStream(1);

// Stop and cleanup
handler->Stop();
handler->Join();
```

### 3. HttpSseStreamHandlerFactory

Factory for creating SSE stream handlers with default configurations.

#### Header File
```cpp
#include "trpc/stream/http_sse/http_sse_stream_handler_factory.h"
```

#### Key Features
- **Default Configuration**: Pre-configured default settings
- **Protocol Support**: Support for multiple SSE protocols
- **Configurable**: Runtime configuration updates
- **Factory Pattern**: Standard factory interface

#### Usage Example
```cpp
#include "trpc/stream/http_sse/http_sse_stream_handler_factory.h"

using namespace trpc::stream::http_sse;

// Create factory
auto factory = std::make_unique<HttpSseStreamHandlerFactory>();

// Set default configuration
HttpSseStreamHandlerFactory::SetDefaultConfig(500, true, true);

// Get default options
auto options = HttpSseStreamHandlerFactory::GetDefaultOptions();

// Create handler with options
auto handler = factory->CreateHandler(std::move(options));

// Check protocol support
bool supports_sse = factory->SupportsProtocol("http_sse");
bool supports_text_event_stream = factory->SupportsProtocol("text/event-stream");
```

## Configuration

### Stream Options

```cpp
struct StreamOptions {
    uint32_t stream_id = 0;           // Unique stream identifier
    uint32_t connection_id = 0;       // Connection identifier
    bool fiber_mode = true;           // Use fiber coroutine mode
    size_t max_streams = 1000;        // Maximum concurrent streams
    bool auto_cleanup = true;         // Enable automatic cleanup
};
```

### Handler Configuration

```cpp
// Set maximum streams
handler->SetMaxStreams(500);

// Enable/disable auto cleanup
handler->SetAutoCleanup(true);

// Update stream options
StreamOptions new_options;
new_options.max_streams = 200;
new_options.fiber_mode = false;
handler->SetStreamOptions(new_options);
```

## Building

### Prerequisites
- Bazel build system
- C++17 or later
- tRPC-Cpp framework
- SSE utilities (`//trpc/util/http/sse:http_sse`)

### Build Commands

#### Build the SSE stream library
```bash
bazel build //trpc/stream/http_sse:http_sse_stream
```

#### Build and run tests
```bash
# Build tests
bazel build //trpc/stream/http_sse/test/...

# Run all SSE stream tests
bazel test //trpc/stream/http_sse/test/...

# Run specific test
bazel test //trpc/stream/http_sse/test:http_sse_stream_test
bazel test //trpc/stream/http_sse/test:http_sse_stream_handler_test
```

#### Build with different configurations
```bash
# Build with debug symbols
bazel build -c dbg //trpc/stream/http_sse:http_sse_stream

# Build with optimizations
bazel build -c opt //trpc/stream/http_sse:http_sse_stream
```

### Dependencies

The HTTP SSE stream module depends on:
- `//trpc/stream/http:http_stream` - Base HTTP stream implementation
- `//trpc/stream/http:http_stream_handler` - Base HTTP stream handler
- `//trpc/stream:stream_handler_factory` - Stream handler factory interface
- `//trpc/util/http/sse:http_sse` - SSE utilities
- `//trpc/util/http/sse:http_sse_parser` - SSE parsing utilities
- `//trpc/coroutine:fiber` - Fiber coroutine support

## Integration

To use the HTTP SSE stream module in your project:

```cpp
// In your BUILD file
cc_library(
    name = "my_sse_service",
    srcs = ["my_sse_service.cc"],
    deps = [
        "//trpc/stream/http_sse:http_sse_stream",
        "//trpc/util/http/sse:http_sse",
        # ... other dependencies
    ],
)
```

## Performance Considerations

### Memory Management
- **Zero-copy Design**: Minimizes data copying for better performance
- **Smart Pointers**: Automatic memory management with RAII
- **Buffer Reuse**: Efficient buffer management for streaming data

### Concurrency
- **Fiber Mode**: Optimized for tRPC-Cpp's fiber coroutine system
- **Thread Safety**: Safe concurrent access to stream handlers
- **Resource Limits**: Configurable limits prevent resource exhaustion

### Streaming Optimization
- **Batch Operations**: Support for sending multiple events at once
- **Lazy Initialization**: Streams are initialized only when needed
- **Auto Cleanup**: Automatic cleanup of inactive streams

## Error Handling

### Status Codes
- **Success**: `Status::OK()` for successful operations
- **Stream Not Ready**: `Status(-1, "Stream is not ready")` when stream is not initialized
- **Send Failure**: `Status(-1, "Failed to send SSE data")` for send failures

### Exception Safety
- **RAII**: Automatic cleanup on exceptions
- **Graceful Degradation**: Fallback behavior for error conditions
- **Logging**: Comprehensive error logging for debugging

## Testing

The test suite provides comprehensive coverage for:
- ✅ **Stream Operations**: Event sending, comment sending, retry instructions
- ✅ **Stream Management**: Creation, retrieval, removal of streams
- ✅ **Handler Operations**: Stream handler lifecycle and management
- ✅ **Configuration**: Options and settings management
- ✅ **Error Handling**: Error conditions and recovery
- ✅ **Concurrency**: Multi-stream concurrent operations

Run tests with:
```bash
# Run all SSE stream tests
bazel test //trpc/stream/http_sse/test/... --test_output=all

# Run specific test with detailed output
bazel test //trpc/stream/http_sse/test:http_sse_stream_test --test_output=all
```

## Notes

- **Namespace**: All components are in `trpc::stream::http_sse` namespace
- **Thread Safety**: All methods are thread-safe and fiber-aware
- **Error Handling**: Graceful handling of errors with comprehensive logging
- **Performance**: Optimized for streaming scenarios with zero-copy design
- **Compatibility**: Fully compatible with existing HTTP stream infrastructure
- **Fiber Mode**: Designed to work seamlessly with tRPC-Cpp's fiber coroutine system

## Related Documentation

- [SSE Utilities README](../../util/http/sse/README.md) - Core SSE utilities
- [HTTP Stream Documentation](../http/README.md) - Base HTTP stream documentation
- [Stream Framework Documentation](../README.md) - Stream framework overview
- [tRPC-Cpp Framework](https://github.com/trpc-group/trpc-cpp) - Main framework documentation
- [W3C SSE Specification](https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events) - SSE standard reference
