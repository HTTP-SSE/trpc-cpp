# HTTP SSE Codec Test Suite

This directory contains comprehensive tests for the HTTP SSE codec implementation.

## Test Files

### 1. `http_sse_codec_test.cc`
Main test file covering the core HTTP SSE codec functionality.

**Test Categories:**
- **Protocol Classes**: Tests for `HttpSseRequestProtocol` and `HttpSseResponseProtocol`
- **Client Codec**: Tests for `HttpSseClientCodec` functionality
- **Server Codec**: Tests for `HttpSseServerCodec` functionality
- **Integration Tests**: End-to-end client-server integration tests
- **Edge Cases**: Tests for edge cases and error handling

### 2. `http_sse_proto_checker_test.cc`
Test file specifically for the SSE protocol checker functionality.

**Test Categories:**
- **Request Validation**: Tests for `IsValidSseRequest` function
- **Response Validation**: Tests for `IsValidSseResponse` function
- **Protocol Checking**: Tests for `HttpSseZeroCopyCheckRequest` and `HttpSseZeroCopyCheckResponse`
- **Edge Cases**: Tests for various edge cases in protocol validation

## Running Tests

### Quick Start
```bash
# From the trpc-cpp root directory
./trpc/codec/http_sse/test/run_tests.sh
```

### Manual Testing
```bash
# Build tests
bazel build //trpc/codec/http_sse:http_sse_codec_test
bazel build //trpc/codec/http_sse:http_sse_proto_checker_test

# Run main codec tests
bazel test //trpc/codec/http_sse:http_sse_codec_test --test_output=all

# Run protocol checker tests
bazel test //trpc/codec/http_sse:http_sse_proto_checker_test --test_output=all
```

### Individual Test Execution
```bash
# Run specific test
bazel test //trpc/codec/http_sse:http_sse_codec_test --test_filter=HttpSseCodecTest.HttpSseRequestProtocol_GetSseEvent

# Run tests matching a pattern
bazel test //trpc/codec/http_sse:http_sse_codec_test --test_filter=*FillRequest*
```

## Test Coverage

### Protocol Classes Tests
- ✅ **HttpSseRequestProtocol_GetSseEvent**: Tests parsing SSE events from requests
- ✅ **HttpSseRequestProtocol_SetSseEvent**: Tests setting SSE events in requests
- ✅ **HttpSseRequestProtocol_InvalidSseData**: Tests handling of invalid SSE data
- ✅ **HttpSseResponseProtocol_GetSseEvent**: Tests parsing SSE events from responses
- ✅ **HttpSseResponseProtocol_SetSseEvent**: Tests setting SSE events in responses
- ✅ **HttpSseResponseProtocol_SetSseEvents**: Tests setting multiple SSE events
- ✅ **HttpSseResponseProtocol_EmptyEvents**: Tests handling empty event lists

### Client Codec Tests
- ✅ **HttpSseClientCodec_CreateRequestPtr**: Tests request protocol creation
- ✅ **HttpSseClientCodec_CreateResponsePtr**: Tests response protocol creation
- ✅ **HttpSseClientCodec_FillRequest**: Tests filling requests with SSE events
- ✅ **HttpSseClientCodec_FillRequest_NullBody**: Tests handling null body
- ✅ **HttpSseClientCodec_FillResponse**: Tests filling responses with SSE events
- ✅ **HttpSseClientCodec_FillResponse_MultipleEvents**: Tests multiple event handling
- ✅ **HttpSseClientCodec_FillResponse_EmptyContent**: Tests empty content handling
- ✅ **HttpSseClientCodec_ZeroCopyEncode**: Tests request encoding

### Server Codec Tests
- ✅ **HttpSseServerCodec_CreateRequestObject**: Tests request object creation
- ✅ **HttpSseServerCodec_CreateResponseObject**: Tests response object creation
- ✅ **HttpSseServerCodec_IsValidSseRequest**: Tests SSE request validation
- ✅ **HttpSseServerCodec_ZeroCopyEncode**: Tests response encoding
- ✅ **HttpSseServerCodec_ZeroCopyDecode**: Tests request decoding

### Protocol Checker Tests
- ✅ **IsValidSseRequest_Function**: Tests request validation function
- ✅ **IsValidSseResponse_Function**: Tests response validation function
- ✅ **HttpSseZeroCopyCheckRequest**: Tests request protocol checking
- ✅ **HttpSseZeroCopyCheckResponse**: Tests response protocol checking

### Integration Tests
- ✅ **ClientServerIntegration**: End-to-end client-server integration test

### Edge Cases and Error Handling
- ✅ **EdgeCase_EmptyEvent**: Tests handling empty events
- ✅ **EdgeCase_LargeEvent**: Tests handling large events (10KB)
- ✅ **EdgeCase_SpecialCharacters**: Tests handling special characters in events

## Test Data

### Valid SSE Events
```cpp
http::sse::SseEvent event;
event.event_type = "message";
event.data = "Hello World";
event.id = "123";
event.retry = 5000;
```

### Valid HTTP SSE Request
```http
GET /events HTTP/1.1
Host: example.com
Accept: text/event-stream
Cache-Control: no-cache
Connection: keep-alive
```

### Valid HTTP SSE Response
```http
HTTP/1.1 200 OK
Content-Type: text/event-stream
Cache-Control: no-cache
Connection: keep-alive

data: Hello World
```

## Test Assertions

The tests use Google Test framework with the following common assertions:

- `EXPECT_TRUE()` / `EXPECT_FALSE()`: Boolean assertions
- `EXPECT_EQ()`: Equality assertions
- `EXPECT_GT()`: Greater than assertions
- `EXPECT_FALSE()`: False assertions
- `EXPECT_TRUE()`: True assertions

## Debugging Tests

### Verbose Output
```bash
bazel test //trpc/codec/http_sse:http_sse_codec_test --test_output=all --verbose_failures
```

### Debug Build
```bash
bazel test //trpc/codec/http_sse:http_sse_codec_test -c dbg --test_output=all
```

### Single Test Debug
```bash
bazel test //trpc/codec/http_sse:http_sse_codec_test --test_filter=TestName --test_output=all --verbose_failures
```

## Test Dependencies

The tests depend on:
- `gtest`: Google Test framework
- `trpc/codec/http_sse:http_sse_codec`: Main SSE codec library
- `trpc/util/http/sse:sse_parser`: SSE parsing utilities
- `trpc/util/http/sse:sse_event`: SSE event structures
- `trpc/util/buffer:noncontiguous_buffer`: Buffer utilities
- `trpc/runtime/iomodel/reactor/common:connection`: Connection utilities

## Adding New Tests

When adding new tests:

1. **Follow Naming Convention**: Use descriptive test names like `TestCategory_TestScenario`
2. **Test Both Success and Failure Cases**: Always test both valid and invalid inputs
3. **Include Edge Cases**: Test boundary conditions and edge cases
4. **Add Documentation**: Update this README with new test descriptions
5. **Update BUILD File**: Add new test files to the BUILD configuration

## Continuous Integration

These tests are designed to run in CI/CD pipelines:

```yaml
# Example CI configuration
- name: Build and Test HTTP SSE Codec
  run: |
    bazel build //trpc/codec/http_sse:all
    bazel test //trpc/codec/http_sse:all
```

## Performance Considerations

- Tests are designed to be fast and lightweight
- Large data tests use reasonable sizes (10KB max)
- Integration tests avoid network dependencies
- Mock objects are used where appropriate

## Troubleshooting

### Common Issues

1. **Build Failures**: Check that all dependencies are properly configured in BUILD files
2. **Test Failures**: Run with `--test_output=all` for detailed error messages
3. **Linker Errors**: Ensure all required libraries are included in dependencies
4. **Runtime Errors**: Check that mock objects are properly initialized

### Getting Help

If you encounter issues:
1. Check the test output for specific error messages
2. Verify that all dependencies are correctly specified
3. Ensure you're running from the correct directory
4. Check that the codec implementation matches the test expectations
