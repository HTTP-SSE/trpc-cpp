#include <iostream>
#include <string>
#include <map>
#include <thread>
#include <atomic>
#include <mutex>
#include <iomanip>
#include <chrono>
#include <regex>
#include <ctime>
#include <sstream>
#include <fstream>
#include <future>
#include <algorithm>

#include "trpc/client/http/http_service_proxy.h"
#include "trpc/client/make_client_context.h"
#include "trpc/client/trpc_client.h"
#include "trpc/client/client_context.h"
#include "trpc/client/service_proxy_option.h"
#include "trpc/common/config/trpc_config.h"
#include "trpc/codec/codec_manager.h"
#include "trpc/runtime/runtime.h"
#include "trpc/common/runtime_manager.h"
#include "trpc/naming/trpc_naming_registry.h"
#include "trpc/util/buffer/noncontiguous_buffer.h"
#include "trpc/common/status.h"
#include "trpc/common/trpc_app.h"
#include "trpc/util/http/sse/sse_parser.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace trpc::http;

// Stock information structure
struct StockInfo {
    double price = 0.0;
    double change = 0.0;
    double change_percent = 0.0;
    std::string timestamp;
};

// tRPC HTTP-SSE Client for stock price monitoring
class TrpcSseClient {
private:
    std::map<std::string, StockInfo> stock_prices_;
    mutable std::mutex stock_mutex_;
    std::atomic<bool> running_{false};
    std::atomic<bool> connected_{false};
    std::atomic<bool> connection_attempted_{false};
    std::thread listener_thread_;
    
    // tRPC HTTP service proxy for configuration
    std::shared_ptr<HttpServiceProxy> http_proxy_;
    
    // Direct socket connection for SSE
    int sock_fd_{-1};
    std::string server_host_{"127.0.0.1"};
    int server_port_{8080};
    
    // Buffer for accumulating SSE data
    std::string sse_buffer_;
    bool headers_processed_{false};

public:
    TrpcSseClient() = default;
    
    ~TrpcSseClient() {
        Stop();
    }

    bool Connect() {
        // Only attempt connection once
        if (connection_attempted_.load()) {
            return connected_.load();
        }
        
        connection_attempted_.store(true);
        
        try {
            std::cout << "Connecting to server..." << std::endl;
            
            // Get tRPC HTTP service proxy for configuration
            http_proxy_ = trpc::GetTrpcClient()->GetProxy<HttpServiceProxy>("stock_price_sse_service");
            if (!http_proxy_) {
                std::cerr << "Failed to get tRPC HTTP service proxy" << std::endl;
                return false;
            }
            
            // Create socket connection for SSE
            sock_fd_ = socket(AF_INET, SOCK_STREAM, 0);
            if (sock_fd_ < 0) {
                std::cerr << "Failed to create socket" << std::endl;
                return false;
            }
            
            // Set up server address
            struct sockaddr_in server_addr;
            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(server_port_);
            server_addr.sin_addr.s_addr = inet_addr(server_host_.c_str());
            
            // Connect to server
            if (connect(sock_fd_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                std::cerr << "Failed to connect to server" << std::endl;
                close(sock_fd_);
                sock_fd_ = -1;
                return false;
            }
            
            // Send HTTP GET request for SSE
            std::string request = 
                "GET /sse HTTP/1.1\r\n"
                "Host: " + server_host_ + ":" + std::to_string(server_port_) + "\r\n"
                "Accept: text/event-stream\r\n"
                "Cache-Control: no-cache\r\n"
                "Connection: keep-alive\r\n"
                "\r\n";
            
            if (send(sock_fd_, request.c_str(), request.length(), 0) < 0) {
                std::cerr << "Failed to send HTTP request" << std::endl;
                close(sock_fd_);
                sock_fd_ = -1;
                return false;
            }
            
            std::cout << "Successfully connected to SSE server!" << std::endl;
            connected_ = true;
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Connect error: " << e.what() << std::endl;
            return false;
        }
    }

    void StartListening() {
        if (!connected_) {
            return;
        }

        running_ = true;
        
        // Start listening for events in a separate thread
        listener_thread_ = std::thread([this]() {
            this->ListenForEvents();
        });
    }

    void Stop() {
        running_ = false;
        connected_ = false;
        
        if (listener_thread_.joinable()) {
            listener_thread_.join();
        }
        
        // Close the socket
        if (sock_fd_ >= 0) {
            close(sock_fd_);
            sock_fd_ = -1;
        }
        
        ClearStockData();
    }

private:
    void ListenForEvents() {
                    std::cout << "Starting to listen for SSE events..." << std::endl;
        
        char buffer[4096];
        
        while (running_ && connected_) {
            try {
                // Read from socket
                ssize_t bytes_read = recv(sock_fd_, buffer, sizeof(buffer) - 1, 0);
                
                if (bytes_read <= 0) {
                    if (bytes_read == 0) {
                        std::cout << "Server closed connection" << std::endl;
                    } else {
                        std::cerr << "Socket read error: " << strerror(errno) << std::endl;
                    }
                    connected_ = false;
                    break;
                }
                
                // Null-terminate the buffer
                buffer[bytes_read] = '\0';
                
                // Accumulate data in buffer
                sse_buffer_ += std::string(buffer, bytes_read);
                
                // Process complete SSE events
                ProcessSseBuffer();
                
            } catch (const std::exception& e) {
                std::cerr << "ListenForEvents error: " << e.what() << std::endl;
                connected_ = false;
                break;
            }
        }
        
        std::cout << "Stopped listening for SSE events" << std::endl;
    }

    void ProcessSseBuffer() {
        // Simple approach: just look for JSON data in the stream
        std::string buffer_copy = sse_buffer_;
        
        // Skip HTTP headers if not already processed
        if (!headers_processed_) {
            size_t header_end = buffer_copy.find("\r\n\r\n");
            if (header_end != std::string::npos) {
                headers_processed_ = true;
                buffer_copy = buffer_copy.substr(header_end + 4);
            } else {
                // Headers not complete yet, wait for more data
                return;
            }
        }
        
        // Process all JSON objects in the buffer
        size_t processed_pos = 0;
        size_t json_start = 0;
        
        while ((json_start = buffer_copy.find("{\"symbol\"", processed_pos)) != std::string::npos) {
            size_t json_end = buffer_copy.find("}", json_start);
            if (json_end != std::string::npos) {
                std::string json_data = buffer_copy.substr(json_start, json_end - json_start + 1);
                
                // Process the JSON data
                ProcessStockUpdate(json_data);
                
                // Move to next position
                processed_pos = json_end + 1;
            } else {
                // Incomplete JSON, wait for more data
                break;
            }
        }
        
        // Remove processed data from buffer
        if (processed_pos > 0) {
            if (processed_pos < sse_buffer_.length()) {
                sse_buffer_ = sse_buffer_.substr(processed_pos);
            } else {
                sse_buffer_.clear();
            }
        }
    }

    void ProcessStockUpdate(const std::string& json_data) {
        // Parse JSON data using regex (could be improved with proper JSON parser)
        std::regex symbol_regex("\"symbol\"\\s*:\\s*\"([^\"]+)\"");
        std::regex price_regex("\"price\"\\s*:\\s*([0-9]+\\.?[0-9]*)");
        std::regex change_regex("\"change\"\\s*:\\s*([+-]?[0-9]+\\.?[0-9]*)");
        std::regex change_percent_regex("\"change_percent\"\\s*:\\s*([+-]?[0-9]+\\.?[0-9]*)");
        std::regex timestamp_regex("\"timestamp\"\\s*:\\s*\"([^\"]+)\"");
        
        std::smatch matches;
        StockInfo stock;
        bool valid_data = false;
        std::string symbol;
        
        if (std::regex_search(json_data, matches, symbol_regex) && matches.size() > 1) {
            symbol = matches[1].str();
            
            if (std::regex_search(json_data, matches, price_regex) && matches.size() > 1) {
                stock.price = std::stod(matches[1].str());
                valid_data = true;
            }
            
            if (std::regex_search(json_data, matches, change_regex) && matches.size() > 1) {
                stock.change = std::stod(matches[1].str());
            }
            
            if (std::regex_search(json_data, matches, change_percent_regex) && matches.size() > 1) {
                stock.change_percent = std::stod(matches[1].str());
            }
            
            if (std::regex_search(json_data, matches, timestamp_regex) && matches.size() > 1) {
                stock.timestamp = matches[1].str();
            }
            
            if (valid_data) {
                // Update stock data in memory
                {
                    std::lock_guard<std::mutex> lock(stock_mutex_);
                    stock_prices_[symbol] = stock;
                }
            }
        }
    }

    void ClearStockData() {
        std::lock_guard<std::mutex> lock(stock_mutex_);
        stock_prices_.clear();
    }

public:
    std::map<std::string, StockInfo> GetStockPrices() const {
        std::lock_guard<std::mutex> lock(stock_mutex_);
        return stock_prices_;
    }

    std::string GetCurrentTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        return std::ctime(&time_t);
    }

    void DisplayStockPrices() {
        while (running_) {
            // Clear screen
            system("clear");

            std::cout << "=== Real-Time Stock Prices ===" << std::endl;
            std::cout << "Last Updated: " << GetCurrentTimestamp();
            std::cout << "Status: " << (connected_ ? "Connected" : "Disconnected") << std::endl;
            std::cout << "Press Ctrl+C to exit\n" << std::endl;

            std::cout << std::left << std::setw(8) << "Symbol" 
                      << std::setw(12) << "Price" 
                      << std::setw(12) << "Change" 
                      << std::setw(15) << "Change %" << std::endl;
            std::cout << std::string(50, '-') << std::endl;

            auto stocks = GetStockPrices();
            if (stocks.empty()) {
                if (connected_) {
                    std::cout << "Waiting for stock data..." << std::endl;
                } else {
                    std::cout << "No data available (server disconnected)" << std::endl;
                }
            } else {
                for (const auto& stock : stocks) {
                    std::cout << std::left << std::setw(8) << stock.first;
                    
                    if (stock.second.price > 0) {
                        std::cout << std::setw(12) << std::fixed << std::setprecision(2) << stock.second.price;
                        std::cout << std::setw(12) << std::fixed << std::setprecision(2) 
                                  << (stock.second.change >= 0 ? "+" : "") << stock.second.change;
                        std::cout << std::setw(15) << std::fixed << std::setprecision(2) 
                                  << (stock.second.change_percent >= 0 ? "+" : "") 
                                  << stock.second.change_percent << "%";
                    } else {
                        std::cout << std::setw(12) << "N/A";
                        std::cout << std::setw(12) << "N/A";
                        std::cout << std::setw(15) << "N/A";
                    }
                    std::cout << std::endl;
                }
            }

            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
};

// Client business logic function
int Run() {
    try {
        // Create a simple client instance
        TrpcSseClient client;
        
        std::cout << "Attempting to connect to server..." << std::endl;
        
        if (!client.Connect()) {
            std::cerr << "Failed to connect to server" << std::endl;
            return 1;
        } else {
            std::cout << "Connection successful!" << std::endl;
            // Start listening and display stock prices
            client.StartListening();
            client.DisplayStockPrices();
        }
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Client error: " << e.what() << std::endl;
        return 1;
    }
}

// Main client application
int main(int argc, char** argv) {
    std::cout << "=== Stock Price SSE Client ===" << std::endl;

    // Initialize tRPC systems
    if (!trpc::codec::Init()) {
        std::cerr << "Failed to initialize codec manager" << std::endl;
        return 1;
    }
    
    if (!trpc::naming::Init()) {
        std::cerr << "Failed to initialize naming system" << std::endl;
        return 1;
    }

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
    
    std::cout << "[Client] Using config file: " << config_path << std::endl;
    
    // Initialize tRPC configuration
    if (trpc::TrpcConfig::GetInstance()->Init(config_path) != 0) {
        std::cerr << "Failed to initialize tRPC configuration" << std::endl;
        return 1;
    }

    // Run the client in tRPC runtime context
    return trpc::RunInTrpcRuntime([]() { return Run(); });
}
