#include <mcp/server_streaming.hpp>
#include <mcp/transport/transport.hpp>
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>

/**
 * @file streaming_server_example.cpp
 * @brief Demonstrates StreamingServer using Generator<T> for incremental results
 * 
 * This example shows how to use the streaming server API for large datasets,
 * file streaming, and progress reporting.
 * 
 * Compile:
 *   g++ -std=c++23 -I../include streaming_server_example.cpp -o streaming_server
 */

using namespace pooriayousefi::mcp;
using json = nlohmann::json;

// Example: Stream large dataset incrementally
core::Generator<json> stream_large_dataset(const json& params) {
    int count = params.value("count", 100);
    int delay_ms = params.value("delay_ms", 10);
    
    for (int i = 0; i < count; ++i) {
        // Simulate processing delay
        if (delay_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
        
        // Yield each item incrementally
        co_yield json{
            {"index", i},
            {"value", i * i},
            {"timestamp", std::chrono::system_clock::now().time_since_epoch().count()}
        };
        
        // Check for cancellation
        if (jsonrpc::is_canceled()) {
            std::cout << "[Server] Dataset streaming cancelled at item " << i << "\n";
            co_return;
        }
    }
    
    std::cout << "[Server] Streamed " << count << " items successfully\n";
}

// Example: Stream file lines without loading entire file
core::Generator<json> stream_log_file(const json& params) {
    std::string filename = params.value("filename", "server.log");
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "[Server] Failed to open file: " << filename << "\n";
        co_return;
    }
    
    std::string line;
    int line_number = 0;
    
    while (std::getline(file, line)) {
        co_yield json{
            {"line_number", ++line_number},
            {"content", line}
        };
        
        if (jsonrpc::is_canceled()) {
            std::cout << "[Server] File streaming cancelled at line " << line_number << "\n";
            co_return;
        }
    }
    
    std::cout << "[Server] Streamed " << line_number << " lines from " << filename << "\n";
}

// Example: Paginated API simulation with streaming
core::Generator<json> stream_paginated_api(const json& params) {
    int total_pages = params.value("pages", 10);
    int items_per_page = params.value("items_per_page", 20);
    
    for (int page = 1; page <= total_pages; ++page) {
        // Simulate API call delay
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        // Yield page metadata
        co_yield json{
            {"page", page},
            {"total_pages", total_pages},
            {"items_count", items_per_page}
        };
        
        // Yield items from this page
        for (int item = 0; item < items_per_page; ++item) {
            co_yield json{
                {"page", page},
                {"item_index", item},
                {"item_id", (page - 1) * items_per_page + item},
                {"data", "Item data for page " + std::to_string(page)}
            };
        }
        
        if (jsonrpc::is_canceled()) {
            std::cout << "[Server] API streaming cancelled at page " << page << "\n";
            co_return;
        }
    }
    
    std::cout << "[Server] Streamed " << total_pages << " pages successfully\n";
}

// Example: Real-time sensor data simulation
core::Generator<json> stream_sensor_data(const json& params) {
    int duration_seconds = params.value("duration", 10);
    int frequency_hz = params.value("frequency", 10);
    
    auto start_time = std::chrono::steady_clock::now();
    auto interval = std::chrono::milliseconds(1000 / frequency_hz);
    
    int sample_count = 0;
    
    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        
        if (elapsed >= duration_seconds) {
            break;
        }
        
        // Simulate sensor reading
        co_yield json{
            {"sample", ++sample_count},
            {"timestamp", elapsed},
            {"temperature", 20.0 + (std::rand() % 100) / 10.0},
            {"humidity", 40.0 + (std::rand() % 400) / 10.0},
            {"pressure", 1013.0 + (std::rand() % 100) / 10.0}
        };
        
        std::this_thread::sleep_for(interval);
        
        if (jsonrpc::is_canceled()) {
            std::cout << "[Server] Sensor streaming cancelled after " << sample_count << " samples\n";
            co_return;
        }
    }
    
    std::cout << "[Server] Streamed " << sample_count << " sensor samples\n";
}

int main() {
    try {
        // Create stdio transport
        auto transport = std::make_shared<StdioTransport>();
        
        // Create server implementation info
        Implementation impl{
            "streaming-server-example",
            "1.0.0"
        };
        
        // Create streaming server
        StreamingServer server(transport, impl);
        
        std::cout << "=== MCP Streaming Server Example ===\n\n";
        
        // Register streaming tools
        std::cout << "Registering streaming tools...\n";
        
        // Tool 1: Stream large dataset
        server.register_streaming_tool(
            Tool{
                "stream_dataset",
                "Stream large dataset incrementally",
                {
                    {"type", "object"},
                    {"properties", {
                        {"count", {{"type", "integer"}, {"description", "Number of items"}}},
                        {"delay_ms", {{"type", "integer"}, {"description", "Delay between items"}}}
                    }}
                }
            },
            stream_large_dataset
        );
        std::cout << "  - stream_dataset: Stream large datasets without memory bloat\n";
        
        // Tool 2: Stream log file
        server.register_streaming_tool(
            Tool{
                "stream_logs",
                "Stream log file lines incrementally",
                {
                    {"type", "object"},
                    {"properties", {
                        {"filename", {{"type", "string"}, {"description", "Log file path"}}}
                    }}
                }
            },
            stream_log_file
        );
        std::cout << "  - stream_logs: Read large files without loading into memory\n";
        
        // Tool 3: Stream paginated API
        server.register_streaming_tool(
            Tool{
                "stream_api",
                "Stream paginated API results",
                {
                    {"type", "object"},
                    {"properties", {
                        {"pages", {{"type", "integer"}, {"description", "Total pages"}}},
                        {"items_per_page", {{"type", "integer"}, {"description", "Items per page"}}}
                    }}
                }
            },
            stream_paginated_api
        );
        std::cout << "  - stream_api: Fetch paginated API incrementally\n";
        
        // Tool 4: Stream sensor data
        server.register_streaming_tool(
            Tool{
                "stream_sensors",
                "Stream real-time sensor data",
                {
                    {"type", "object"},
                    {"properties", {
                        {"duration", {{"type", "integer"}, {"description", "Duration in seconds"}}},
                        {"frequency", {{"type", "integer"}, {"description", "Samples per second"}}}
                    }}
                }
            },
            stream_sensor_data
        );
        std::cout << "  - stream_sensors: Real-time sensor data streaming\n\n";
        
        // Register streaming resources
        std::cout << "Registering streaming resources...\n";
        
        Resource config_resource{
            "config://settings.json",
            "Server Settings",
            "Streaming server configuration",
            "application/json"
        };
        
        server.register_streaming_resource(
            config_resource,
            [](const std::string& uri) -> core::Generator<ResourceContent> {
                // Stream configuration in chunks
                co_yield ResourceContent{
                    uri,
                    "application/json",
                    R"({"server": {"name": "streaming-example",)",
                    std::nullopt
                };
                
                co_yield ResourceContent{
                    uri,
                    "application/json",
                    R"("version": "1.0.0",)",
                    std::nullopt
                };
                
                co_yield ResourceContent{
                    uri,
                    "application/json",
                    R"("features": ["streaming", "async", "cancellation"]}})",
                    std::nullopt
                };
                
                std::cout << "[Server] Streamed config resource\n";
            }
        );
        std::cout << "  - config://settings.json: Server configuration\n\n";
        
        // Enable capabilities
        server.enable_tools();
        server.enable_resources();
        
        std::cout << "Server is ready! Capabilities:\n";
        std::cout << "  ✓ 4 streaming tools registered\n";
        std::cout << "  ✓ 1 streaming resource registered\n";
        std::cout << "  ✓ Progress reporting enabled\n";
        std::cout << "  ✓ Cancellation support enabled\n";
        std::cout << "\nListening for requests...\n\n";
        
        // Start message loop
        while (true) {
            try {
                auto request = transport->receive();
                auto response = server.dispatch(request);
                transport->send(response);
            } catch (const std::exception& e) {
                std::cerr << "[Server] Error: " << e.what() << "\n";
                break;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

/**
 * @section Key Features Demonstrated
 * 
 * 1. **Streaming Tools**: Tools that yield results incrementally using Generator<T>
 * 2. **Memory Efficiency**: Process large datasets without loading into memory
 * 3. **Progress Reporting**: Automatic progress updates during streaming
 * 4. **Cancellation**: Support for cancelling long-running streams
 * 5. **Real-time Data**: Stream live sensor data or API results
 * 
 * @section Use Cases
 * 
 * - **Large Files**: Stream log files, CSV data, JSON arrays
 * - **Database Queries**: Stream result sets without memory bloat
 * - **API Pagination**: Fetch and stream paginated endpoints
 * - **Real-time Sensors**: Stream live data feeds
 * - **Long Computations**: Stream intermediate results
 * 
 * @section Performance Benefits
 * 
 * - **Low Memory**: O(1) memory usage regardless of dataset size
 * - **Fast Start**: First results available immediately
 * - **Responsive**: Client can cancel anytime
 * - **Efficient**: No buffering, true streaming
 * 
 * @section Testing Commands
 * 
 * ```bash
 * # Test with echo client
 * ./streaming_server | mcp-client
 * 
 * # Call streaming tool
 * {"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"stream_dataset","arguments":{"count":1000,"delay_ms":5}}}
 * 
 * # Cancel during streaming
 * {"jsonrpc":"2.0","method":"$/cancelRequest","params":{"requestId":1}}
 * ```
 */
