#!/bin/bash
#
# SSE AI Demo Script for tRPC-Cpp
#
# Usage:
#   ./run.sh          - Run full demo (build + server + client tests)
#   ./run.sh server    - Start server only
#   ./run.sh client    - Run client only (requires server to be running)
#   ./run.sh demo      - Interactive SSE streaming demo
#
# Examples:
#   ./run.sh demo      - Best way to see SSE streaming in action!
#   ./run.sh server &  - Start server in background
#   ./run.sh client    - Test client against running server
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

# Parse command line arguments
MODE="full"
if [[ $# -gt 0 ]]; then
    MODE="$1"
fi

case "$MODE" in
    "server")
        echo "🚀 Starting SSE AI Server Only..."
        ;;
    "client")
        echo "🤖 Running SSE AI Client Only..."
        ;;
    "demo")
        echo "🌊 Running SSE Streaming Demo..."
        ;;
    "full"|*)
        echo "🎯 Running Full SSE AI Demo..."
        ;;
esac

# Function to check if server is running
check_server_running() {
    # Try multiple methods to check server
    if command -v curl > /dev/null 2>&1; then
        curl -s http://127.0.0.1:24857/health > /dev/null 2>&1
    elif command -v wget > /dev/null 2>&1; then
        wget -q --spider http://127.0.0.1:24857/health > /dev/null 2>&1
    elif command -v nc > /dev/null 2>&1; then
        nc -z 127.0.0.1 24857 > /dev/null 2>&1
    else
        # Fallback: try to connect using telnet-like approach
        (echo > /dev/tcp/127.0.0.1/24857) > /dev/null 2>&1
    fi
}

# Function for SSE streaming demo
run_sse_streaming_demo() {
    echo ""
    echo "🌊 === SSE Streaming Demo ==="
    echo "=========================================="
    echo "This demo showcases real-time HTTP SSE streaming"
    echo ""
    
    # Interactive question selection
    echo "Choose a demo question:"
    echo "1) Tell me about tRPC framework streaming capabilities"
    echo "2) What's the weather like today?"
    echo "3) Can you show me some C++ code examples?"
    echo "4) Custom question"
    echo "5) Run all questions"
    echo ""
    read -p "Enter your choice (1-5): " choice
    
    case "$choice" in
        "1")
            demo_question="Tell me about tRPC framework streaming capabilities"
            ;;
        "2")
            demo_question="What's the weather like today?"
            ;;
        "3")
            demo_question="Can you show me some C++ code examples?"
            ;;
        "4")
            read -p "Enter your custom question: " demo_question
            ;;
        "5")
            run_all_demo_questions
            return
            ;;
        *)
            demo_question="Hello! How can you help me with tRPC?"
            ;;
    esac
    
    echo ""
    echo "🚀 Starting SSE Streaming Demo..."
    echo "Question: $demo_question"
    echo ""
    echo "🔄 Connecting to SSE stream... (Watch for real-time response!)"
    echo "=========================================="
    
    # Execute SSE streaming client
    $CLIENT_BIN --client_config=$CLIENT_CONFIG --question="$demo_question"
    
    echo ""
    echo "=========================================="
    echo "✅ SSE Streaming Demo Complete!"
    echo ""
    echo "📝 Demo Features Showcased:"
    echo "  ✓ Real-time HTTP SSE connection"
    echo "  ✓ Continuous data streaming"
    echo "  ✓ Live AI response rendering"
    echo "  ✓ Proper connection lifecycle"
    echo ""
}

# Function to run all demo questions
run_all_demo_questions() {
    questions=(
        "Tell me about tRPC framework streaming capabilities"
        "What's the weather like today?"
        "Can you show me some C++ code examples?"
        "How does SSE work in web applications?"
    )
    
    echo ""
    echo "🌊 Running Complete SSE Demo Suite..."
    echo "=========================================="
    
    for i in "${!questions[@]}"; do
        question="${questions[$i]}"
        echo ""
        echo "🔢 Demo $((i+1))/${#questions[@]}: $question"
        echo "-------------------"
        
        $CLIENT_BIN --client_config=$CLIENT_CONFIG --question="$question"
        
        echo ""
        if [[ $((i+1)) -lt ${#questions[@]} ]]; then
            echo "Waiting 2 seconds before next demo..."
            sleep 2
        fi
    done
    
    echo ""
    echo "=========================================="
    echo "✅ All SSE Streaming Demos Complete!"
}

# Function for full client demo (original behavior)
run_full_client_demo() {
    echo ""
    echo "🤖 Running SSE AI Client Demo..."
    echo "=================================================="
    
    # Run client with default question
    echo "➡️ Testing with default question..."
    echo "DEBUG: Executing command: $CLIENT_BIN --client_config=$CLIENT_CONFIG"
    $CLIENT_BIN --client_config=$CLIENT_CONFIG
    
    echo ""
    echo "➡️ Testing with weather question..."
    $CLIENT_BIN --client_config=$CLIENT_CONFIG --question="What's the weather like today?"
    
    echo ""
    echo "➡️ Testing with programming question..."
    $CLIENT_BIN --client_config=$CLIENT_CONFIG --question="Can you show me some C++ code?"
    
    echo ""
    echo "➡️ Testing with tRPC question..."
    $CLIENT_BIN --client_config=$CLIENT_CONFIG --question="Tell me about tRPC framework"
    
    echo ""
    echo "=================================================="
    echo "🎆 SSE AI Demo completed!"
    
    echo ""
    echo "🔧 Manual Testing Options:"
    echo "Test with curl: curl -N -H 'Accept: text/event-stream' 'http://127.0.0.1:24857/ai/chat?question=Hello'"
    echo "Check health: curl http://127.0.0.1:24857/health"
    echo "SSE Demo: ./run.sh demo"
}

# Function to cleanup on exit
cleanup() {
    if [[ "$MODE" != "client" && -n "$server_pid" ]]; then
        echo "Cleaning up..."
        kill $server_pid 2>/dev/null || true
        killall sse_ai_server 2>/dev/null || true
    fi
}

echo "Building SSE AI example..."

# Check if we're in a path with spaces
if [[ "$PWD" == *" "* ]]; then
    echo "❌ Error: Bazel does not work properly with paths containing spaces."
    echo "Current path: $PWD"
    echo ""
    echo "Solutions:"
    echo "1. Move the project to a path without spaces:"
    echo "   mv \"$(dirname \"$PWD\")\" /tmp/trpc-cpp-Draft"
    echo "   cd /tmp/trpc-cpp-Draft/examples/features/sse_ai"
    echo "   ./run.sh"
    echo ""
    echo "2. Use CMake instead:"
    echo "   ./run_cmake.sh"
    echo ""
    exit 1
fi

# Building server and client
echo "Building server..."
bazel build //examples/features/sse_ai/server:sse_ai_server

echo "Building client..."
bazel build //examples/features/sse_ai/client:client

echo "Build completed successfully!"


# Skip server start if only running client
if [[ "$MODE" == "client" ]]; then
    echo "Skipping server startup (client-only mode)"
    echo "Checking if server is running..."
    
    # Check for required tools
    if ! command -v curl > /dev/null 2>&1 && ! command -v wget > /dev/null 2>&1 && ! command -v nc > /dev/null 2>&1; then
        echo "⚠️  Warning: curl, wget, and nc are not available."
        echo "Installing curl for better server detection..."
        echo "Please run: sudo apt install curl"
        echo ""
        echo "Attempting basic connectivity test..."
    fi
    
    # Wait a bit and retry server detection
    server_ready=false
    for i in {1..10}; do
        if check_server_running; then
            echo "✅ Server is running and ready!"
            server_ready=true
            break
        fi
        echo "Attempt $i/10: Waiting for server..."
        sleep 1
    done
    
    if [[ "$server_ready" != "true" ]]; then
        echo "❌ Server is not running or not ready."
        echo ""
        echo "Please start server first in another terminal:"
        echo "  bash run.sh server"
        echo ""
        echo "Or test if server is actually running:"
        if command -v curl > /dev/null 2>&1; then
            echo "  curl http://127.0.0.1:24857/health"
        elif command -v wget > /dev/null 2>&1; then
            echo "  wget -q --spider http://127.0.0.1:24857/health"
        else
            echo "  nc -z 127.0.0.1 24857"
        fi
        echo "  ps aux | grep sse_ai_server"
        exit 1
    fi
else
    # Kill any existing server process
    echo "Stopping any existing sse_ai_server..."
    killall sse_ai_server 2>/dev/null || true
    sleep 1

    # Start server
    echo "Starting SSE AI server..."
fi

# Determine the correct path to the binary
if [[ -f "bazel-bin/examples/features/sse_ai/server/sse_ai_server" ]]; then
    SERVER_BIN="bazel-bin/examples/features/sse_ai/server/sse_ai_server"
elif [[ -f "../../../bazel-bin/examples/features/sse_ai/server/sse_ai_server" ]]; then
    SERVER_BIN="../../../bazel-bin/examples/features/sse_ai/server/sse_ai_server"
else
    echo "❌ Error: Cannot find sse_ai_server binary."
    echo "Please make sure you're running this script from the workspace root or the example directory."
    echo "Expected locations:"
    echo "  - bazel-bin/examples/features/sse_ai/server/sse_ai_server"
    echo "  - ../../../bazel-bin/examples/features/sse_ai/server/sse_ai_server"
    exit 1
fi

echo "Using server binary: $SERVER_BIN"

# Determine the correct config file path
if [[ -f "examples/features/sse_ai/server/trpc_cpp_fiber.yaml" ]]; then
    SERVER_CONFIG="examples/features/sse_ai/server/trpc_cpp_fiber.yaml"
elif [[ -f "server/trpc_cpp_fiber.yaml" ]]; then
    SERVER_CONFIG="server/trpc_cpp_fiber.yaml"
else
    echo "❌ Error: Cannot find server config file."
    echo "Expected locations:"
    echo "  - examples/features/sse_ai/server/trpc_cpp_fiber.yaml"
    echo "  - server/trpc_cpp_fiber.yaml"
    exit 1
fi

if [[ "$MODE" != "client" ]]; then
    echo "Using server config: $SERVER_CONFIG"
    $SERVER_BIN --config=$SERVER_CONFIG &
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
fi

# Exit if only starting server
if [[ "$MODE" == "server" ]]; then
    echo ""
    echo "🌐 SSE AI Server is running at http://127.0.0.1:24857"
    echo "Health check: curl http://127.0.0.1:24857/health"
    echo "Test SSE: curl -N -H 'Accept: text/event-stream' 'http://127.0.0.1:24857/ai/chat?question=Hello'"
    echo ""
    echo "Press Ctrl+C to stop server..."
    wait $server_pid
    exit 0
fi

trap cleanup EXIT

# Determine the correct path to the client binary
if [[ -f "bazel-bin/examples/features/sse_ai/client/client" ]]; then
    CLIENT_BIN="bazel-bin/examples/features/sse_ai/client/client"
elif [[ -f "../../../bazel-bin/examples/features/sse_ai/client/client" ]]; then
    CLIENT_BIN="../../../bazel-bin/examples/features/sse_ai/client/client"
else
    echo "❌ Error: Cannot find client binary."
    echo "Please make sure the client was built successfully."
    exit 1
fi

echo "Using client binary: $CLIENT_BIN"

# Determine the correct client config file path
echo "DEBUG: Checking for client config files..."
echo "DEBUG: Current directory: $(pwd)"
echo "DEBUG: Checking: examples/features/sse_ai/client/trpc_cpp_fiber.yaml"
ls -la examples/features/sse_ai/client/trpc_cpp_fiber.yaml 2>/dev/null || echo "  - Not found"
echo "DEBUG: Checking: client/trpc_cpp_fiber.yaml"
ls -la client/trpc_cpp_fiber.yaml 2>/dev/null || echo "  - Not found"

if [[ -f "examples/features/sse_ai/client/trpc_cpp_fiber.yaml" ]]; then
    CLIENT_CONFIG="examples/features/sse_ai/client/trpc_cpp_fiber.yaml"
elif [[ -f "client/trpc_cpp_fiber.yaml" ]]; then
    CLIENT_CONFIG="client/trpc_cpp_fiber.yaml"
else
    echo "❌ Error: Cannot find client config file."
    echo "Expected locations:"
    echo "  - examples/features/sse_ai/client/trpc_cpp_fiber.yaml"
    echo "  - client/trpc_cpp_fiber.yaml"
    exit 1
fi

echo "Using client config: $CLIENT_CONFIG"
echo "DEBUG: Resolved client config path: $(readlink -f "$CLIENT_CONFIG" 2>/dev/null || echo "$CLIENT_CONFIG")"


# Different demo modes based on user selection
case "$MODE" in
    "demo")
        run_sse_streaming_demo
        ;;
    "client"|"full"|*)
        run_full_client_demo
        ;;
esac
