//
// Tencent is pleased to support the open source community by making tRPC available.
//
// Copyright (C) 2023 Tencent.
// All rights reserved.
//
// If you have downloaded a copy of the tRPC source code from Tencent,
// please note that tRPC source code is licensed under the  Apache 2.0 License,
// A copy of the Apache 2.0 License is included in this file.
//

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "gflags/gflags.h"

#include "trpc/client/http/http_service_proxy.h"
#include "trpc/client/make_client_context.h"
#include "trpc/client/sse/http_sse_stream_proxy.h"
#include "trpc/client/trpc_client.h"
#include "trpc/codec/http/http_protocol.h"
#include "trpc/codec/http_sse/http_sse_codec.h"
#include "trpc/codec/client_codec_factory.h"
#include "trpc/codec/client_codec.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/common/status.h"
#include "trpc/common/trpc_plugin.h"
#include "trpc/coroutine/fiber.h"
#include "trpc/stream/http/http_client_stream.h"
#include "trpc/util/http/sse/sse_event.h"
#include "trpc/util/http/sse/sse_parser.h"
#include "trpc/util/http/util.h"
#include "trpc/util/log/logging.h"

DEFINE_string(service_name, "sse_ai_client", "client service name");
DEFINE_string(client_config, "trpc_cpp_fiber.yaml", "client configuration file");
DEFINE_string(addr, "127.0.0.1:24857", "server address (ip:port)");
DEFINE_string(question, "Hello! How can you help me?", "question to ask the AI");

namespace sse_ai::demo {

// Register HTTP SSE codec
void RegisterHttpSseCodec() {
  auto client_codec = std::make_shared<::trpc::HttpSseClientCodec>();
  ::trpc::ClientCodecFactory::GetInstance()->Register(client_codec);
  TRPC_LOG_INFO("HTTP SSE client codec registered successfully");
}

class SseAiClientWithProxy {
 public:
  SseAiClientWithProxy() = default;
  
  bool TestSseAiChatWithProxy() {
    std::cout << "\n🌊 === SSE AI Chat Demo with HttpSseStreamProxy ===" << std::endl;
    std::cout << "Question: " << FLAGS_question << std::endl;

    // Create service proxy options
    ::trpc::ServiceProxyOption option;
    option.name = FLAGS_service_name;
    option.codec_name = "http";
    option.network = "tcp";
    option.conn_type = "long";
    option.timeout = 60000; // 60 seconds timeout for SSE
    option.selector_name = "direct";
    option.target = FLAGS_addr;

    // Create HttpSseStreamProxy using the factory function from the library
    auto sse_proxy = ::trpc::CreateHttpSseStreamProxy(option);
    if (!sse_proxy) {
      std::cerr << "Failed to create HttpSseStreamProxy" << std::endl;
      return false;
    }

    // Create SSE context
    std::string url = "/ai/chat?question=" + ::trpc::http::PercentEncode(FLAGS_question);
    auto ctx = sse_proxy->CreateSseContext(url, 60000);
    if (!ctx) {
      std::cerr << "Failed to create SSE context" << std::endl;
      return false;
    }

    std::cout << "AI: ";
    
    // Connect and receive SSE events using the proxy
    bool success = true;
    auto status = sse_proxy->ConnectAndReceive(ctx, url, [&](const ::trpc::http::sse::SseEvent& event) {
      return DisplaySseEvent(event);
    });
    
    if (!status.OK()) {
      std::cerr << "\nSSE connection failed: " << status.ToString() << std::endl;
      success = false;
    }
    
    return success;
  }

  bool TestMultipleQuestionsWithProxy() {
    std::cout << "\n=== Multiple AI Questions with HttpSseStreamProxy ===" << std::endl;
    
    std::vector<std::string> questions = {
      "What's the weather like today?",
      "Can you show me a simple code example?",
      "Tell me about tRPC framework"
    };

    bool all_success = true;
    
    for (size_t i = 0; i < questions.size(); ++i) {
      std::cout << "\n--- Question " << (i + 1) << " ---" << std::endl;
      std::cout << "Q: " << questions[i] << std::endl;
      
      bool success = SendQuestionAndReceiveResponseWithProxy(questions[i]);
      if (!success) {
        all_success = false;
      }
      
      // Wait a bit between questions
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    return all_success;
  }

 private:
  bool SendQuestionAndReceiveResponseWithProxy(const std::string& question) {
    // Create service proxy options
    ::trpc::ServiceProxyOption option;
    option.name = FLAGS_service_name;
    option.codec_name = "http";
    option.network = "tcp";
    option.conn_type = "long";
    option.timeout = 30000;
    option.selector_name = "direct";
    option.target = FLAGS_addr;

    auto sse_proxy = ::trpc::CreateHttpSseStreamProxy(option);
    if (!sse_proxy) {
      std::cerr << "Failed to create HttpSseStreamProxy" << std::endl;
      return false;
    }

    std::string url = "/ai/chat?question=" + ::trpc::http::PercentEncode(question);
    auto ctx = sse_proxy->CreateSseContext(url, 30000);
    if (!ctx) {
      std::cerr << "Failed to create SSE context" << std::endl;
      return false;
    }

    std::cout << "AI: ";
    
    auto status = sse_proxy->ConnectAndReceive(ctx, url, [&](const ::trpc::http::sse::SseEvent& event) {
      return DisplaySseEvent(event);
    });
    
    return status.OK();
  }

  bool DisplaySseEvent(const ::trpc::http::sse::SseEvent& event) {
    if (event.event_type == "ai_start") {
      return true; // Don't show technical details
    } else if (event.event_type == "ai_chunk") {
      // Display only clean content
      std::string data = event.data;
      // Replace escaped newlines with actual newlines
      size_t pos = 0;
      while ((pos = data.find("\\n", pos)) != std::string::npos) {
        data.replace(pos, 2, "\n");
        pos += 1;
      }
      std::cout << data << std::flush;
    } else if (event.event_type == "ai_complete") {
      std::cout << std::endl; // Just clean line break
    }
    return true; // Continue reading
  }
};

}  // namespace sse_ai::demo

int main(int argc, char* argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  gflags::SetUsageMessage("SSE AI client with HttpSseStreamProxy integration demo");

  // Initialize tRPC framework
  auto ret = ::trpc::TrpcConfig::GetInstance()->Init(FLAGS_client_config);
  if (ret != 0) {
    std::cerr << "Failed to init trpc config: " << FLAGS_client_config << ", ret: " << ret << std::endl;
    return ret;
  }

  // Register SSE codec
  sse_ai::demo::RegisterHttpSseCodec();

  // Use RunInTrpcRuntime for proper runtime lifecycle management
  return ::trpc::RunInTrpcRuntime([]() {
    std::cout << "\n🎯 === HttpSseStreamProxy Integration Demonstration ===" << std::endl;
    std::cout << "This demo shows how HttpSseStreamProxy integrates with tRPC architecture" << std::endl;
    std::cout << "to provide a specialized SSE proxy layer." << std::endl;

    // Create and run the SSE AI client with proxy
    sse_ai::demo::SseAiClientWithProxy client;
    
    bool success = true;
    
    // Test single question with proxy
    if (!client.TestSseAiChatWithProxy()) {
      success = false;
    }
    
    // Test multiple questions with proxy
    if (!client.TestMultipleQuestionsWithProxy()) {
      success = false;
    }

    std::cout << "\n🎯 === Demo Summary ===" << std::endl;
    if (success) {
      std::cout << "✅ HttpSseStreamProxy integration successful!" << std::endl;
      std::cout << "The proxy demonstrates:" << std::endl;
      std::cout << "  - Proper tRPC ServiceProxy integration" << std::endl;
      std::cout << "  - SSE-specific header management" << std::endl;
      std::cout << "  - Streaming event processing" << std::endl;
      std::cout << "  - Clean AI response display" << std::endl;
    } else {
      std::cout << "❌ HttpSseStreamProxy integration failed." << std::endl;
    }

    return success ? 0 : 1;
  });
}