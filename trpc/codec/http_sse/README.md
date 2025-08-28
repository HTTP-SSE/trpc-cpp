# HTTP SSE Codec

This module provides HTTP Server-Sent Events (SSE) codec implementation for tRPC-C++.

## Overview

The HTTP SSE codec extends the base HTTP codec to handle Server-Sent Events, which is a web standard for real-time streaming data from server to client.

## Components

### 1. Protocol Classes

- `HttpSseRequestProtocol`: Extends `HttpRequestProtocol` with SSE-specific functionality
- `HttpSseResponseProtocol`: Extends `HttpResponseProtocol` with SSE-specific functionality

### 2. Codec Classes

- `HttpSseClientCodec`: Client-side codec for encoding SSE requests and decoding SSE responses
- `HttpSseServerCodec`: Server-side codec for decoding SSE requests and encoding SSE responses

### 3. Protocol Checker

- `HttpSseZeroCopyCheckRequest`: Validates and parses SSE requests
- `HttpSseZeroCopyCheckResponse`: Validates and parses SSE responses

## Key Features

1. **SSE Event Parsing**: Automatically parses SSE events from request/response bodies
2. **SSE Event Serialization**: Converts SSE events to proper SSE format
3. **Header Management**: Sets appropriate SSE headers (Content-Type, Cache-Control, etc.)
4. **Validation**: Validates SSE requests and responses
5. **Streaming Support**: Handles the streaming nature of SSE

## Usage

### Client Side

```cpp
// Create SSE client codec
auto codec = std::make_shared<HttpSseClientCodec>();

// Create SSE request
auto request = std::make_shared<HttpSseRequestProtocol>();

// Set SSE event
http::sse::SseEvent event;
event.event_type = "message";
event.data = "Hello World";
event.id = "123";
request->SetSseEvent(event);

// Encode request
NoncontiguousBuffer buffer;
codec->ZeroCopyEncode(ctx, request, buffer);
```

### Server Side

```cpp
// Create SSE server codec
auto codec = std::make_shared<HttpSseServerCodec>();

// Create SSE response
auto response = std::make_shared<HttpSseResponseProtocol>();

// Set SSE event
http::sse::SseEvent event;
event.event_type = "notification";
event.data = "User logged in";
response->SetSseEvent(event);

// Encode response
NoncontiguousBuffer buffer;
codec->ZeroCopyEncode(ctx, response, buffer);
```

## SSE Event Format

SSE events follow the standard format:

```
event: <event_type>
data: <event_data>
id: <event_id>
retry: <retry_timeout>
```

## Headers

The codec automatically sets the following headers:

- `Content-Type: text/event-stream`
- `Cache-Control: no-cache`
- `Connection: keep-alive`
- `Accept: text/event-stream` (for requests)

## Validation

The codec validates:

1. **Requests**: Must be GET requests with `Accept: text/event-stream`
2. **Responses**: Must have `Content-Type: text/event-stream` and `Cache-Control: no-cache`

## Error Handling

- Invalid SSE format is logged as warnings but doesn't fail the request
- Parsing errors are logged and handled gracefully
- Connection errors are properly propagated

## Dependencies

- `trpc/codec/http`: Base HTTP codec functionality
- `trpc/util/http/sse`: SSE parsing and event handling
- `trpc/util/buffer`: Buffer management
- `trpc/common`: Common utilities
- `trpc/log`: Logging functionality
