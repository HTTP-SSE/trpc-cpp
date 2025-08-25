#include <iostream>
#include <string>
#include <map>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <random>
#include <sstream>
#include <iomanip>

#include "trpc/server/trpc_server.h"
#include "trpc/server/http_service.h"
#include "trpc/server/http_sse/HttpSseService.h"
#include "trpc/common/trpc_app.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/codec/codec_manager.h"
#include "trpc/runtime/runtime.h"
#include "trpc/naming/trpc_naming_registry.h"
#include "trpc/util/http/http_handler.h"
#include "trpc/util/http/routes.h"
#include "trpc/util/http/sse/sse_parser.h"
#include "trpc/util/http/sse/sse_event.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/common/status.h"
#include "trpc/log/logging.h"

using namespace trpc;

// Stock information structure
struct StockInfo {
    double price = 0.0;
    double change = 0.0;
    double change_percent = 0.0;
    std::string timestamp;
};

// Thread-safe stock price simulator
class StockPriceSimulator {
public:
    StockPriceSimulator() : running_(false) {
        // Initialize with realistic stock prices
        stocks_ = {
            {"AAPL", {150.0, 0.0, 0.0}},
            {"AMZN", {3500.0, 0.0, 0.0}},
            {"GOOGL", {2200.0, 0.0, 0.0}},
            {"MSFT", {160.0, 0.0, 0.0}},
            {"TSLA", {260.0, 0.0, 0.0}},
        };
    }

    ~StockPriceSimulator() {
        Stop();
    }

    void Start() {
        if (!running_) {
            running_ = true;
            simulator_thread_ = std::thread([this]() { Run(); });
        }
    }

    void Stop() {
        running_ = false;
        if (simulator_thread_.joinable()) {
            simulator_thread_.join();
        }
    }

    std::map<std::string, StockInfo> GetStocks() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return stocks_;
    }

private:
    void Run() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<> price_change(-5.0, 5.0);
        std::uniform_real_distribution<> percent_change(-3.0, 3.0);

        while (running_) {
            UpdateStocks(gen, price_change, percent_change);
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }

    void UpdateStocks(std::mt19937& gen, std::uniform_real_distribution<>& price_change, 
                     std::uniform_real_distribution<>& percent_change) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        for (auto& [symbol, stock] : stocks_) {
            double old_price = stock.price;
            double change = price_change(gen);
            double percent = percent_change(gen);
            
            stock.price = std::max(1.0, old_price + change);
            stock.change = stock.price - old_price;
            stock.change_percent = percent;
            
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            stock.timestamp = std::ctime(&time_t);
            
            // Remove newline from timestamp
            if (!stock.timestamp.empty() && stock.timestamp.back() == '\n') {
                stock.timestamp.pop_back();
            }
            
            TRPC_LOG_INFO("[StockSimulator] Updated " << symbol << ": $" 
                         << std::fixed << std::setprecision(2) << stock.price
                         << " (" << (stock.change >= 0 ? "+" : "") << stock.change 
                         << ", " << (stock.change_percent >= 0 ? "+" : "") 
                         << stock.change_percent << "%)");
        }
    }

    std::map<std::string, StockInfo> stocks_;
    std::atomic<bool> running_;
    std::thread simulator_thread_;
    mutable std::mutex mutex_;
};

// HTTP handler for welcome page
class WelcomeHandler : public http::HttpHandler {
public:
    Status Get(const ServerContextPtr& ctx, const http::RequestPtr& req, http::Response* resp) override {
        std::string html = R"(
<!DOCTYPE html>
<html>
<head>
    <title>Stock Price SSE Server</title>
    <style>
        body { font-family: Arial, sans-serif; margin: 40px; }
        .container { max-width: 800px; margin: 0 auto; }
        .header { background: #f0f0f0; padding: 20px; border-radius: 5px; }
        .endpoint { background: #e8f4f8; padding: 15px; margin: 10px 0; border-radius: 3px; }
        code { background: #f5f5f5; padding: 2px 4px; border-radius: 2px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>📈 Stock Price SSE Server</h1>
            <p>Real-time stock price monitoring using Server-Sent Events (SSE)</p>
        </div>
        
        <h2>Available Endpoints:</h2>
        
        <div class="endpoint">
            <h3>🎯 SSE Stream</h3>
            <p><code>GET /sse</code> - Real-time stock price updates</p>
            <p>Connect to this endpoint to receive live stock price updates via SSE.</p>
        </div>
        
        <div class="endpoint">
            <h3>📊 JSON Data</h3>
            <p><code>GET /stocks</code> - Current stock data in JSON format</p>
            <p>Get the current state of all stocks in JSON format.</p>
        </div>
        
        <h2>Supported Stocks:</h2>
        <ul>
            <li><strong>AAPL</strong> - Apple Inc.</li>
            <li><strong>AMZN</strong> - Amazon.com Inc.</li>
            <li><strong>GOOGL</strong> - Alphabet Inc.</li>
            <li><strong>MSFT</strong> - Microsoft Corporation</li>
            <li><strong>TSLA</strong> - Tesla Inc.</li>
        </ul>
        
        <h2>SSE Event Format:</h2>
        <pre><code>event: stock_update
data: {"symbol":"AAPL","price":150.25,"change":1.25,"change_percent":0.84,"timestamp":"..."}
id: 1</code></pre>
        
        <p><em>Stock prices update every 2 seconds with realistic price movements.</em></p>
    </div>
</body>
</html>
        )";
        
        resp->SetStatus(200);
        resp->SetHeader("Content-Type", "text/html");
        resp->SetHeader("Access-Control-Allow-Origin", "*");
        resp->SetContent(html);
        return kSuccStatus;
    }
};

// HTTP handler for JSON stock data
class StockDataHandler : public http::HttpHandler {
public:
    StockDataHandler(StockPriceSimulator* simulator) : simulator_(simulator) {}

    Status Get(const ServerContextPtr& ctx, const http::RequestPtr& req, http::Response* resp) override {
        auto stocks = simulator_->GetStocks();
        
        std::stringstream json;
        json << "{\n";
        json << "  \"timestamp\": \"" << GetCurrentTimestamp() << "\",\n";
        json << "  \"stocks\": [\n";
        
        bool first = true;
        for (const auto& [symbol, stock] : stocks) {
            if (!first) json << ",\n";
            json << "    {\n";
            json << "      \"symbol\": \"" << symbol << "\",\n";
            json << "      \"price\": " << std::fixed << std::setprecision(2) << stock.price << ",\n";
            json << "      \"change\": " << std::fixed << std::setprecision(2) << stock.change << ",\n";
            json << "      \"change_percent\": " << std::fixed << std::setprecision(2) << stock.change_percent << ",\n";
            json << "      \"timestamp\": \"" << stock.timestamp << "\"\n";
            json << "    }";
            first = false;
        }
        
        json << "\n  ]\n";
        json << "}";
        
        resp->SetStatus(200);
        resp->SetHeader("Content-Type", "application/json");
        resp->SetHeader("Access-Control-Allow-Origin", "*");
        resp->SetContent(json.str());
        return kSuccStatus;
    }

private:
    std::string GetCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::string timestamp = std::ctime(&time_t);
        if (!timestamp.empty() && timestamp.back() == '\n') {
            timestamp.pop_back();
        }
        return timestamp;
    }

    StockPriceSimulator* simulator_;
};

// HTTP handler for SSE endpoint
class StockPriceSseHandler : public http::HttpHandler {
public:
    StockPriceSseHandler(StockPriceSimulator* simulator, HttpSseService* sse_service) 
        : simulator_(simulator), sse_service_(sse_service) {}

    Status Get(const ServerContextPtr& ctx, const http::RequestPtr& req, http::Response* resp) override {
        // Use the HTTP-SSE service to handle the connection
        if (sse_service_->HandleSseRequest(ctx)) {
            uint64_t client_id = std::any_cast<uint64_t>(ctx->GetUserData());
            TRPC_LOG_INFO("[Server] SSE connection accepted, client_id=" << client_id);
            
            // Send initial welcome event
            http::sse::SseEvent welcome_event;
            welcome_event.event_type = "welcome";
            welcome_event.data = "Connected to Stock Price SSE Server";
            welcome_event.id = "welcome_1";
            
            sse_service_->SendToClient(client_id, welcome_event);
            
            TRPC_LOG_INFO("[Server] SSE connection established, starting continuous updates");
            return kSuccStatus;
        } else {
            TRPC_LOG_ERROR("[Server] Failed to accept SSE connection");
            return Status(static_cast<int>(codec::ServerRetCode::ENCODE_ERROR), 0, "Failed to establish SSE connection");
        }
    }

private:
    StockPriceSimulator* simulator_;
    HttpSseService* sse_service_;
};

// Set up HTTP routes
void SetHttpRoutes(http::HttpRoutes& routes, StockPriceSimulator* simulator, HttpSseService* sse_service) {
    auto sse_handler = std::make_shared<StockPriceSseHandler>(simulator, sse_service);
    auto welcome_handler = std::make_shared<WelcomeHandler>();
    auto stock_data_handler = std::make_shared<StockDataHandler>(simulator);
    
    routes.Add(http::MethodType::GET, http::Path("/sse"), sse_handler);
    routes.Add(http::MethodType::GET, http::Path("/"), welcome_handler);
    routes.Add(http::MethodType::GET, http::Path("/stocks"), stock_data_handler);
}

// Main server application
class StockPriceServer : public TrpcApp {
public:
    int Initialize() override {
        // Start the stock price simulator
        simulator_.Start();
        
        // Create HTTP-SSE service
        sse_service_ = std::make_unique<HttpSseService>();
        
        auto http_service = std::make_shared<HttpService>();
        http_service->SetRoutes([this](http::HttpRoutes& routes) {
            SetHttpRoutes(routes, &simulator_, sse_service_.get());
        });

        RegisterService("stock_price_sse_service", http_service);
        
        // Start background thread for sending stock updates
        StartStockUpdateThread();
        
        return 0;
    }

    void Destroy() override {
        // Stop the background thread
        running_ = false;
        if (update_thread_.joinable()) {
            update_thread_.join();
        }
        
        // Shutdown SSE service
        if (sse_service_) {
            sse_service_->Shutdown();
        }
        
        simulator_.Stop();
    }

private:
    void StartStockUpdateThread() {
        running_ = true;
        update_thread_ = std::thread([this]() {
            int event_id = 1;
            
            while (running_) {
                // Get current stock data
                auto stocks = simulator_.GetStocks();
                
                // Send updates for each stock to all connected clients
                for (const auto& [symbol, stock] : stocks) {
                    std::stringstream json;
                    json << "{\"symbol\":\"" << symbol << "\",";
                    json << "\"price\":" << std::fixed << std::setprecision(2) << stock.price << ",";
                    json << "\"change\":" << std::fixed << std::setprecision(2) << stock.change << ",";
                    json << "\"change_percent\":" << std::fixed << std::setprecision(2) << stock.change_percent << ",";
                    json << "\"timestamp\":\"" << stock.timestamp << "\"}";
                    
                    // Create SSE event
                    http::sse::SseEvent stock_event;
                    stock_event.event_type = "stock_update";
                    stock_event.data = json.str();
                    stock_event.id = std::to_string(event_id++);
                    
                    // Broadcast to all connected clients
                    size_t sent_count = sse_service_->Broadcast(stock_event);
                    
                    TRPC_LOG_INFO("[Server] Sent update for " << symbol << ": $" 
                                 << std::fixed << std::setprecision(2) << stock.price
                                 << " (" << (stock.change >= 0 ? "+" : "") << stock.change 
                                 << ", " << (stock.change_percent >= 0 ? "+" : "") 
                                 << stock.change_percent << "%) to " << sent_count << " clients");
                }
                
                // Send keepalive event
                http::sse::SseEvent keepalive_event;
                keepalive_event.event_type = "keepalive";
                keepalive_event.data = "Server is alive";
                keepalive_event.id = "keepalive_" + std::to_string(event_id);
                
                sse_service_->Broadcast(keepalive_event);
                
                // Wait for next update
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        });
    }

    StockPriceSimulator simulator_;
    std::unique_ptr<HttpSseService> sse_service_;
    std::atomic<bool> running_{false};
    std::thread update_thread_;
};

int main() {
    std::cout << "=== Stock Price SSE Server (tRPC HTTP-SSE) ===" << std::endl;

    // Initialize tRPC systems
    if (!trpc::codec::Init()) {
        std::cerr << "Failed to initialize codec manager" << std::endl;
        return 1;
    }
    
    if (!trpc::naming::Init()) {
        std::cerr << "Failed to initialize naming system" << std::endl;
        return 1;
    }

    StockPriceServer server;
    
    // Try different config file paths
    std::vector<std::string> config_paths = {
        "examples/MonitorStockSse/trpc_merge.yaml",
        "trpc_merge.yaml",
        "./trpc_merge.yaml"
    };
    
    std::string config_path;
    for (const auto& path : config_paths) {
        std::ifstream file(path);
        if (file.good()) {
            config_path = path;
            break;
        }
    }
    
    if (config_path.empty()) {
        std::cerr << "Could not find config file trpc_merge.yaml" << std::endl;
        return 1;
    }
    
    std::cout << "[Server] Using config file: " << config_path << std::endl;
    
    // Run the server
    std::string config_arg = "--config=" + config_path;
    char* argv[] = {const_cast<char*>("stock_price_sse_server"), const_cast<char*>(config_arg.c_str())};
    int ret = server.Main(2, argv);
    if (ret != 0) {
        std::cerr << "Failed to start server" << std::endl;
        return 1;
    }

    std::cout << "[Server] Server started successfully!" << std::endl;
    std::cout << "[Server] SSE endpoint: http://localhost:8080/sse" << std::endl;
    std::cout << "[Server] Welcome page: http://localhost:8080/" << std::endl;
    std::cout << "[Server] Stock data: http://localhost:8080/stocks" << std::endl;
    std::cout << "[Server] Press Enter to stop..." << std::endl;
    
    std::cin.get();

    // Cleanup
    server.Destroy();

    std::cout << "[Server] Server stopped" << std::endl;
    return 0;
}
