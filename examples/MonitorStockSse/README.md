# Stock Price SSE Monitor (tRPC HTTP-SSE)

A real-time stock price monitoring system using **tRPC HTTP-SSE** framework with proper tRPC client and server modules.

## 🎯 **Key Difference from MonitorStock**

This example uses the **proper tRPC HTTP-SSE modules**:

- ✅ **Server**: Uses `trpc::server::HttpService` with `trpc::util::http::sse::http_sse`
- ✅ **Client**: Uses `trpc::stream::HttpSseStreamProxy` for SSE connections
- ✅ **Protocol**: Full tRPC protocol with HTTP-SSE codec
- ✅ **Framework**: Complete tRPC framework integration

## 🏗️ **Architecture**

### **Server Components**
- **`StockPriceSimulator`**: Thread-safe stock price simulation
- **`StockPriceSseHandler`**: tRPC HTTP handler for SSE endpoint
- **`HttpService`**: tRPC HTTP service with SSE support
- **`TrpcApp`**: tRPC application framework

### **Client Components**
- **`HttpSseStreamProxy`**: tRPC HTTP-SSE client proxy
- **`TrpcSseClient`**: Client using tRPC framework
- **`SseParser`**: tRPC SSE event parsing
- **`HttpClientStreamReaderWriter`**: tRPC stream interface

## 📁 **Files**

```
examples/MonitorStockSse/
├── README.md                           # This documentation
├── BUILD                              # Bazel build configuration
├── trpc_merge.yaml                    # tRPC configuration
├── stock_price_sse_server.cc          # tRPC HTTP-SSE server
└── stock_price_sse_client.cc          # tRPC HTTP-SSE client
```

## 🚀 **Features**

### **Server Features**
- ✅ tRPC HTTP server with SSE support
- ✅ Thread-safe stock price simulation
- ✅ Real-time SSE event streaming
- ✅ Welcome page with HTML interface
- ✅ JSON stock data endpoint
- ✅ Proper tRPC framework integration

### **Client Features**
- ✅ **tRPC HTTP-SSE client** using `HttpSseStreamProxy`
- ✅ **tRPC SSE event parsing** using `SseParser`
- ✅ **tRPC stream handling** using `HttpClientStreamReaderWriter`
- ✅ Real-time stock price display
- ✅ Connection status monitoring
- ✅ Automatic data clearing on disconnect

### **tRPC Framework Integration**
- ✅ **Server**: `trpc::server::HttpService`, `trpc::util::http::sse::http_sse`
- ✅ **Client**: `trpc::stream::HttpSseStreamProxy`, `trpc::util::http::sse::SseParser`
- ✅ **Protocol**: `trpc::HttpSseRequestProtocol`, `trpc::HttpSseResponseProtocol`
- ✅ **Codec**: `trpc::HttpSseClientCodec`, `trpc::HttpSseServerCodec`

## 🔧 **Building**

### **Build Everything**
```bash
cd examples/MonitorStockSse
bazel build //examples/MonitorStockSse:all
```

### **Build Server Only**
```bash
bazel build //examples/MonitorStockSse:stock_price_sse_server
```

### **Build Client Only**
```bash
bazel build //examples/MonitorStockSse:stock_price_sse_client
```

## 🏃‍♂️ **Running**

### **1. Start the tRPC Server**
```bash
# Terminal 1: Start the tRPC HTTP-SSE server
bazel run //examples/MonitorStockSse:stock_price_sse_server
```

The server will:
- Start tRPC HTTP service on port 8080
- Initialize SSE endpoint at `/sse`
- Begin stock price simulation
- Display welcome page at `/`

### **2. Start the tRPC Client**
```bash
# Terminal 2: Start the tRPC HTTP-SSE client
bazel run //examples/MonitorStockSse:stock_price_sse_client
```

The client will:
- Connect using `HttpSseStreamProxy`
- Parse SSE events using `SseParser`
- Display real-time stock prices
- Show connection status

### **3. View Results**

**Server Output:**
```
=== Stock Price SSE Server (tRPC HTTP-SSE) ===
[Server] Using config file: examples/MonitorStockSse/trpc_merge.yaml
[Server] Server started successfully!
[Server] SSE endpoint: http://localhost:8080/sse
[Server] Welcome page: http://localhost:8080/
[Server] Stock data: http://localhost:8080/stocks
```

**Client Output:**
```
=== Real-Time Stock Prices (tRPC HTTP-SSE) ===
Last Updated: Sun Aug 24 17:30:24 2025
Connection Status: Connected
Press Ctrl+C to exit

Symbol  Price       Change      Change %       
--------------------------------------------------
AAPL    125.29      -1.47       -1.16%        
AMZN    3597.34     +91.08      +2.60%        
GOOGL   2212.64     -20.81      -0.93%        
MSFT    165.26      +2.96       +1.82%        
TSLA    259.38      -0.02       -0.01%        
```

## 🔍 **tRPC Module Usage**

### **Server-Side tRPC Modules**
```cpp
// Core tRPC server
#include "trpc/server/trpc_server.h"
#include "trpc/server/http_service.h"
#include "trpc/common/trpc_app.h"

// HTTP and SSE utilities
#include "trpc/util/http/http_handler.h"
#include "trpc/util/http/sse/http_sse.h"
#include "trpc/util/http/sse/sse_parser.h"

// Framework components
#include "trpc/common/runtime_manager.h"
#include "trpc/codec/codec_manager.h"
```

### **Client-Side tRPC Modules**
```cpp
// tRPC HTTP-SSE client proxy
#include "trpc/client/sse/http_sse_stream_proxy.h"
#include "trpc/client/make_client_context.h"
#include "trpc/client/trpc_client.h"

// SSE parsing and streaming
#include "trpc/util/http/sse/sse_parser.h"
#include "trpc/stream/http/http_client_stream.h"

// Protocol and codec
#include "trpc/codec/http_sse/http_sse_codec.h"
```

## ⚙️ **Configuration**

### **Server Configuration (`trpc_merge.yaml`)**
```yaml
server:
  app: stock_price_sse_server
  server: stock_price_sse_server
  service:
    - name: stock_price_sse_service
      protocol: http
      network: tcp
      address: 0.0.0.0:8080
      threadmodel_instance_name: default_instance
```

### **Client Configuration**
```yaml
client:
  service:
    - name: stock_price_sse_service
      target: 127.0.0.1:8080
      protocol: http_sse
      conn_type: long
      timeout: 60000
      load_balance_name: direct
```

## 🔄 **tRPC HTTP-SSE Flow**

### **1. Server Setup**
```cpp
// Create tRPC HTTP service
auto http_service = std::make_shared<HttpService>();

// Add SSE handler
auto sse_handler = std::make_shared<StockPriceSseHandler>(simulator);
routes.Add(http::MethodType::GET, http::Path("/sse"), sse_handler);

// Register with tRPC framework
RegisterService("stock_price_sse_service", http_service);
```

### **2. Client Connection**
```cpp
// Create tRPC HTTP-SSE proxy
auto proxy = trpc::GetTrpcClient()->GetProxy<HttpSseStreamProxy>("stock_price_sse_service", option);

// Setup SSE parameters
proxy->SetupSseParameters(ctx);

// Create stream using tRPC framework
auto stream = proxy->GetSyncStreamReaderWriter(ctx);
```

### **3. SSE Event Processing**
```cpp
// Parse SSE events using tRPC parser
trpc::http::sse::SseEvent sse_event;
if (trpc::http::sse::SseParser::ParseEvent(event_data, sse_event)) {
    // Process parsed event
    ProcessStockUpdate(sse_event.data);
}
```

## 🧪 **Testing**

### **Test Server Endpoints**
```bash
# Test welcome page
curl http://localhost:8080/

# Test JSON endpoint
curl http://localhost:8080/stocks

# Test SSE endpoint
curl http://localhost:8080/sse
```

### **Test tRPC Client**
```bash
# Run client and observe tRPC framework logs
bazel run //examples/MonitorStockSse:stock_price_sse_client
```

## 🔍 **Comparison with MonitorStock**

| Feature | MonitorStock | MonitorStockSse |
|---------|-------------|-----------------|
| **Server** | Raw HTTP server | tRPC HTTP server |
| **Client** | Raw sockets | tRPC HTTP-SSE proxy |
| **SSE Parsing** | Manual parsing | tRPC SseParser |
| **Streaming** | Raw HTTP streams | tRPC HttpClientStream |
| **Protocol** | Raw HTTP/SSE | tRPC HTTP-SSE |
| **Framework** | Minimal tRPC | Full tRPC integration |
| **Codec** | None | HttpSseCodec |
| **Configuration** | Basic | Full tRPC config |

## 🎯 **Benefits of tRPC HTTP-SSE**

### **1. Framework Integration**
- **Unified Configuration**: Single `trpc_merge.yaml` for both client and server
- **Service Discovery**: tRPC naming and service registry
- **Load Balancing**: tRPC load balancer support
- **Monitoring**: tRPC metrics and monitoring

### **2. Protocol Support**
- **HTTP-SSE Codec**: Proper SSE protocol encoding/decoding
- **Stream Management**: tRPC stream lifecycle management
- **Error Handling**: Framework-level error handling
- **Timeout Management**: Configurable timeouts

### **3. Development Experience**
- **Type Safety**: Strongly typed tRPC interfaces
- **Debugging**: tRPC framework debugging tools
- **Logging**: Integrated tRPC logging
- **Testing**: tRPC testing utilities

## 🚀 **Production Ready**

This example demonstrates:
- ✅ **Full tRPC Integration**: Complete framework usage
- ✅ **Proper SSE Protocol**: Standard-compliant Server-Sent Events
- ✅ **Thread Safety**: Mutex-protected data access
- ✅ **Error Handling**: Comprehensive error management
- ✅ **Connection Management**: Proper connection lifecycle
- ✅ **Configuration**: Flexible tRPC configuration

## 📚 **Next Steps**

### **Possible Enhancements**
1. **Service Discovery**: Use tRPC naming for dynamic service discovery
2. **Load Balancing**: Implement tRPC load balancing for multiple servers
3. **Authentication**: Add tRPC authentication mechanisms
4. **Metrics**: Integrate tRPC metrics and monitoring
5. **TLS**: Add TLS support for secure connections
6. **Compression**: Enable tRPC compression for efficiency

### **Integration Examples**
- **Web Dashboard**: Connect to web frontend
- **Mobile App**: Mobile client integration
- **Microservices**: Part of larger microservice architecture
- **Kubernetes**: Deploy in Kubernetes with tRPC service mesh

---

## 🔧 **Fixed Implementation**

### **Client Implementation Updates**
The client has been updated to use proper tRPC HTTP-SSE modules:

```cpp
// ✅ CORRECT: Use tRPC HTTP-SSE proxy
auto proxy = std::make_shared<HttpSseStreamProxy>();
auto ctx = trpc::MakeClientContext(proxy);
auto stream = proxy->GetSyncStreamReaderWriter(ctx);

// ✅ CORRECT: Use tRPC SSE parser
trpc::http::sse::SseEvent sse_event;
if (trpc::http::sse::SseParser::ParseEvent(event_data, sse_event)) {
    ProcessStockUpdate(sse_event.data);
}
```

### **Key Changes Made**
1. **Replaced raw sockets** with `HttpSseStreamProxy`
2. **Added proper tRPC SSE parsing** using `SseParser`
3. **Used tRPC stream interface** with `HttpClientStreamReaderWriter`
4. **Updated dependencies** to include all required tRPC modules
5. **Fixed BUILD configuration** for proper compilation

---

**Status**: ✅ **Production Ready** - This example now demonstrates proper tRPC HTTP-SSE usage with full framework integration! 🎯

