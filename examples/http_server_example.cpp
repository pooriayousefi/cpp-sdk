#include <mcp/server.hpp>
#include <mcp/transport/http_transport.hpp>
#include <iostream>
#include <signal.h>

/**
 * @file http_server_example.cpp
 * @brief Demonstrates HTTP/SSE transport for MCP server
 * 
 * This example shows how to host an MCP server over HTTP with
 * Server-Sent Events for real-time notifications.
 * 
 * Compile:
 *   g++ -std=c++23 -I../include http_server_example.cpp -o http_server -pthread
 * 
 * Note: Requires cpp-httplib header (https://github.com/yhirose/cpp-httplib)
 */

using namespace pooriayousefi::mcp;
using namespace pooriayousefi::mcp::transport;
using json = nlohmann::json;

// Global server pointer for signal handler
HttpServerTransport* g_server_transport = nullptr;

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\n\nShutting down server...\n";
        if (g_server_transport) {
            g_server_transport->stop();
        }
        exit(0);
    }
}

int main() {
    try {
        std::cout << "=== MCP HTTP Server Example ===\n\n";
        
        // Create HTTP server transport
        int port = 8080;
        std::string host = "0.0.0.0";
        
        std::cout << "1. Creating HTTP server transport...\n";
        auto transport = std::make_shared<HttpServerTransport>(port, host);
        g_server_transport = transport.get();
        
        std::cout << "   Host: " << host << "\n";
        std::cout << "   Port: " << port << "\n";
        std::cout << "   Endpoints:\n";
        std::cout << "   - POST http://localhost:" << port << "/jsonrpc (JSON-RPC 2.0)\n";
        std::cout << "   - GET  http://localhost:" << port << "/events  (Server-Sent Events)\n";
        std::cout << "   - GET  http://localhost:" << port << "/health  (Health check)\n\n";
        
        // Create server implementation
        Implementation impl{
            "http-server-example",
            "1.0.0"
        };
        
        // Create MCP server
        std::cout << "2. Creating MCP server...\n";
        Server server(transport, impl);
        
        // Register tools
        std::cout << "3. Registering tools...\n";
        
        server.register_tool(
            Tool{
                "echo",
                "Echo back the provided message",
                {
                    {"type", "object"},
                    {"properties", {
                        {"message", {{"type", "string"}, {"description", "Message to echo"}}}
                    }},
                    {"required", json::array({"message"})}
                }
            },
            [transport](const json& params) -> json {
                std::string message = params.value("message", "");
                
                // Send SSE notification to all connected clients
                transport->send_sse_notification({
                    {"method", "notification/message"},
                    {"params", {
                        {"level", "info"},
                        {"message", "Echo tool called with: " + message}
                    }}
                });
                
                return json{{"echoed", message}};
            }
        );
        std::cout << "   - echo: Echo messages with SSE notification\n";
        
        server.register_tool(
            Tool{
                "get_time",
                "Get current server time",
                {{"type", "object"}}
            },
            [](const json&) -> json {
                auto now = std::chrono::system_clock::now();
                auto time_t = std::chrono::system_clock::to_time_t(now);
                
                return json{
                    {"timestamp", time_t},
                    {"time_string", std::ctime(&time_t)}
                };
            }
        );
        std::cout << "   - get_time: Return current server time\n";
        
        server.register_tool(
            Tool{
                "compute",
                "Perform a computation",
                {
                    {"type", "object"},
                    {"properties", {
                        {"operation", {{"type", "string"}, {"enum", json::array({"add", "multiply", "power"})}}},
                        {"a", {{"type", "number"}}},
                        {"b", {{"type", "number"}}}
                    }},
                    {"required", json::array({"operation", "a", "b"})}
                }
            },
            [](const json& params) -> json {
                std::string op = params.value("operation", "add");
                double a = params.value("a", 0.0);
                double b = params.value("b", 0.0);
                double result = 0.0;
                
                if (op == "add") {
                    result = a + b;
                } else if (op == "multiply") {
                    result = a * b;
                } else if (op == "power") {
                    result = std::pow(a, b);
                }
                
                return json{
                    {"operation", op},
                    {"a", a},
                    {"b", b},
                    {"result", result}
                };
            }
        );
        std::cout << "   - compute: Arithmetic operations\n\n";
        
        // Enable capabilities
        server.enable_tools();
        
        std::cout << "4. Server configuration:\n";
        std::cout << "   ✓ 3 tools registered\n";
        std::cout << "   ✓ HTTP/JSON-RPC transport\n";
        std::cout << "   ✓ Server-Sent Events for notifications\n";
        std::cout << "   ✓ Health check endpoint\n\n";
        
        // Setup signal handlers
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
        
        // Start HTTP server
        std::cout << "5. Starting HTTP server...\n\n";
        transport->start();
        
        std::cout << "╔════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  MCP HTTP Server Running on http://localhost:" << port << "    ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
        
        std::cout << "Test with curl:\n\n";
        
        std::cout << "# Initialize connection\n";
        std::cout << "curl -X POST http://localhost:" << port << "/jsonrpc \\\n";
        std::cout << "  -H 'Content-Type: application/json' \\\n";
        std::cout << "  -d '{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"initialize\",\"params\":{\"protocolVersion\":\"2024-11-05\",\"clientInfo\":{\"name\":\"curl-client\",\"version\":\"1.0.0\"}}}'\n\n";
        
        std::cout << "# List tools\n";
        std::cout << "curl -X POST http://localhost:" << port << "/jsonrpc \\\n";
        std::cout << "  -H 'Content-Type: application/json' \\\n";
        std::cout << "  -d '{\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"tools/list\"}'\n\n";
        
        std::cout << "# Call echo tool\n";
        std::cout << "curl -X POST http://localhost:" << port << "/jsonrpc \\\n";
        std::cout << "  -H 'Content-Type: application/json' \\\n";
        std::cout << "  -d '{\"jsonrpc\":\"2.0\",\"id\":3,\"method\":\"tools/call\",\"params\":{\"name\":\"echo\",\"arguments\":{\"message\":\"Hello from HTTP!\"}}}'\n\n";
        
        std::cout << "# Subscribe to SSE notifications\n";
        std::cout << "curl -N http://localhost:" << port << "/events\n\n";
        
        std::cout << "# Health check\n";
        std::cout << "curl http://localhost:" << port << "/health\n\n";
        
        std::cout << "Press Ctrl+C to stop the server.\n\n";
        
        // Message processing loop
        while (true) {
            try {
                // Receive request from HTTP transport
                auto request = transport->receive();
                
                // Log request
                std::cout << "[" << std::chrono::system_clock::now().time_since_epoch().count() << "] ";
                std::cout << "Request: " << request.value("method", "unknown") << "\n";
                
                // Dispatch to server
                auto response = server.dispatch(request);
                
                // Send response via HTTP transport
                transport->send(response);
                
                // Log response
                if (response.contains("error")) {
                    std::cout << "  Error: " << response["error"]["message"] << "\n";
                } else {
                    std::cout << "  Success\n";
                }
                
            } catch (const std::exception& e) {
                std::cerr << "Error processing request: " << e.what() << "\n";
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
 * 1. **HTTP Transport**: MCP over HTTP POST (JSON-RPC 2.0)
 * 2. **Server-Sent Events**: Real-time notifications to clients
 * 3. **Health Endpoint**: Monitoring and status checks
 * 4. **Cross-Origin**: Accessible from web browsers and clients
 * 5. **Concurrent**: Multi-threaded request handling
 * 
 * @section Architecture
 * 
 * ```
 * Client (curl/browser)
 *   |
 *   | HTTP POST /jsonrpc
 *   v
 * HttpServerTransport
 *   |
 *   | JSON-RPC 2.0
 *   v
 * MCP Server
 *   |
 *   | Tool Dispatch
 *   v
 * Tool Handlers
 *   |
 *   | Notifications
 *   v
 * SSE Stream (/events)
 *   |
 *   v
 * Connected Clients
 * ```
 * 
 * @section Endpoints
 * 
 * - **POST /jsonrpc**: JSON-RPC 2.0 endpoint for MCP protocol
 * - **GET /events**: Server-Sent Events stream for notifications
 * - **GET /health**: Health check (returns {"status":"ok"})
 * 
 * @section SSE Notifications
 * 
 * The server sends real-time notifications via SSE:
 * - Tool execution events
 * - Progress updates
 * - Server status changes
 * - Custom application events
 * 
 * @section Deployment
 * 
 * This can be deployed as:
 * - Standalone HTTP server
 * - Behind nginx/Apache reverse proxy
 * - Docker container
 * - Cloud service (AWS, GCP, Azure)
 * 
 * @section Security Considerations
 * 
 * For production deployment:
 * - Add HTTPS/TLS support
 * - Implement authentication (API keys, OAuth)
 * - Add rate limiting
 * - Enable CORS properly
 * - Input validation
 * - Request logging
 */
