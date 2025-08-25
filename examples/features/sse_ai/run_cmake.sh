#!/bin/bash
#
# Tencent is pleased to support the open source community by making tRPC available.
#
# Copyright (C) 2023 Tencent.
# All rights reserved.
#
# If you have downloaded a copy of the tRPC source code from Tencent,
# please note that tRPC source code is licensed under the  Apache 2.0 License,
# A copy of the Apache 2.0 License is included in this file.
#

set -e

echo "Building SSE AI example with CMake..."

# Check if we're in a path with spaces
if [[ "$PWD" == *" "* ]]; then
    echo "⚠️  Warning: Current path contains spaces. This may cause issues with some build tools."
    echo "Current path: $PWD"
    echo "Consider moving the project to a path without spaces for best compatibility."
    echo ""
fi

# Create build directory
mkdir -p build
cd build

# Build with CMake
echo "Configuring with CMake..."
cmake -DCMAKE_BUILD_TYPE=Release ..

echo "Building with CMake..."
make -j$(nproc)

cd ..

echo "Build completed successfully!"

# Kill any existing server process
echo "Stopping any existing sse_ai_server..."
killall sse_ai_server 2>/dev/null || true
sleep 1

# Start server
echo "Starting SSE AI server..."
./build/sse_ai_server --config=server/trpc_cpp_fiber.yaml &
server_pid=$!

# Wait for server to start
echo "Waiting for server to initialize..."
sleep 3

# Check if server is running
if ! ps -p $server_pid > /dev/null; then
    echo "❌ Failed to start server"
    exit 1
fi

echo "✅ Server started successfully (PID: $server_pid)"

# Function to cleanup on exit
cleanup() {
    echo "Cleaning up..."
    kill $server_pid 2>/dev/null || true
    killall sse_ai_server 2>/dev/null || true
}
trap cleanup EXIT

echo ""
echo "🤖 Running SSE AI Client Demo..."
echo "=================================================="

# Run client with default question
echo "Testing with default question..."
./build/sse_ai_client --client_config=client/trpc_cpp_fiber.yaml

echo ""
echo "Testing with weather question..."
./build/sse_ai_client --client_config=client/trpc_cpp_fiber.yaml --question="What's the weather like today?"

echo ""
echo "Testing with programming question..."
./build/sse_ai_client --client_config=client/trpc_cpp_fiber.yaml --question="Can you show me some C++ code?"

echo ""
echo "Testing with tRPC question..."
./build/sse_ai_client --client_config=client/trpc_cpp_fiber.yaml --question="Tell me about tRPC framework"

echo ""
echo "=================================================="
echo "🎉 SSE AI Demo completed!"
echo ""
echo "You can also test manually with curl:"
echo "curl -N -H \"Accept: text/event-stream\" \"http://127.0.0.1:24857/ai/chat?question=Hello\""
echo ""
echo "Or check server health:"
echo "curl http://127.0.0.1:24857/health"