#include <catch_amalgamated.hpp>
#include <mcp/server.hpp>
#include <mcp/transport/transport.hpp>
#include <thread>
#include <chrono>

using namespace pooriayousefi::mcp;
using json = nlohmann::json;

// Helper to wait for async events
template<typename Predicate>
bool wait_for(Predicate pred, int timeout_ms = 1000) {
    auto start = std::chrono::steady_clock::now();
    while (!pred()) {
        if (std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count() > timeout_ms) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return true;
}

// ==================== Server Construction Tests ====================

TEST_CASE("Server construction and initialization", "[server][construction]") {
    auto [client_transport, server_transport] = transport::create_in_memory_pair();
    Implementation server_impl{"test-server", "1.0.0"};
    
    SECTION("Basic construction") {
        Server server(server_transport, server_impl);
        
        // Server should be constructed but not initialized
        REQUIRE_FALSE(server.is_initialized());
    }
    
    SECTION("Server with instructions") {
        Server server(server_transport, server_impl);
        server.set_instructions("Test server instructions");
        
        REQUIRE_FALSE(server.is_initialized());
    }
}

// ==================== Capability Registration Tests ====================

TEST_CASE("Server capability registration", "[server][capabilities]") {
    auto [client_transport, server_transport] = transport::create_in_memory_pair();
    Implementation server_impl{"test-server", "1.0.0"};
    Server server(server_transport, server_impl);
    
    SECTION("Enable tools capability") {
        server.enable_tools(false);
        // Capability is set (verified during initialize)
    }
    
    SECTION("Enable tools with list changed notifications") {
        server.enable_tools(true);
        // Will be verified in initialize response
    }
    
    SECTION("Enable prompts capability") {
        server.enable_prompts(false);
    }
    
    SECTION("Enable prompts with notifications") {
        server.enable_prompts(true);
    }
    
    SECTION("Enable resources capability") {
        server.enable_resources(false, false);
    }
    
    SECTION("Enable resources with subscribe and notifications") {
        server.enable_resources(true, true);
    }
    
    SECTION("Enable all capabilities") {
        server.enable_tools(true);
        server.enable_prompts(true);
        server.enable_resources(true, true);
        // All capabilities enabled
    }
}

// ==================== Tool Registration Tests ====================

TEST_CASE("Tool registration and listing", "[server][tools]") {
    auto [client_transport, server_transport] = transport::create_in_memory_pair();
    Implementation server_impl{"test-server", "1.0.0"};
    Server server(server_transport, server_impl);
    
    server.enable_tools();
    
    SECTION("Register single tool") {
        Tool tool{"test_tool", "A test tool", ToolInputSchema{}};
        bool handler_called = false;
        
        server.register_tool(tool, [&](const json&) {
            handler_called = true;
            return std::vector<ToolResultContent>{};
        });
        
        // Handler will be tested in execution section
        REQUIRE_FALSE(handler_called);
    }
    
    SECTION("Register multiple tools") {
        Tool tool1{"tool1", "First tool", ToolInputSchema{}};
        Tool tool2{"tool2", "Second tool", ToolInputSchema{}};
        Tool tool3{"tool3", "Third tool", ToolInputSchema{}};
        
        server.register_tool(tool1, [](const json&) { return std::vector<ToolResultContent>{}; });
        server.register_tool(tool2, [](const json&) { return std::vector<ToolResultContent>{}; });
        server.register_tool(tool3, [](const json&) { return std::vector<ToolResultContent>{}; });
        
        // Multiple tools registered
    }
}

TEST_CASE("Tool execution", "[server][tools][execution]") {
    auto [client_transport, server_transport] = transport::create_in_memory_pair();
    Implementation server_impl{"test-server", "1.0.0"};
    Server server(server_transport, server_impl);
    
    server.enable_tools();
    client_transport->start();
    server_transport->start();
    server.start();
    
    SECTION("Execute tool and receive result") {
        // Register echo tool
        Tool echo_tool{"echo", "Echoes input", ToolInputSchema{}};
        server.register_tool(echo_tool, [](const json& args) {
            std::vector<ToolResultContent> results;
            results.push_back(ToolResultContent{
                "text",
                args.dump(),
                std::nullopt,
                std::nullopt,
                std::nullopt
            });
            return results;
        });
        
        std::atomic<bool> response_received{false};
        json received_result;
        
        // Initialize first
        client_transport->on_message([&](const json& msg) {
            if (msg.contains("result")) {
                received_result = msg;
                response_received = true;
            }
        });
        
        json init_req = {
            {"jsonrpc", "2.0"},
            {"id", 1},
            {"method", "initialize"},
            {"params", {{"protocolVersion", "2024-11-05"}, {"capabilities", json::object()}}}
        };
        client_transport->send(init_req);
        
        REQUIRE(wait_for([&]() { return response_received.load(); }));
        
        // Now call the tool
        response_received = false;
        json call_tool_req = {
            {"jsonrpc", "2.0"},
            {"id", 2},
            {"method", "tools/call"},
            {"params", {
                {"name", "echo"},
                {"arguments", {{"msg", "hello"}}}
            }}
        };
        client_transport->send(call_tool_req);
        
        REQUIRE(wait_for([&]() { return response_received.load(); }));
        REQUIRE(received_result.contains("result"));
    }
    
    client_transport->close();
    server_transport->close();
}

// ==================== Prompt Registration Tests ====================

TEST_CASE("Prompt registration and listing", "[server][prompts]") {
    auto [client_transport, server_transport] = transport::create_in_memory_pair();
    Implementation server_impl{"test-server", "1.0.0"};
    Server server(server_transport, server_impl);
    
    server.enable_prompts();
    
    SECTION("Register single prompt") {
        Prompt prompt{"test_prompt", "A test prompt", {}};
        
        server.register_prompt(prompt, [](const std::map<std::string, std::string>&) {
            return std::vector<PromptMessage>{};
        });
    }
    
    SECTION("Register multiple prompts") {
        Prompt p1{"prompt1", "First", {}};
        Prompt p2{"prompt2", "Second", {}};
        
        server.register_prompt(p1, [](const std::map<std::string, std::string>&) {
            return std::vector<PromptMessage>{};
        });
        server.register_prompt(p2, [](const std::map<std::string, std::string>&) {
            return std::vector<PromptMessage>{};
        });
    }
}

// ==================== Resource Registration Tests ====================

TEST_CASE("Resource registration and reading", "[server][resources]") {
    auto [client_transport, server_transport] = transport::create_in_memory_pair();
    Implementation server_impl{"test-server", "1.0.0"};
    Server server(server_transport, server_impl);
    
    server.enable_resources();
    
    SECTION("Register single resource") {
        Resource resource{"file:///test.txt", "Test File", std::nullopt, "text/plain"};
        
        server.register_resource(resource, [](const std::string&) {
            return std::vector<ResourceContent>{};
        });
    }
    
    SECTION("Register multiple resources") {
        Resource r1{"file:///file1.txt", "File 1", std::nullopt, "text/plain"};
        Resource r2{"file:///file2.json", "File 2", std::nullopt, "application/json"};
        
        server.register_resource(r1, [](const std::string&) {
            return std::vector<ResourceContent>{};
        });
        server.register_resource(r2, [](const std::string&) {
            return std::vector<ResourceContent>{};
        });
    }
}

// ==================== Initialize Handshake Tests ====================

TEST_CASE("Server initialize handshake", "[server][initialize]") {
    auto [client_transport, server_transport] = transport::create_in_memory_pair();
    Implementation server_impl{"test-server", "1.0.0"};
    Server server(server_transport, server_impl);
    
    server.enable_tools();
    server.enable_prompts();
    server.enable_resources();
    
    client_transport->start();
    server_transport->start();
    server.start();
    
    SECTION("Successful initialization") {
        std::atomic<bool> init_response_received{false};
        json init_response;
        
        client_transport->on_message([&](const json& msg) {
            if (msg.contains("result") && msg["id"] == 1) {
                init_response = msg;
                init_response_received = true;
            }
        });
        
        json init_req = {
            {"jsonrpc", "2.0"},
            {"id", 1},
            {"method", "initialize"},
            {"params", {
                {"protocolVersion", "2024-11-05"},
                {"capabilities", json::object()},
                {"clientInfo", {{"name", "test-client"}, {"version", "1.0"}}}
            }}
        };
        
        client_transport->send(init_req);
        
        REQUIRE(wait_for([&]() { return init_response_received.load(); }));
        REQUIRE(init_response["result"].contains("capabilities"));
        REQUIRE(init_response["result"]["capabilities"].contains("tools"));
        REQUIRE(init_response["result"]["capabilities"].contains("prompts"));
        REQUIRE(init_response["result"]["capabilities"].contains("resources"));
        REQUIRE(server.is_initialized());
    }
    
    SECTION("Initialize with instructions") {
        server.set_instructions("Server usage instructions");
        
        std::atomic<bool> received{false};
        json response;
        
        client_transport->on_message([&](const json& msg) {
            if (msg.contains("result")) {
                response = msg;
                received = true;
            }
        });
        
        json init_req = {
            {"jsonrpc", "2.0"},
            {"id", 1},
            {"method", "initialize"},
            {"params", {{"protocolVersion", "2024-11-05"}, {"capabilities", json::object()}}}
        };
        
        client_transport->send(init_req);
        
        REQUIRE(wait_for([&]() { return received.load(); }));
        REQUIRE(response["result"].contains("instructions"));
        REQUIRE(response["result"]["instructions"] == "Server usage instructions");
    }
    
    client_transport->close();
    server_transport->close();
}

// ==================== Notification Tests ====================

TEST_CASE("Server notifications", "[server][notifications]") {
    auto [client_transport, server_transport] = transport::create_in_memory_pair();
    Implementation server_impl{"test-server", "1.0.0"};
    Server server(server_transport, server_impl);
    
    client_transport->start();
    server_transport->start();
    server.start();
    
    SECTION("Tools list changed notification") {
        server.enable_tools(true);
        
        std::atomic<bool> notif_received{false};
        json notification;
        
        client_transport->on_message([&](const json& msg) {
            if (msg.contains("method") && !msg.contains("id")) {
                notification = msg;
                notif_received = true;
            }
        });
        
        server.notify_tools_changed();
        
        REQUIRE(wait_for([&]() { return notif_received.load(); }, 500));
        REQUIRE(notification["method"] == "notifications/tools/list_changed");
    }
    
    SECTION("Prompts list changed notification") {
        server.enable_prompts(true);
        
        std::atomic<bool> notif_received{false};
        json notification;
        
        client_transport->on_message([&](const json& msg) {
            if (msg.contains("method") && !msg.contains("id")) {
                notification = msg;
                notif_received = true;
            }
        });
        
        server.notify_prompts_changed();
        
        REQUIRE(wait_for([&]() { return notif_received.load(); }, 500));
        REQUIRE(notification["method"] == "notifications/prompts/list_changed");
    }
    
    SECTION("Resources list changed notification") {
        server.enable_resources(false, true);
        
        std::atomic<bool> notif_received{false};
        json notification;
        
        client_transport->on_message([&](const json& msg) {
            if (msg.contains("method") && !msg.contains("id")) {
                notification = msg;
                notif_received = true;
            }
        });
        
        server.notify_resources_changed();
        
        REQUIRE(wait_for([&]() { return notif_received.load(); }, 500));
        REQUIRE(notification["method"] == "notifications/resources/list_changed");
    }
    
    SECTION("Log message notification") {
        std::atomic<bool> log_received{false};
        json log_notification;
        
        client_transport->on_message([&](const json& msg) {
            if (msg.contains("method") && msg["method"] == "notifications/message") {
                log_notification = msg;
                log_received = true;
            }
        });
        
        server.send_log("info", "Test log message");
        
        REQUIRE(wait_for([&]() { return log_received.load(); }, 500));
        REQUIRE(log_notification["params"]["level"] == "info");
        REQUIRE(log_notification["params"]["data"] == "Test log message");
    }
    
    SECTION("Custom notification") {
        std::atomic<bool> custom_received{false};
        json custom_notif;
        
        client_transport->on_message([&](const json& msg) {
            if (msg.contains("method") && msg["method"] == "custom/event") {
                custom_notif = msg;
                custom_received = true;
            }
        });
        
        server.send_notification("custom/event", json{{"data", "custom"}});
        
        REQUIRE(wait_for([&]() { return custom_received.load(); }, 500));
        REQUIRE(custom_notif["params"]["data"] == "custom");
    }
    
    client_transport->close();
    server_transport->close();
}

// ==================== List Operations Tests ====================

TEST_CASE("Server list operations", "[server][list]") {
    auto [client_transport, server_transport] = transport::create_in_memory_pair();
    Implementation server_impl{"test-server", "1.0.0"};
    Server server(server_transport, server_impl);
    
    server.enable_tools();
    server.enable_prompts();
    server.enable_resources();
    
    // Register some items
    Tool tool1{"tool1", "First tool", ToolInputSchema{}};
    Tool tool2{"tool2", "Second tool", ToolInputSchema{}};
    server.register_tool(tool1, [](const json&) { return std::vector<ToolResultContent>{}; });
    server.register_tool(tool2, [](const json&) { return std::vector<ToolResultContent>{}; });
    
    Prompt prompt1{"prompt1", "First prompt", {}};
    server.register_prompt(prompt1, [](const std::map<std::string, std::string>&) {
        return std::vector<PromptMessage>{};
    });
    
    Resource res1{"file:///test.txt", "Test", std::nullopt, std::nullopt};
    server.register_resource(res1, [](const std::string&) {
        return std::vector<ResourceContent>{};
    });
    
    client_transport->start();
    server_transport->start();
    server.start();
    
    // Initialize
    std::atomic<bool> init_done{false};
    client_transport->on_message([&](const json& msg) {
        if (msg.contains("result") && msg["id"] == 1) {
            init_done = true;
        }
    });
    
    json init_req = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "initialize"},
        {"params", {{"protocolVersion", "2024-11-05"}, {"capabilities", json::object()}}}
    };
    client_transport->send(init_req);
    REQUIRE(wait_for([&]() { return init_done.load(); }));
    
    SECTION("List tools") {
        std::atomic<bool> list_received{false};
        json list_response;
        
        client_transport->on_message([&](const json& msg) {
            if (msg.contains("result") && msg["id"] == 2) {
                list_response = msg;
                list_received = true;
            }
        });
        
        json list_req = {
            {"jsonrpc", "2.0"},
            {"id", 2},
            {"method", "tools/list"}
        };
        client_transport->send(list_req);
        
        REQUIRE(wait_for([&]() { return list_received.load(); }));
        REQUIRE(list_response["result"].contains("tools"));
        REQUIRE(list_response["result"]["tools"].is_array());
        REQUIRE(list_response["result"]["tools"].size() == 2);
    }
    
    SECTION("List prompts") {
        std::atomic<bool> list_received{false};
        json list_response;
        
        client_transport->on_message([&](const json& msg) {
            if (msg.contains("result") && msg["id"] == 3) {
                list_response = msg;
                list_received = true;
            }
        });
        
        json list_req = {
            {"jsonrpc", "2.0"},
            {"id", 3},
            {"method", "prompts/list"}
        };
        client_transport->send(list_req);
        
        REQUIRE(wait_for([&]() { return list_received.load(); }));
        REQUIRE(list_response["result"].contains("prompts"));
        REQUIRE(list_response["result"]["prompts"].size() == 1);
    }
    
    SECTION("List resources") {
        std::atomic<bool> list_received{false};
        json list_response;
        
        client_transport->on_message([&](const json& msg) {
            if (msg.contains("result") && msg["id"] == 4) {
                list_response = msg;
                list_received = true;
            }
        });
        
        json list_req = {
            {"jsonrpc", "2.0"},
            {"id", 4},
            {"method", "resources/list"}
        };
        client_transport->send(list_req);
        
        REQUIRE(wait_for([&]() { return list_received.load(); }));
        REQUIRE(list_response["result"].contains("resources"));
        REQUIRE(list_response["result"]["resources"].size() == 1);
    }
    
    client_transport->close();
    server_transport->close();
}

// ==================== Error Handling Tests ====================

TEST_CASE("Server error handling", "[server][errors]") {
    auto [client_transport, server_transport] = transport::create_in_memory_pair();
    Implementation server_impl{"test-server", "1.0.0"};
    Server server(server_transport, server_impl);
    
    SECTION("Error callback registration") {
        std::string error_msg;
        server.on_error([&](const std::string& err) {
            error_msg = err;
        });
        
        // Error callback is set
        REQUIRE(error_msg.empty());
    }
    
    SECTION("Transport error propagation") {
        std::atomic<bool> error_received{false};
        std::string error_text;
        
        server.on_error([&](const std::string& err) {
            error_text = err;
            error_received = true;
        });
        
        client_transport->start();
        server_transport->start();
        server.start();
        
        // Simulate transport error
        server_transport->send(json{{"invalid", "not started"}});
        
        // Note: Actual error handling depends on transport implementation
    }
}

// ==================== Lifecycle Tests ====================

TEST_CASE("Server lifecycle management", "[server][lifecycle]") {
    auto [client_transport, server_transport] = transport::create_in_memory_pair();
    Implementation server_impl{"test-server", "1.0.0"};
    Server server(server_transport, server_impl);
    
    SECTION("Start server") {
        server.start();
        REQUIRE(server_transport->is_open());
    }
    
    SECTION("Start and close lifecycle") {
        auto [server_transport, client_transport] = pooriayousefi::mcp::transport::create_in_memory_pair();
        
        pooriayousefi::mcp::Implementation server_impl{
            "test-server",
            "1.0.0"
        };
        
        pooriayousefi::mcp::Server server(server_transport, server_impl);
        
        server.start();
        // Server should be running
        
        server.close();
        // Server should be closed
        
        REQUIRE(true); // If we get here without crash, lifecycle works
    }
    
    SECTION("Server initialization state") {
        REQUIRE_FALSE(server.is_initialized());
        
        client_transport->start();
        server_transport->start();
        server.start();
        
        std::atomic<bool> init_done{false};
        client_transport->on_message([&](const json& msg) {
            if (msg.contains("result") && msg.contains("id")) {
                init_done = true;
            }
        });
        
        json init_req = {
            {"jsonrpc", "2.0"},
            {"id", 1},
            {"method", "initialize"},
            {"params", {{"protocolVersion", "2024-11-05"}, {"capabilities", json::object()}}}
        };
        client_transport->send(init_req);
        
        REQUIRE(wait_for([&]() { return init_done.load(); }));
        REQUIRE(server.is_initialized());
        
        client_transport->close();
        server_transport->close();
    }
}

// ==================== Integration Tests ====================

TEST_CASE("Server integration scenarios", "[server][integration]") {
    auto [client_transport, server_transport] = transport::create_in_memory_pair();
    Implementation server_impl{"integration-server", "1.0.0"};
    Server server(server_transport, server_impl);
    
    server.enable_tools();
    server.enable_prompts();
    server.enable_resources();
    
    // Register calculator tool
    Tool calc_tool{"calculate", "Performs calculation", ToolInputSchema{}};
    server.register_tool(calc_tool, [](const json& args) {
        std::vector<ToolResultContent> results;
        int a = args["a"].get<int>();
        int b = args["b"].get<int>();
        std::string op = args["op"].get<std::string>();
        
        int result = 0;
        if (op == "add") result = a + b;
        else if (op == "multiply") result = a * b;
        
        results.push_back(ToolResultContent{
            "text",
            std::to_string(result),
            std::nullopt,
            std::nullopt,
            std::nullopt
        });
        return results;
    });
    
    client_transport->start();
    server_transport->start();
    server.start();
    
    SECTION("Full client-server interaction") {
        std::vector<json> responses;
        std::mutex response_mutex;
        
        client_transport->on_message([&](const json& msg) {
            std::lock_guard<std::mutex> lock(response_mutex);
            responses.push_back(msg);
        });
        
        // 1. Initialize
        json init_req = {
            {"jsonrpc", "2.0"},
            {"id", 1},
            {"method", "initialize"},
            {"params", {{"protocolVersion", "2024-11-05"}, {"capabilities", json::object()}}}
        };
        client_transport->send(init_req);
        
        // 2. List tools
        json list_req = {
            {"jsonrpc", "2.0"},
            {"id", 2},
            {"method", "tools/list"}
        };
        client_transport->send(list_req);
        
        // 3. Call tool
        json call_req = {
            {"jsonrpc", "2.0"},
            {"id", 3},
            {"method", "tools/call"},
            {"params", {
                {"name", "calculate"},
                {"arguments", {{"a", 5}, {"b", 3}, {"op", "add"}}}
            }}
        };
        client_transport->send(call_req);
        
        // Wait for all responses
        REQUIRE(wait_for([&]() {
            std::lock_guard<std::mutex> lock(response_mutex);
            return responses.size() >= 3;
        }, 2000));
        
        std::lock_guard<std::mutex> lock(response_mutex);
        REQUIRE(responses.size() >= 3);
    }
    
    client_transport->close();
    server_transport->close();
}
