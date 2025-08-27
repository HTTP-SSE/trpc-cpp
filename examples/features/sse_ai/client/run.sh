#!/bin/bash

# SSE AI Client with HttpSseStreamProxy Demo Runner
# This script demonstrates how to build and run the SSE proxy integration demo

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
SERVER_TARGET="//examples/features/sse_ai/server:sse_ai_server"
CLIENT_TARGET="//examples/features/sse_ai/client:client"
DEMO_TARGET="//examples/features/sse_ai/client:sse_proxy_demo"
SERVER_ADDR="127.0.0.1:24857"
QUIET_MODE=${QUIET_MODE:-false}

# Dynamic configuration file resolution
resolve_config_file() {
    local current_dir=$(pwd)
    local config_name="trpc_cpp_fiber.yaml"
    if [ "$QUIET_MODE" = "true" ]; then
        config_name="trpc_cpp_fiber_quiet.yaml"
    fi
    
    local config_candidates=(
        "$current_dir/$config_name"                                           # Absolute path from current directory
        "examples/features/sse_ai/client/$config_name"                      # From workspace root
        "$config_name"                                                       # From current directory
        "../client/$config_name"                                             # From parent directory
        "./examples/features/sse_ai/client/$config_name"                     # Alternative path
    )
    
    for config_path in "${config_candidates[@]}"; do
        if [ -f "$config_path" ]; then
            echo "$config_path"
            return 0
        fi
    done
    
    return 1
}

# Function to show config resolution debug info
show_config_debug() {
    local current_dir=$(pwd)
    local config_name="trpc_cpp_fiber.yaml"
    if [ "$QUIET_MODE" = "true" ]; then
        config_name="trpc_cpp_fiber_quiet.yaml"
    fi
    
    local config_candidates=(
        "$current_dir/$config_name"                                           # Absolute path from current directory
        "examples/features/sse_ai/client/$config_name"                      # From workspace root
        "$config_name"                                                       # From current directory
        "../client/$config_name"                                             # From parent directory
        "./examples/features/sse_ai/client/$config_name"                     # Alternative path
    )
    
    if [ "$QUIET_MODE" != "true" ]; then
        echo -e "${YELLOW}🔍 Resolving configuration...${NC}"
    fi
    
    for config_path in "${config_candidates[@]}"; do
        if [ -f "$config_path" ]; then
            if [ "$QUIET_MODE" != "true" ]; then
                echo -e "${GREEN}✅ Found config: $(basename $config_path)${NC}"
            fi
            return 0
        fi
    done
    
    echo -e "${RED}❌ Configuration file not found${NC}"
    return 1
}

# Resolve configuration file
show_config_debug
CONFIG_FILE=$(resolve_config_file)
if [ $? -ne 0 ]; then
    echo -e "${RED}❌ Failed to resolve configuration file${NC}"
    exit 1
fi

echo -e "${BLUE}🌊 === SSE AI Client with HttpSseStreamProxy Demo ===${NC}"
echo -e "${BLUE}This script demonstrates the HttpSseStreamProxy integration with tRPC architecture${NC}"
echo ""

# Function to check if server is running
check_server() {
    local retries=10
    local count=0
    
    if [ "$QUIET_MODE" != "true" ]; then
        echo -e "${YELLOW}⏳ Checking server readiness...${NC}"
    fi
    
    while [ $count -lt $retries ]; do
        if curl -s --connect-timeout 2 "http://${SERVER_ADDR}/health" >/dev/null 2>&1; then
            if [ "$QUIET_MODE" != "true" ]; then
                echo -e "${GREEN}✅ Server ready!${NC}"
            fi
            return 0
        fi
        
        if [ "$QUIET_MODE" != "true" ] && [ $((count % 3)) -eq 0 ]; then
            echo -e "${YELLOW}   Waiting for server... ($((count + 1))/${retries})${NC}"
        fi
        sleep 2
        count=$((count + 1))
    done
    
    echo -e "${RED}❌ Server not responding${NC}"
    return 1
}

# Function to build targets
build_targets() {
    if [ "$QUIET_MODE" != "true" ]; then
        echo -e "${BLUE}🔨 Building components...${NC}"
        
        echo -e "${YELLOW}Building HttpSseStreamProxy...${NC}"
        bazel build //trpc/client/sse:http_sse_stream_proxy --ui_event_filters=-info,-debug,-warning >/dev/null 2>&1
        
        echo -e "${YELLOW}Building SSE AI server...${NC}"
        bazel build $SERVER_TARGET --ui_event_filters=-info,-debug,-warning >/dev/null 2>&1
        
        echo -e "${YELLOW}Building SSE AI client...${NC}"
        bazel build $CLIENT_TARGET --ui_event_filters=-info,-debug,-warning >/dev/null 2>&1
        
        echo -e "${YELLOW}Building SSE proxy demo...${NC}"
        bazel build $DEMO_TARGET --ui_event_filters=-info,-debug,-warning >/dev/null 2>&1
        
        echo -e "${GREEN}✅ All components built!${NC}"
    else
        echo -e "${YELLOW}Building...${NC}"
        bazel build //trpc/client/sse:http_sse_stream_proxy $SERVER_TARGET $CLIENT_TARGET $DEMO_TARGET --ui_event_filters=-info,-debug,-warning >/dev/null 2>&1
        echo -e "${GREEN}✅ Built${NC}"
    fi
}

# Function to run server in background
start_server() {
    if [ "$QUIET_MODE" != "true" ]; then
        echo -e "${BLUE}🚀 Starting SSE AI server...${NC}"
    fi
    
    # Kill any existing server process
    pkill -f "sse_ai_server" 2>/dev/null || true
    sleep 1
    
    # Choose server config based on quiet mode
    local server_config="trpc_cpp_fiber.yaml"
    if [ "$QUIET_MODE" = "true" ]; then
        server_config="trpc_cpp_fiber_quiet.yaml"
    fi
    
    # Start server in background with appropriate config
    bazel run $SERVER_TARGET -- --addr=$SERVER_ADDR --config=$server_config > server.log 2>&1 &
    SERVER_PID=$!
    
    if [ "$QUIET_MODE" != "true" ]; then
        echo -e "${YELLOW}Server PID: $SERVER_PID${NC}"
    fi
    
    # Wait for server to be ready
    sleep 3
    
    if check_server; then
        if [ "$QUIET_MODE" != "true" ]; then
            echo -e "${GREEN}✅ Server ready for connections${NC}"
        fi
        return 0
    else
        echo -e "${RED}❌ Failed to start server${NC}"
        return 1
    fi
}

# Function to run original client
run_original_client() {
    echo -e "${BLUE}🔄 Running original SSE client...${NC}"
    echo -e "${YELLOW}Question: Hello! How can you help me?${NC}"
    echo ""
    
    bazel run $CLIENT_TARGET -- \
        --addr=$SERVER_ADDR \
        --question="Hello! How can you help me?" \
        --client_config="$CONFIG_FILE" \
        2>/dev/null
    
    echo ""
    echo -e "${GREEN}✅ Original client completed${NC}"
}

# Function to run demo client
run_demo_client() {
    if [ "$QUIET_MODE" = "true" ]; then
        echo -e "${BLUE}🎯 Running HttpSseStreamProxy demo...${NC}"
        echo -e "${YELLOW}Question: Tell me about tRPC framework and HttpSseStreamProxy integration${NC}"
        echo ""
        
        # Use quiet config and filter out any remaining log messages
        bazel run $DEMO_TARGET -- \
            --addr=$SERVER_ADDR \
            --question="Tell me about tRPC framework and HttpSseStreamProxy integration" \
            --client_config="$CONFIG_FILE" \
            2>/dev/null | grep -v "\[thread" | grep -v "\[info\]" | grep -v "\[debug\]" | grep -v "http_sse_stream_proxy.cc"
    else
        echo -e "${BLUE}🎯 Running HttpSseStreamProxy demo...${NC}"
        echo -e "${YELLOW}Question: Tell me about tRPC framework and HttpSseStreamProxy integration${NC}"
        echo ""
        
        bazel run $DEMO_TARGET -- \
            --addr=$SERVER_ADDR \
            --question="Tell me about tRPC framework and HttpSseStreamProxy integration" \
            --client_config="$CONFIG_FILE" \
            2>/dev/null
    fi
    
    echo ""
    echo -e "${GREEN}✅ Demo completed${NC}"
}

# Function to run multiple test questions
run_test_suite() {
    if [ "$QUIET_MODE" = "true" ]; then
        echo -e "${BLUE}🧪 Running test suite (quiet)...${NC}"
    else
        echo -e "${BLUE}🧪 Running test suite...${NC}"
    fi
    
    local questions=(
        "What's the weather like today?"
        "Can you show me a simple C++ code example?"
        "Explain how tRPC SSE integration works"
        "What are the benefits of HttpSseStreamProxy?"
    )
    
    for i in "${!questions[@]}"; do
        if [ "$QUIET_MODE" != "true" ]; then
            echo -e "${YELLOW}Test $((i + 1)): ${questions[i]}${NC}"
        fi
        
        bazel run $DEMO_TARGET -- \
            --addr=$SERVER_ADDR \
            --question="${questions[i]}" \
            --client_config="$CONFIG_FILE" \
            2>/dev/null | if [ "$QUIET_MODE" = "true" ]; then grep -v "\[thread" | grep -v "\[info\]" | grep -v "\[debug\]" | grep -v "http_sse_stream_proxy.cc"; else cat; fi
        
        if [ "$QUIET_MODE" != "true" ]; then
            echo ""
            echo -e "${GREEN}✅ Test $((i + 1)) completed${NC}"
            echo "---"
        fi
        
        # Wait between requests
        sleep 1
    done
    
    if [ "$QUIET_MODE" = "true" ]; then
        echo -e "${GREEN}✅ All tests completed${NC}"
    fi
}

# Function to cleanup
cleanup() {
    echo -e "${YELLOW}🧹 Cleaning up...${NC}"
    
    if [ ! -z "$SERVER_PID" ]; then
        kill $SERVER_PID 2>/dev/null || true
        sleep 1
        kill -9 $SERVER_PID 2>/dev/null || true
    fi
    
    # Kill any remaining server processes
    pkill -f "sse_ai_server" 2>/dev/null || true
    
    echo -e "${GREEN}✅ Cleanup completed${NC}"
}

# Function to show usage
show_usage() {
    echo -e "${BLUE}Usage: $0 [OPTION]${NC}"
    echo ""
    echo "Options:"
    echo "  build-only     Build all components without running"
    echo "  original       Run only the original SSE client"
    echo "  demo           Run only the HttpSseStreamProxy demo"
    echo "  quiet          Run demo with minimal output (quiet mode)"
    echo "  silent         Run demo with ultra-minimal output"
    echo "  test-suite     Run comprehensive test suite"
    echo "  compare        Run both original and demo clients for comparison"
    echo "  full           Build and run complete demonstration (default)"
    echo "  help           Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0              # Run full demonstration"
    echo "  $0 quiet        # Run demo with minimal output"
    echo "  $0 silent       # Run demo with ultra-minimal output"
    echo "  $0 demo         # Run only the proxy demo"
    echo "  $0 compare      # Compare original vs proxy implementation"
    echo "  $0 test-suite   # Run comprehensive tests"
}

# Trap to ensure cleanup on exit
trap cleanup EXIT

# Main execution based on arguments
case "${1:-full}" in
    "build-only")
        build_targets
        ;;
    "original")
        build_targets
        start_server
        run_original_client
        ;;
    "demo")
        build_targets
        start_server
        run_demo_client
        ;;
    "test-suite")
        build_targets
        start_server
        run_test_suite
        ;;
    "quiet")
        QUIET_MODE=true
        build_targets
        start_server
        echo -e "${BLUE}🎯 === HttpSseStreamProxy Demo (Quiet Mode) ===${NC}"
        run_demo_client
        ;;
    "silent")
        QUIET_MODE=true
        build_targets >/dev/null 2>&1
        start_server >/dev/null 2>&1
        echo -e "${BLUE}SSE Demo${NC}"
        bazel run $DEMO_TARGET -- \
            --addr=$SERVER_ADDR \
            --question="Tell me about tRPC framework and HttpSseStreamProxy integration" \
            --client_config="$CONFIG_FILE" \
            2>/dev/null | grep -v "\[" | grep -v "HTTP SSE" | grep -v "===" | grep -v "This demo" | grep -v "Question:" | sed '/^$/d'
        echo -e "${GREEN}✅ Done${NC}"
        ;;
    "compare")
        build_targets
        start_server
        echo -e "${BLUE}📊 === Comparison Demo ===${NC}"
        run_original_client
        echo ""
        echo -e "${BLUE}HttpSseStreamProxy demo:${NC}"
        run_demo_client
        ;;
    "full")
        build_targets
        start_server
        echo -e "${BLUE}🎯 === Complete HttpSseStreamProxy Demo ===${NC}"
        run_original_client
        echo ""
        echo -e "${BLUE}HttpSseStreamProxy integration:${NC}"
        run_demo_client
        echo ""
        echo -e "${BLUE}Additional test cases:${NC}"
        run_test_suite
        ;;
    "help"|"-h"|"--help")
        show_usage
        exit 0
        ;;
    *)
        echo -e "${RED}❌ Unknown option: $1${NC}"
        show_usage
        exit 1
        ;;
esac

echo ""
echo -e "${GREEN}🎉 === Demo Summary ===${NC}"
echo -e "${GREEN}✅ HttpSseStreamProxy integration completed successfully!${NC}"
echo ""
echo -e "${BLUE}Key Features:${NC}"
echo -e "  • tRPC-integrated SSE proxy component"
echo -e "  • ServiceProxy pattern implementation"
echo -e "  • Real-time streaming and event processing"
echo -e "  • Clean AI response display"
echo ""
echo -e "${YELLOW}📁 Generated: server.log${NC}"
echo -e "${GREEN}Demo completed! 🚀${NC}"