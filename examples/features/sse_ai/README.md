# SSE AI Chat Example

This example demonstrates how to use Server-Sent Events (SSE) with tRPC-Cpp to build real-time AI chat applications. It showcases streaming AI responses in real-time, simulating modern AI chatbot interactions.

## Overview

The SSE AI Chat example consists of:
- **Server**: Implements an AI chat service that streams responses using SSE
- **Client**: Connects to the SSE stream and displays AI responses in real-time
- **Real-time Streaming**: Demonstrates how AI responses can be streamed chunk by chunk
- **Multiple Question Types**: Shows different AI response patterns based on question content

## Features

### Server Features
- 🤖 **AI Response Simulation**: Generates realistic AI responses with streaming
- 📡 **SSE Protocol**: Full Server-Sent Events implementation
- 🎯 **Content-Aware Responses**: Different response patterns for weather, code, and tRPC questions
- 🔥 **High Performance**: Built on tRPC-Cpp's fiber runtime
- 📊 **Health Monitoring**: Built-in health check endpoint

### Client Features
- 🚀 **Real-time Display**: Shows AI responses as they stream
- 🔄 **Multiple Questions**: Tests various question types automatically
- 📱 **User-Friendly Output**: Clean, formatted display of streaming responses
- ⚡ **Efficient Parsing**: Proper SSE event parsing and handling

## Directory Structure

```
examples/features/sse_ai/
├── client/
│   ├── BUILD                    # Bazel build file for client
│   ├── client.cc               # SSE AI client implementation
│   └── trpc_cpp_fiber.yaml     # Client configuration
├── server/
│   ├── BUILD                    # Bazel build file for server
│   ├── sse_ai_server.cc        # SSE AI server implementation
│   └── trpc_cpp_fiber.yaml     # Server configuration
├── CMakeLists.txt              # CMake build configuration
├── README.md                   # This documentation
└── run.sh                      # Build and run script
```

## Quick Start

### Using the Run Script (Recommended)

The easiest way to build and run the example. The script can be run from either the workspace root or the example directory:

**From workspace root:**
```bash
cd trpc-cpp-Draft
examples/features/sse_ai/run.sh
```

**From example directory:**
```bash
cd examples/features/sse_ai/
chmod +x run.sh
./run.sh
```

**Note**: If you get a Bazel error about paths with spaces, use the CMake alternative:

```bash
chmod +x run_cmake.sh
./run_cmake.sh
```

This script will:
1. Detect the correct binary paths automatically
2. Build both server and client using Bazel
3. Start the server
4. Run multiple client test scenarios
5. Display the results and cleanup

### Manual Build and Run

#### 1. Build with Bazel

```bash
# Build server
bazel build //examples/features/sse_ai/server:sse_ai_server

# Build client
bazel build //examples/features/sse_ai/client:client
```

#### 2. Build with CMake (Alternative)

```bash
# Build trpc-cpp first (if not already built)
mkdir -p build && cd build && cmake -DCMAKE_BUILD_TYPE=Release .. && make -j8 && cd -

# Build SSE AI example
mkdir -p examples/features/sse_ai/build 
cd examples/features/sse_ai/build 
cmake -DCMAKE_BUILD_TYPE=Release .. 
make -j8 
cd -
```

#### 3. Run the Server

```bash
# Using Bazel build
bazel-bin/examples/features/sse_ai/server/sse_ai_server --config=examples/features/sse_ai/server/trpc_cpp_fiber.yaml

# Or using CMake build
examples/features/sse_ai/build/sse_ai_server --config=examples/features/sse_ai/server/trpc_cpp_fiber.yaml
```

#### 4. Run the Client

```bash
# Using Bazel build
bazel-bin/examples/features/sse_ai/client/client --client_config=examples/features/sse_ai/client/trpc_cpp_fiber.yaml

# With custom question
bazel-bin/examples/features/sse_ai/client/client --client_config=examples/features/sse_ai/client/trpc_cpp_fiber.yaml --question="How does tRPC work?"
```

## Usage Examples

### Basic AI Chat

```bash
# Ask a general question
./client --question="Hello, how can you help me?"
```

Expected output:
```
=== SSE AI Chat Demo ===
Question: Hello, how can you help me?
Connecting to AI chat server...

AI Response:
--------------------------------------------------
AI: Thank you for your question: "Hello, how can you help me?"

I'm an AI assistant powered by tRPC-Cpp's Server-Sent Events implementation. This streaming response demonstrates how real-time AI interactions can be built using the tRPC framework. Each chunk you see is being streamed individually, simulating how modern AI chat systems work. You can ask me about weather, programming, or tRPC-Cpp itself for more specialized responses!

--------------------------------------------------

✅ SSE AI Chat completed successfully!
```

### Weather Query

```bash
./client --question="What's the weather like today?"
```

Expected output:
```
AI: Looking up current weather information...Based on the latest data, today's weather is partly cloudy with a temperature of 22°C (72°F). There's a light breeze from the southwest and the humidity is around 65%. Perfect weather for outdoor activities!
```

### Programming Help

```bash
./client --question="Show me some C++ code"
```

Expected output:
```
AI: Great question about programming! Here's a simple example to get you started:

```cpp
#include <iostream>
int main() {
    std::cout << "Hello, World!" << std::endl;
    return 0;
}
```

This basic C++ program demonstrates the fundamental structure of a C++ application. Would you like me to explain any specific part?
```

### tRPC Information

```bash
./client --question="Tell me about tRPC"
```

Expected output:
```
AI: tRPC-Cpp is a high-performance RPC framework! Here are some key features:

🚀 High Performance: Built for speed and efficiency
🔧 Multiple Protocols: Supports HTTP, gRPC, and custom protocols
🌐 Server-Sent Events: Real-time streaming like this example
⚙️ Flexible Configuration: Easy to configure and extend
🔗 Service Mesh Ready: Built for modern microservices

This SSE example demonstrates how tRPC-Cpp can handle real-time streaming for AI applications!
```

## Testing with curl

You can also test the SSE endpoint directly with curl:

```bash
# Basic SSE request
curl -N -H "Accept: text/event-stream" "http://127.0.0.1:24857/ai/chat?question=Hello"

# Health check
curl http://127.0.0.1:24857/health
```

Expected SSE output format:
```
event: ai_start
data: Starting AI response for: Hello
id: start_1

event: ai_chunk
data: Thank you for your question: "Hello"
id: chunk_0

event: ai_chunk
data: I'm an AI assistant powered by tRPC-Cpp's
id: chunk_1

event: ai_complete
data: Response completed
id: end_1
```

## API Endpoints

### `/ai/chat` - SSE AI Chat Endpoint

- **Method**: GET or POST
- **Headers**: 
  - `Accept: text/event-stream` (required)
  - `Cache-Control: no-cache` (recommended)
- **Parameters**:
  - `question` (query parameter): The question to ask the AI
- **Response**: SSE stream with AI response chunks

### `/health` - Health Check Endpoint

- **Method**: GET
- **Response**: JSON health status

## SSE Event Types

The server sends the following SSE event types:

| Event Type | Description | Data Content |
|------------|-------------|--------------|
| `ai_start` | Indicates start of AI processing | Processing start message |
| `ai_chunk` | Contains a chunk of AI response | Response text chunk |
| `ai_complete` | Indicates completion of response | Completion message |
| `ai_error` | Indicates an error occurred | Error description |

## Configuration

### Server Configuration

Key settings in `server/trpc_cpp_fiber.yaml`:
- **Port**: 24857 (HTTP SSE service)
- **Protocol**: HTTP with SSE support
- **Thread Model**: Fiber-based for high concurrency

### Client Configuration

Key settings in `client/trpc_cpp_fiber.yaml`:
- **Target**: 127.0.0.1:24857 (server address)
- **Timeout**: 30 seconds for SSE streams
- **Connection**: Long connection for persistent streams

## Implementation Details

### Server Implementation

The server uses `HttpSseService` to manage SSE connections:

1. **Connection Management**: Accepts and registers SSE connections
2. **AI Response Generation**: Simulates AI processing with realistic delays
3. **Streaming**: Sends response chunks as SSE events
4. **Error Handling**: Graceful error handling and connection cleanup

### Client Implementation

The client implements SSE stream consumption:

1. **SSE Request**: Sends HTTP request with SSE headers
2. **Stream Processing**: Parses incoming SSE events
3. **Real-time Display**: Shows AI responses as they arrive
4. **Multiple Tests**: Runs various test scenarios

### Key Components

- **`HttpSseService`**: Server-side SSE connection management
- **`SseEvent`**: SSE event structure for data exchange
- **`SseParser`**: Client-side SSE event parsing
- **Fiber Runtime**: High-performance async execution

## Troubleshooting

### Common Issues

1. **Bazel path space issue**
   - **Error**: `bazel does not currently work properly from paths containing spaces`
   - **Solution**: Move the project to a path without spaces, e.g.:
     ```bash
     # Move to a path without spaces
     mv "/home/user/path with spaces/trpc-cpp-Draft" "/home/user/trpc-cpp-Draft"
     cd /home/user/trpc-cpp-Draft/examples/features/sse_ai
     ./run.sh
     ```
   - **Alternative**: Use CMake build instead of Bazel if path cannot be changed

2. **Compilation errors**
   - **Error**: `'StartRuntime' is not a member of 'trpc'` or `'UrlEncode' is not a member of 'trpc::http'`
   - **Solution**: These errors indicate incorrect API usage. The example has been updated to use:
     - `::trpc::RunInTrpcRuntime()` instead of manual runtime management
     - `::trpc::http::PercentEncode()` instead of `UrlEncode()`
   - **Fix**: Ensure you're using the latest version of the example code

3. **Binary not found errors**
   - **Error**: `bazel-bin/examples/features/sse_ai/server/sse_ai_server: No such file or directory`
   - **Cause**: Script is looking for binaries in the wrong location
   - **Solution**: The updated run.sh script automatically detects binary paths. Ensure you:
     - Run the script from the workspace root OR the example directory
     - Have completed the build step successfully
     - Check that `bazel-bin` symlink exists in workspace root

4. **Configuration file errors**
   - **Error**: `init config error: bad file: examples/features/sse_ai/server/trpc_cpp_fiber.yaml`
   - **Cause**: Configuration file path is relative to execution location
   - **Solution**: The updated run.sh script automatically detects config file paths. Ensure:
     - Configuration files exist in the expected locations
     - You're running from the correct directory context
     - File permissions allow reading the config files

5. **Server fails to start**
   - Check if port 24857 is available
   - Verify configuration file paths
   - Check log files for error details

2. **Client connection fails**
   - Ensure server is running
   - Verify network connectivity
   - Check firewall settings

3. **SSE stream not working**
   - Verify `Accept: text/event-stream` header
   - Check server SSE implementation
   - Monitor server logs for errors

### Debug Tips

1. **Enable verbose logging**:
   - Set `min_level: 0` in configuration files
   
2. **Check server health**:
   ```bash
   curl http://127.0.0.1:24857/health
   ```

3. **Monitor server logs**:
   ```bash
   tail -f sse_ai_server.log
   ```

## Performance Considerations

- **Fiber Runtime**: Uses tRPC-Cpp's high-performance fiber runtime
- **Connection Pooling**: Efficient SSE connection management
- **Memory Management**: Proper cleanup of SSE streams
- **Streaming Optimization**: Chunked response delivery for better UX

## Related Documentation

- [tRPC-Cpp SSE Utilities](../../../trpc/util/http/sse/README.md)
- [tRPC-Cpp HTTP SSE Codec](../../../trpc/codec/http_sse/README.md)
- [tRPC-Cpp Server-Side SSE](../../../trpc/server/http_sse/README.md)
- [tRPC-Cpp Client-Side SSE](../../../trpc/client/sse/README.md)
- [W3C Server-Sent Events Specification](https://developer.mozilla.org/en-US/docs/Web/API/Server-sent_events/Using_server-sent_events)

## License

This example is part of tRPC-Cpp and is licensed under the Apache 2.0 License.