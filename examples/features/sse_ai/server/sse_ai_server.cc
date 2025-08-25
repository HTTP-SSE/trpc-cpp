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

#include <atomic>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "trpc/common/trpc_app.h"
#include "trpc/server/http_service.h"
#include "trpc/util/http/function_handlers.h"
#include "trpc/util/http/http_handler.h"
#include "trpc/util/http/routes.h"
#include "trpc/util/http/sse/sse_event.h"
#include "trpc/util/log/logging.h"

namespace sse_ai::demo {

// AI Chat Handler using Server-Sent Events
class AiChatHandler : public ::trpc::http::HttpHandler {
 public:
  AiChatHandler() {}

  ::trpc::Status Handle(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req,
                        ::trpc::http::Response* rsp) override {
    // Extract question from query parameters
    std::string question = GetQuestionFromRequest(req);
    if (question.empty()) {
      question = "Hello! How can I help you today?";
    }

    // Simple request log
    std::cout << "📥 Received: " << question << std::endl;

    // Check if this is an SSE request
    std::string accept = req->GetHeader("Accept");
    if (accept.find("text/event-stream") == std::string::npos) {
      rsp->SetStatus(::trpc::http::ResponseStatus::kBadRequest);
      rsp->SetContent("This endpoint requires SSE. Please set Accept: text/event-stream");
      return ::trpc::kSuccStatus;
    }

    // Set proper SSE response headers
    rsp->SetHeader("Content-Type", "text/event-stream");
    rsp->SetHeader("Cache-Control", "no-cache");
    rsp->SetHeader("Connection", "keep-alive");
    rsp->SetHeader("Access-Control-Allow-Origin", "*");
    rsp->SetHeader("Access-Control-Allow-Headers", "Cache-Control");
    rsp->SetStatus(::trpc::http::ResponseStatus::kOk);

    // Generate and stream AI response directly
    std::string sse_response = GenerateAiResponseSSE(question);
    rsp->SetContent(sse_response);

    // Simple response log
    std::cout << "📤 Sent response (" << sse_response.length() << " bytes)" << std::endl;
    return ::trpc::kSuccStatus;
  }

 private:
  std::string GetQuestionFromRequest(const ::trpc::http::RequestPtr& req) {
    // Try to get question from query parameter first
    std::string url = req->GetUrl();
    size_t question_pos = url.find("question=");
    if (question_pos != std::string::npos) {
      size_t start = question_pos + 9; // length of "question="
      size_t end = url.find("&", start);
      if (end == std::string::npos) end = url.length();
      std::string encoded_question = url.substr(start, end - start);
      // Simple URL decode for %20 -> space, %21 -> !, etc.
      return DecodeUrl(encoded_question);
    }

    // Try to get from POST body
    if (req->GetMethodType() == ::trpc::http::MethodType::POST) {
      return req->GetContent();
    }

    return "";
  }

  std::string DecodeUrl(const std::string& encoded) {
    std::string decoded;
    for (size_t i = 0; i < encoded.length(); ++i) {
      if (encoded[i] == '%' && i + 2 < encoded.length()) {
        int hex = std::stoi(encoded.substr(i + 1, 2), nullptr, 16);
        decoded += static_cast<char>(hex);
        i += 2;
      } else if (encoded[i] == '+') {
        decoded += ' ';
      } else {
        decoded += encoded[i];
      }
    }
    return decoded;
  }

  std::string GenerateAiResponseSSE(const std::string& question) {
    std::string sse_content;
    
    // Send start event
    ::trpc::http::sse::SseEvent start_event;
    start_event.event_type = "ai_start";
    start_event.data = "Starting AI response for: " + question;
    start_event.id = "start";
    sse_content += start_event.ToString();

    // Generate response chunks
    std::vector<std::string> response_chunks = GenerateAiResponse(question);
    
    for (size_t i = 0; i < response_chunks.size(); ++i) {
      ::trpc::http::sse::SseEvent chunk_event;
      chunk_event.event_type = "ai_chunk";
      chunk_event.data = response_chunks[i];
      chunk_event.id = "chunk_" + std::to_string(i);
      sse_content += chunk_event.ToString();
    }

    // Send completion event
    ::trpc::http::sse::SseEvent end_event;
    end_event.event_type = "ai_complete";
    end_event.data = "Response completed";
    end_event.id = "end";
    sse_content += end_event.ToString();

    return sse_content;
  }

  std::vector<std::string> GenerateAiResponse(const std::string& question) {
    // Simulate different AI responses based on the question
    if (question.find("weather") != std::string::npos || question.find("Weather") != std::string::npos) {
      return {
        "Looking up current weather information...",
        "Based on the latest data, ",
        "today's weather is partly cloudy with ",
        "a temperature of 22°C (72°F). ",
        "There's a light breeze from the southwest ",
        "and the humidity is around 65%. ",
        "Perfect weather for outdoor activities!"
      };
    } else if (question.find("code") != std::string::npos || question.find("programming") != std::string::npos) {
      return {
        "Great question about programming! ",
        "Here's a simple example to get you started:\\n\\n",
        "```cpp\\n",
        "#include <iostream>\\n",
        "int main() {\\n",
        "    std::cout << \"Hello, World!\" << std::endl;\\n",
        "    return 0;\\n",
        "}\\n",
        "```\\n\\n",
        "This basic C++ program demonstrates ",
        "the fundamental structure of a C++ application. ",
        "Would you like me to explain any specific part?"
      };
    } else if (question.find("trpc") != std::string::npos || question.find("tRPC") != std::string::npos) {
      return {
        "tRPC-Cpp is a high-performance RPC framework! ",
        "Here are some key features:\\n\\n",
        "🚀 High Performance: Built for speed and efficiency\\n",
        "🔧 Multiple Protocols: Supports HTTP, gRPC, and custom protocols\\n",
        "🌐 Server-Sent Events: Real-time streaming like this example\\n",
        "⚙️ Flexible Configuration: Easy to configure and extend\\n",
        "🔗 Service Mesh Ready: Built for modern microservices\\n\\n",
        "This SSE example demonstrates how tRPC-Cpp ",
        "can handle real-time streaming for AI applications!"
      };
    } else {
      return {
        "Thank you for your question: \"" + question + "\"\\n\\n",
        "I'm an AI assistant powered by tRPC-Cpp's ",
        "Server-Sent Events implementation. ",
        "This streaming response demonstrates how ",
        "real-time AI interactions can be built ",
        "using the tRPC framework. ",
        "Each chunk you see is being streamed ",
        "individually, simulating how modern AI ",
        "chat systems work. ",
        "You can ask me about weather, programming, ",
        "or tRPC-Cpp itself for more specialized responses!"
      };
    }
  }
};

// Health check handler
class HealthHandler : public ::trpc::http::HttpHandler {
 public:
  ::trpc::Status Handle(const ::trpc::ServerContextPtr& ctx, const ::trpc::http::RequestPtr& req,
                        ::trpc::http::Response* rsp) override {
    rsp->SetContent("{\"status\": \"healthy\", \"service\": \"sse_ai_server\"}");
    rsp->SetHeader("Content-Type", "application/json");
    return ::trpc::kSuccStatus;
  }
};

void SetHttpRoutes(::trpc::http::HttpRoutes& r) {
  auto ai_chat_handler = std::make_shared<AiChatHandler>();
  auto health_handler = std::make_shared<HealthHandler>();

  // SSE AI Chat endpoint
  r.Add(::trpc::http::MethodType::GET, ::trpc::http::Path("/ai/chat"), ai_chat_handler);
  r.Add(::trpc::http::MethodType::POST, ::trpc::http::Path("/ai/chat"), ai_chat_handler);
  
  // Health check endpoint
  r.Add(::trpc::http::MethodType::GET, ::trpc::http::Path("/health"), health_handler);
}

class SseAiServer : public ::trpc::TrpcApp {
 public:
  int Initialize() override {
    auto http_service = std::make_shared<::trpc::HttpService>();
    http_service->SetRoutes(SetHttpRoutes);

    RegisterService("sse_ai_service", http_service);

    std::cout << "🚀 SSE AI Server started" << std::endl;
    return 0;
  }

  void Destroy() override {
    std::cout << "📴 SSE AI Server stopped" << std::endl;
  }
};

}  // namespace sse_ai::demo

int main(int argc, char** argv) {
  sse_ai::demo::SseAiServer server;

  server.Main(argc, argv);
  server.Wait();

  return 0;
}