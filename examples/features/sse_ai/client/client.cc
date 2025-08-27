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
#include "trpc/client/trpc_client.h"
#include "trpc/codec/http/http_protocol.h"
#include "trpc/codec/http_sse/http_sse_codec.h"
#include "trpc/codec/client_codec_factory.h"
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

class SseAiClient {
 public:
  SseAiClient() = default;
  
  bool TestSseAiChat() {
    std::cout << "\n🌊 === SSE AI Chat Demo ===" << std::endl;
    std::cout << "Question: " << FLAGS_question << std::endl;

    // Create HTTP service proxy (which can handle SSE)
    ::trpc::ServiceProxyOption option;
    option.name = FLAGS_service_name;
    option.codec_name = "http";
    option.network = "tcp";
    option.conn_type = "long";
    option.timeout = 60000; // 60 seconds timeout for SSE
    option.selector_name = "direct";
    option.target = FLAGS_addr;

    auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::http::HttpServiceProxy>(FLAGS_service_name, option);
    if (!proxy) {
      std::cerr << "Failed to create HTTP service proxy" << std::endl;
      return false;
    }

    // Create client context
    auto ctx = ::trpc::MakeClientContext(proxy);
    ctx->SetTimeout(60000); // 60 seconds timeout for SSE

    // Send request and receive SSE stream
    std::cout << "AI: ";
    
    bool success = ReceiveSseStreamWithHttpProxy(ctx, proxy, FLAGS_question);
    
    return success;
  }

  bool TestMultipleQuestions() {
    std::cout << "\n=== Multiple AI Questions ===" << std::endl;
    
    std::vector<std::string> questions = {
      "What's the weather like today?",
      "Can you show me a simple code example?",
      "Tell me about tRPC framework"
    };

    bool all_success = true;
    
    for (size_t i = 0; i < questions.size(); ++i) {
      std::cout << "\n--- Question " << (i + 1) << " ---" << std::endl;
      std::cout << "Q: " << questions[i] << std::endl;
      
      bool success = SendQuestionAndReceiveResponse(questions[i]);
      if (!success) {
        all_success = false;
      }
      
      // Wait a bit between questions
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    
    return all_success;
  }

 private:
  bool ReceiveSseStreamWithHttpProxy(const ::trpc::ClientContextPtr& ctx, 
                                     const std::shared_ptr<::trpc::http::HttpServiceProxy>& proxy,
                                     const std::string& question) {
    try {
      // Create HTTP request URL for SSE
      std::string url = "/ai/chat?question=" + ::trpc::http::PercentEncode(question);
      
      // Create request context with SSE headers
      auto request_ctx = ::trpc::MakeClientContext(proxy);
      request_ctx->SetTimeout(30000);
      request_ctx->SetHttpHeader("Accept", "text/event-stream");
      request_ctx->SetHttpHeader("Cache-Control", "no-cache");
      request_ctx->SetHttpHeader("Connection", "keep-alive");
      
      // Use streaming Get method for SSE
      auto stream_rw = proxy->Get(request_ctx, url);
      
      // Check stream status
      if (!stream_rw.GetStatus().OK()) {
        std::cerr << "Failed to create SSE stream: " << stream_rw.GetStatus().ToString() << std::endl;
        return false;
      }
      
      TRPC_LOG_INFO("SSE stream established, reading events...");
      
      // Read SSE events from stream
      std::string accumulated_data;
      bool stream_active = true;
      
      while (stream_active) {
        ::trpc::NoncontiguousBuffer buffer;
        auto status = stream_rw.Read(buffer, 5000); // 5 second timeout
        
        if (!status.OK()) {
          if (status.GetFrameworkRetCode() == static_cast<int>(::trpc::stream::StreamStatus::kStreamEof)) {
            TRPC_LOG_INFO("SSE stream ended normally");
            break;
          } else {
            TRPC_LOG_WARN("SSE stream read error: " << status.ToString());
            break;
          }
        }
        
        // Convert buffer to string and accumulate
        std::string chunk = ::trpc::FlattenSlow(buffer);
        if (!chunk.empty()) {
          accumulated_data += chunk;
          
          // Process any complete events
          ProcessSseDataChunk(accumulated_data);
        }
      }
      
      // Process any remaining data
      if (!accumulated_data.empty()) {
        ProcessSseDataChunk(accumulated_data, true);
      }
      
      return true;

    } catch (const std::exception& e) {
      std::cerr << "Exception in SSE stream: " << e.what() << std::endl;
      return false;
    }
  }

  bool SendQuestionAndReceiveResponse(const std::string& question) {
    // Create HTTP service proxy (dummy for compatibility)
    ::trpc::ServiceProxyOption option;
    option.name = FLAGS_service_name;
    option.codec_name = "http";
    option.network = "tcp";
    option.conn_type = "long";
    option.timeout = 30000;
    option.selector_name = "direct";
    option.target = FLAGS_addr;

    auto proxy = ::trpc::GetTrpcClient()->GetProxy<::trpc::http::HttpServiceProxy>(FLAGS_service_name, option);
    if (!proxy) {
      std::cerr << "Failed to create HTTP service proxy" << std::endl;
      return false;
    }

    auto ctx = ::trpc::MakeClientContext(proxy);
    ctx->SetTimeout(30000);

    std::cout << "AI: ";
    return ReceiveSseStreamWithHttpProxy(ctx, proxy, question);
  }

  void ProcessSseDataChunk(std::string& data, bool force_process = false) {
    try {
      // Split data by double newlines (SSE event separator)
      size_t pos = 0;
      while ((pos = data.find("\n\n")) != std::string::npos) {
        std::string event_data = data.substr(0, pos);
        data = data.substr(pos + 2); // Remove processed part
        
        if (!event_data.empty()) {
          try {
            auto event = ::trpc::http::sse::SseParser::ParseEvent(event_data);
            DisplaySseEvent(event);
          } catch (const std::exception& e) {
            TRPC_LOG_WARN("Failed to parse SSE event: " << e.what() << ", raw data: " << event_data);
          }
        }
      }
      
      // If force_process is true, try to process any remaining data as a final event
      if (force_process && !data.empty()) {
        try {
          auto event = ::trpc::http::sse::SseParser::ParseEvent(data);
          DisplaySseEvent(event);
        } catch (const std::exception& e) {
          TRPC_LOG_WARN("Failed to parse final SSE event: " << e.what() << ", raw data: " << data);
        }
        data.clear();
      }
    } catch (const std::exception& e) {
      std::cerr << "Error processing SSE data chunk: " << e.what() << std::endl;
    }
  }

  void ParseAndDisplaySseEvents(const std::string& sse_data) {
    try {
      // Split response by double newlines (SSE event separator)
      std::vector<std::string> events_raw;
      size_t pos = 0;
      size_t prev_pos = 0;
      
      while ((pos = sse_data.find("\n\n", prev_pos)) != std::string::npos) {
        std::string event_data = sse_data.substr(prev_pos, pos - prev_pos);
        if (!event_data.empty()) {
          events_raw.push_back(event_data);
        }
        prev_pos = pos + 2;
      }
      
      // Add the last event if there's remaining data
      if (prev_pos < sse_data.length()) {
        std::string event_data = sse_data.substr(prev_pos);
        if (!event_data.empty()) {
          events_raw.push_back(event_data);
        }
      }

      // Parse and display events
      for (const auto& event_raw : events_raw) {
        try {
          auto event = ::trpc::http::sse::SseParser::ParseEvent(event_raw);
          DisplaySseEvent(event);
        } catch (const std::exception& e) {
          TRPC_LOG_WARN("Failed to parse SSE event: " << e.what() << ", raw data: " << event_raw);
        }
      }

    } catch (const std::exception& e) {
      std::cerr << "Error parsing SSE events: " << e.what() << std::endl;
    }
  }

  void DisplaySseEvent(const ::trpc::http::sse::SseEvent& event) {
    if (event.event_type == "ai_start") {
      // Start event - just start the response, no technical details
      return; // Don't show "Starting AI response for:" message
    } else if (event.event_type == "ai_chunk") {
      // Display content chunks in real-time - only the actual content
      std::string data = event.data;
      // Replace literal \\n with actual newlines
      ReplaceAll(data, "\\n", "\n");
      // Add space after each chunk for better readability (except for newlines)
      if (!data.empty() && data.back() != '\n' && !data.empty()) {
        std::cout << data;
        // Add space only if the data doesn't end with punctuation or newline
        if (data.back() != ' ' && data.back() != '\n' && data.back() != '.' && 
            data.back() != '!' && data.back() != '?' && data.back() != ':') {
          std::cout << " ";
        }
      } else {
        std::cout << data;
      }
      std::cout << std::flush;
    } else if (event.event_type == "ai_complete") {
      // Just add a clean line break when response is complete
      std::cout << std::endl;
    } else if (event.event_type == "ai_error") {
      // Show error without technical prefix
      std::cout << "\n❌ " << event.data << std::endl;
    }
    // Remove other event types display - keep it clean
  }

  void ReplaceAll(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length();
    }
  }
};

}  // namespace sse_ai::demo

int Run() {
  sse_ai::demo::SseAiClient client;

  bool success = true;

  // Test single question
  if (!client.TestSseAiChat()) {
    success = false;
  }

  // Test multiple questions
  if (!client.TestMultipleQuestions()) {
    success = false;
  }

  if (success) {
    std::cout << "\n🎉 All SSE AI tests completed successfully!" << std::endl;
    return 0;
  } else {
    std::cout << "\n💥 Some SSE AI tests failed!" << std::endl;
    return -1;
  }
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);

  // Initialize tRPC runtime
  int ret = ::trpc::TrpcConfig::GetInstance()->Init(FLAGS_client_config);
  if (ret != 0) {
    std::cerr << "Failed to init config, ret: " << ret << std::endl;
    return -1;
  }

  // Register HTTP SSE codec
  sse_ai::demo::RegisterHttpSseCodec();

  return ::trpc::RunInTrpcRuntime([]() {
    return Run();
  });
}