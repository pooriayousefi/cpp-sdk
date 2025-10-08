#include "catch_amalgamated.hpp"
#include "../include/mcp/client_async.hpp"
#include "../include/mcp/server.hpp"
#include "../include/mcp/transport/transport.hpp"
#include <atomic>
#include <chrono>
#include <thread>

using namespace pooriayousefi::mcp;
using json = nlohmann::json;

// Helper to synchronously execute Task<void> coroutines
template<typename TaskType>
void sync_wait_client(TaskType&& task) {
    pooriayousefi::core::sync_wait(std::forward<TaskType>(task));
}

// Helper function to wait for async operations
template<typename Predicate>
bool wait_for(Predicate pred, int timeout_ms = 1000) {
    auto start = std::chrono::steady_clock::now();
    while (!pred()) {
        if (std::chrono::steady_clock::now() - start > std::chrono::milliseconds(timeout_ms)) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return true;
}

TEST_CASE("AsyncClient construction", "[construction][async_client]") {
    SECTION("AsyncClient can wrap regular Client") {
        auto [client_transport, server_transport] = transport::create_in_memory_pair();
        
        Client client(client_transport);
        AsyncClient async_client(client);
        
        REQUIRE(true); // If we get here, construction succeeded
    }
}

TEST_CASE("AsyncClient initialize", "[initialize][async_client]") {
    SECTION("Async initialization with server") {
        auto [client_transport, server_transport] = transport::create_in_memory_pair();
        
        // Setup server
        Implementation server_impl{"async-test-server", "1.0.0"};
        Server server(server_transport, server_impl);
        server.enable_tools();
        server.start();
        
        // Setup client
        Client client(client_transport);
        AsyncClient async_client(client);
        client.start();
        
        // Test async initialization
        auto init_task = [&]() -> Task<void> {
            Implementation client_impl{"async-client", "1.0.0"};
            ClientCapabilities capabilities;
            
            ServerInfo info = co_await async_client.initialize_async(client_impl, capabilities);
            
            REQUIRE(info.server_info.name == "async-test-server");
            REQUIRE(info.server_info.version == "1.0.0");
            REQUIRE(client.is_initialized());
            co_return;
        };
        
        sync_wait_client(init_task());
    }
}

TEST_CASE("AsyncClient list tools", "[async_client][tools][list]") {
    SECTION("Async list tools from server") {
        auto [client_transport, server_transport] = transport::create_in_memory_pair();
        
        // Setup server with tools
        Implementation server_impl{"test-server", "1.0.0"};
        Server server(server_transport, server_impl);
        server.enable_tools();
        
        Tool echo_tool;
        echo_tool.name = "echo";
        echo_tool.description = "Echo tool";
        echo_tool.input_schema = ToolInputSchema{};
        
        Tool calc_tool;
        calc_tool.name = "calculator";
        calc_tool.description = "Calculator tool";
        calc_tool.input_schema = ToolInputSchema{};
        
        server.register_tool(echo_tool, [](const json&) {
            return std::vector<ToolResultContent>{};
        });
        
        server.register_tool(calc_tool, [](const json&) {
            return std::vector<ToolResultContent>{};
        });
        
        server.start();
        
        // Setup client
        Client client(client_transport);
        AsyncClient async_client(client);
        client.start();
        
        // Initialize and list tools
        auto test_task = [&]() -> Task<void> {
            // Initialize first
            co_await async_client.initialize_async(
                Implementation{"client", "1.0.0"},
                ClientCapabilities{}
            );
            
            // List tools
            auto tools = co_await async_client.list_tools_async();
            
            REQUIRE(tools.size() == 2);
            
            bool has_echo = false;
            bool has_calc = false;
            for (const auto& tool : tools) {
                if (tool.name == "echo") has_echo = true;
                if (tool.name == "calculator") has_calc = true;
            }
            REQUIRE(has_echo);
            REQUIRE(has_calc);
            co_return;
        };
        
        sync_wait_client(test_task());
    }
}

TEST_CASE("AsyncClient call tool", "[async_client][tools][execution]") {
    SECTION("Async call tool with arguments") {
        auto [client_transport, server_transport] = transport::create_in_memory_pair();
        
        // Setup server with calculator
        Implementation server_impl{"test-server", "1.0.0"};
        Server server(server_transport, server_impl);
        server.enable_tools();
        
        Tool calc_tool;
        calc_tool.name = "add";
        calc_tool.description = "Add two numbers";
        calc_tool.input_schema = ToolInputSchema{};
        
        server.register_tool(calc_tool, [](const json& args) {
            int a = args["a"].get<int>();
            int b = args["b"].get<int>();
            int result = a + b;
            
            std::vector<ToolResultContent> results;
            results.push_back(ToolResultContent{
                "text",
                std::to_string(result),
                std::nullopt,
                std::nullopt,
                std::nullopt
            });
            return results;
        });
        
        server.start();
        
        // Setup client
        Client client(client_transport);
        AsyncClient async_client(client);
        client.start();
        
        // Test async tool call
        auto test_task = [&]() -> Task<void> {
            // Initialize
            co_await async_client.initialize_async(
                Implementation{"client", "1.0.0"},
                ClientCapabilities{}
            );
            
            // Call tool
            json args = {{"a", 15}, {"b", 27}};
            auto results = co_await async_client.call_tool_async("add", args);
            
            REQUIRE(results.size() == 1);
            REQUIRE(results[0].type == "text");
            REQUIRE(results[0].text.has_value());
            REQUIRE(results[0].text.value() == "42");
            co_return;
        };
        
        sync_wait_client(test_task());
    }
}

TEST_CASE("AsyncClient list prompts", "[async_client][prompts][list]") {
    SECTION("Async list prompts from server") {
        auto [client_transport, server_transport] = transport::create_in_memory_pair();
        
        // Setup server with prompts
        Implementation server_impl{"test-server", "1.0.0"};
        Server server(server_transport, server_impl);
        server.enable_prompts();
        
        Prompt greeting_prompt;
        greeting_prompt.name = "greeting";
        greeting_prompt.description = "Greeting prompt";
        
        Prompt summary_prompt;
        summary_prompt.name = "summarize";
        summary_prompt.description = "Summarize text";
        
        server.register_prompt(greeting_prompt, [](const std::map<std::string, std::string>&) {
            return std::vector<PromptMessage>{};
        });
        
        server.register_prompt(summary_prompt, [](const std::map<std::string, std::string>&) {
            return std::vector<PromptMessage>{};
        });
        
        server.start();
        
        // Setup client
        Client client(client_transport);
        AsyncClient async_client(client);
        client.start();
        
        // Test async list prompts
        auto test_task = [&]() -> Task<void> {
            // Initialize
            co_await async_client.initialize_async(
                Implementation{"client", "1.0.0"},
                ClientCapabilities{}
            );
            
            // List prompts
            auto prompts = co_await async_client.list_prompts_async();
            
            REQUIRE(prompts.size() == 2);
            
            bool has_greeting = false;
            bool has_summarize = false;
            for (const auto& prompt : prompts) {
                if (prompt.name == "greeting") has_greeting = true;
                if (prompt.name == "summarize") has_summarize = true;
            }
            REQUIRE(has_greeting);
            REQUIRE(has_summarize);
            co_return;
        };
        
        sync_wait_client(test_task());
    }
}

TEST_CASE("AsyncClient get prompt", "[async_client][prompts][execution]") {
    SECTION("Async get prompt with arguments") {
        auto [client_transport, server_transport] = transport::create_in_memory_pair();
        
        // Setup server with prompt
        Implementation server_impl{"test-server", "1.0.0"};
        Server server(server_transport, server_impl);
        server.enable_prompts();
        
        Prompt greeting_prompt;
        greeting_prompt.name = "greeting";
        greeting_prompt.description = "Generate greeting";
        
        server.register_prompt(greeting_prompt, [](const std::map<std::string, std::string>& args) {
            std::string name = args.count("name") ? args.at("name") : "World";
            
            PromptMessage msg;
            msg.role = MessageRole::User;
            msg.content.push_back(MessageContent{
                "text",
                std::optional<std::string>("Hello, " + name + "!"),
                std::nullopt,
                std::nullopt
            });
            
            return std::vector<PromptMessage>{msg};
        });
        
        server.start();
        
        // Setup client
        Client client(client_transport);
        AsyncClient async_client(client);
        client.start();
        
        // Test async get prompt
        auto test_task = [&]() -> Task<void> {
            // Initialize
            co_await async_client.initialize_async(
                Implementation{"client", "1.0.0"},
                ClientCapabilities{}
            );
            
            // Get prompt
            std::map<std::string, std::string> args = {{"name", "AsyncWorld"}};
            auto messages = co_await async_client.get_prompt_async("greeting", args);
            
            REQUIRE(messages.size() == 1);
            REQUIRE(messages[0].role == MessageRole::User);
            co_return;
        };
        
        sync_wait_client(test_task());
    }
}

TEST_CASE("AsyncClient list resources", "[async_client][resources][list]") {
    SECTION("Async list resources from server") {
        auto [client_transport, server_transport] = transport::create_in_memory_pair();
        
        // Setup server with resources
        Implementation server_impl{"test-server", "1.0.0"};
        Server server(server_transport, server_impl);
        server.enable_resources();
        
        Resource file_resource;
        file_resource.uri = "file:///test.txt";
        file_resource.name = "test.txt";
        file_resource.mime_type = "text/plain";
        
        Resource config_resource;
        config_resource.uri = "file:///config.json";
        config_resource.name = "config.json";
        config_resource.mime_type = "application/json";
        
        server.register_resource(file_resource, [](const std::string&) {
            return std::vector<ResourceContent>{};
        });
        
        server.register_resource(config_resource, [](const std::string&) {
            return std::vector<ResourceContent>{};
        });
        
        server.start();
        
        // Setup client
        Client client(client_transport);
        AsyncClient async_client(client);
        client.start();
        
        // Test async list resources
        auto test_task = [&]() -> Task<void> {
            // Initialize
            co_await async_client.initialize_async(
                Implementation{"client", "1.0.0"},
                ClientCapabilities{}
            );
            
            // List resources
            auto resources = co_await async_client.list_resources_async();
            
            REQUIRE(resources.size() == 2);
            
            bool has_test_txt = false;
            bool has_config_json = false;
            for (const auto& resource : resources) {
                if (resource.uri == "file:///test.txt") has_test_txt = true;
                if (resource.uri == "file:///config.json") has_config_json = true;
            }
            REQUIRE(has_test_txt);
            REQUIRE(has_config_json);
            co_return;
        };
        
        sync_wait_client(test_task());
    }
}

TEST_CASE("AsyncClient read resource", "[async_client][resources][execution]") {
    SECTION("Async read resource content") {
        auto [client_transport, server_transport] = transport::create_in_memory_pair();
        
        // Setup server with resource
        Implementation server_impl{"test-server", "1.0.0"};
        Server server(server_transport, server_impl);
        server.enable_resources();
        
        Resource file_resource;
        file_resource.uri = "file:///async_test.txt";
        file_resource.name = "async_test.txt";
        file_resource.mime_type = "text/plain";
        
        server.register_resource(file_resource, [](const std::string& uri) {
            ResourceContent content;
            content.uri = uri;
            content.mime_type = "text/plain";
            content.text = "Async resource content!";
            
            return std::vector<ResourceContent>{content};
        });
        
        server.start();
        
        // Setup client
        Client client(client_transport);
        AsyncClient async_client(client);
        client.start();
        
        // Test async read resource
        auto test_task = [&]() -> Task<void> {
            // Initialize
            co_await async_client.initialize_async(
                Implementation{"client", "1.0.0"},
                ClientCapabilities{}
            );
            
            // Read resource
            auto contents = co_await async_client.read_resource_async("file:///async_test.txt");
            
            REQUIRE(contents.size() == 1);
            REQUIRE(contents[0].uri == "file:///async_test.txt");
            REQUIRE(contents[0].text.has_value());
            REQUIRE(contents[0].text.value() == "Async resource content!");
            co_return;
        };
        
        sync_wait_client(test_task());
    }
}

TEST_CASE("AsyncClient parallel execution", "[async_client][parallel][execution]") {
    SECTION("Execute multiple tools in parallel") {
        auto [client_transport, server_transport] = transport::create_in_memory_pair();
        
        // Setup server with multiple tools
        Implementation server_impl{"test-server", "1.0.0"};
        Server server(server_transport, server_impl);
        server.enable_tools();
        
        // Tool 1: Add
        Tool add_tool;
        add_tool.name = "add";
        add_tool.description = "Add numbers";
        add_tool.input_schema = ToolInputSchema{};
        
        server.register_tool(add_tool, [](const json& args) {
            int result = args["a"].get<int>() + args["b"].get<int>();
            std::vector<ToolResultContent> results;
            results.push_back(ToolResultContent{
                "text",
                std::to_string(result),
                std::nullopt,
                std::nullopt,
                std::nullopt
            });
            return results;
        });
        
        // Tool 2: Multiply
        Tool mul_tool;
        mul_tool.name = "multiply";
        mul_tool.description = "Multiply numbers";
        mul_tool.input_schema = ToolInputSchema{};
        
        server.register_tool(mul_tool, [](const json& args) {
            int result = args["a"].get<int>() * args["b"].get<int>();
            std::vector<ToolResultContent> results;
            results.push_back(ToolResultContent{
                "text",
                std::to_string(result),
                std::nullopt,
                std::nullopt,
                std::nullopt
            });
            return results;
        });
        
        // Tool 3: Square
        Tool square_tool;
        square_tool.name = "square";
        square_tool.description = "Square a number";
        square_tool.input_schema = ToolInputSchema{};
        
        server.register_tool(square_tool, [](const json& args) {
            int x = args["x"].get<int>();
            int result = x * x;
            std::vector<ToolResultContent> results;
            results.push_back(ToolResultContent{
                "text",
                std::to_string(result),
                std::nullopt,
                std::nullopt,
                std::nullopt
            });
            return results;
        });
        
        server.start();
        
        // Setup client
        Client client(client_transport);
        AsyncClient async_client(client);
        client.start();
        
        // Test parallel execution
        auto test_task = [&]() -> Task<void> {
            // Initialize
            co_await async_client.initialize_async(
                Implementation{"client", "1.0.0"},
                ClientCapabilities{}
            );
            
            // Execute three tools in parallel
            std::vector<std::pair<std::string, json>> calls = {
                {"add", json{{"a", 10}, {"b", 5}}},
                {"multiply", json{{"a", 3}, {"b", 7}}},
                {"square", json{{"x", 6}}}
            };
            
            auto results = co_await async_client.execute_parallel_async(calls);
            
            REQUIRE(results.size() == 3);
            
            // Check add result: 10 + 5 = 15
            REQUIRE(results[0].size() == 1);
            REQUIRE(results[0][0].text.value() == "15");
            
            // Check multiply result: 3 * 7 = 21
            REQUIRE(results[1].size() == 1);
            REQUIRE(results[1][0].text.value() == "21");
            
            // Check square result: 6^2 = 36
            REQUIRE(results[2].size() == 1);
            REQUIRE(results[2][0].text.value() == "36");
            co_return;
        };
        
        sync_wait_client(test_task());
    }
}

TEST_CASE("AsyncClient error handling", "[async_client][errors]") {
    SECTION("Error throws exception in async context") {
        auto [client_transport, server_transport] = transport::create_in_memory_pair();
        
        Client client(client_transport);
        AsyncClient async_client(client);
        client.start();
        
        // Try to list tools without initialization
        auto test_task = [&]() -> Task<void> {
            bool exception_caught = false;
            try {
                auto tools = co_await async_client.list_tools_async();
            } catch (const std::runtime_error& e) {
                exception_caught = true;
                REQUIRE(std::string(e.what()) == "Client not initialized");
            }
            REQUIRE(exception_caught);
            co_return;
        };
        
        sync_wait_client(test_task());
    }
}

TEST_CASE("AsyncClient integration scenarios", "[async_client][integration]") {
    SECTION("Full async client-server workflow") {
        auto [client_transport, server_transport] = transport::create_in_memory_pair();
        
        // Setup server
        Implementation server_impl{"calc-server", "1.0.0"};
        Server server(server_transport, server_impl);
        server.enable_tools();
        
        Tool calc_tool;
        calc_tool.name = "calculate";
        calc_tool.description = "Perform calculation";
        calc_tool.input_schema = ToolInputSchema{};
        
        server.register_tool(calc_tool, [](const json& args) {
            std::string op = args["op"].get<std::string>();
            int a = args["a"].get<int>();
            int b = args["b"].get<int>();
            
            int result = 0;
            if (op == "add") result = a + b;
            else if (op == "sub") result = a - b;
            else if (op == "mul") result = a * b;
            else if (op == "div") result = a / b;
            
            std::vector<ToolResultContent> results;
            results.push_back(ToolResultContent{
                "text",
                std::to_string(result),
                std::nullopt,
                std::nullopt,
                std::nullopt
            });
            return results;
        });
        
        server.start();
        
        // Setup client
        Client client(client_transport);
        AsyncClient async_client(client);
        client.start();
        
        // Full workflow test
        auto test_task = [&]() -> Task<void> {
            // 1. Initialize
            auto info = co_await async_client.initialize_async(
                Implementation{"calc-client", "1.0.0"},
                ClientCapabilities{}
            );
            REQUIRE(info.server_info.name == "calc-server");
            
            // 2. List tools
            auto tools = co_await async_client.list_tools_async();
            REQUIRE(tools.size() == 1);
            REQUIRE(tools[0].name == "calculate");
            
            // 3. Perform calculations
            auto result1 = co_await async_client.call_tool_async(
                "calculate",
                json{{"op", "add"}, {"a", 100}, {"b", 50}}
            );
            REQUIRE(result1[0].text.value() == "150");
            
            auto result2 = co_await async_client.call_tool_async(
                "calculate",
                json{{"op", "mul"}, {"a", 12}, {"b", 3}}
            );
            REQUIRE(result2[0].text.value() == "36");
            
            auto result3 = co_await async_client.call_tool_async(
                "calculate",
                json{{"op", "div"}, {"a", 100}, {"b", 4}}
            );
            REQUIRE(result3[0].text.value() == "25");
            co_return;
        };
        
        sync_wait_client(test_task());
    }
    
    SECTION("Sequential async operations") {
        auto [client_transport, server_transport] = transport::create_in_memory_pair();
        
        // Setup server
        Implementation server_impl{"test-server", "1.0.0"};
        Server server(server_transport, server_impl);
        server.enable_tools();
        server.enable_prompts();
        server.enable_resources();
        
        // Register minimal handlers
        Tool tool;
        tool.name = "test_tool";
        tool.description = "Test";
        tool.input_schema = ToolInputSchema{};
        server.register_tool(tool, [](const json&) { return std::vector<ToolResultContent>{}; });
        
        Prompt prompt;
        prompt.name = "test_prompt";
        prompt.description = "Test";
        server.register_prompt(prompt, [](const std::map<std::string, std::string>&) {
            return std::vector<PromptMessage>{};
        });
        
        Resource resource;
        resource.uri = "test://resource";
        resource.name = "test";
        server.register_resource(resource, [](const std::string&) {
            return std::vector<ResourceContent>{};
        });
        
        server.start();
        
        // Setup client
        Client client(client_transport);
        AsyncClient async_client(client);
        client.start();
        
        // Test sequential operations
        auto test_task = [&]() -> Task<void> {
            // Initialize
            co_await async_client.initialize_async(
                Implementation{"client", "1.0.0"},
                ClientCapabilities{}
            );
            
            // Sequential operations
            auto tools = co_await async_client.list_tools_async();
            REQUIRE(tools.size() == 1);
            
            auto prompts = co_await async_client.list_prompts_async();
            REQUIRE(prompts.size() == 1);
            
            auto resources = co_await async_client.list_resources_async();
            REQUIRE(resources.size() == 1);
            co_return;
        };
        
        sync_wait_client(test_task());
    }
}

TEST_CASE("AsyncClient sync_wait functionality", "[async_client][sync_wait]") {
    SECTION("sync_wait_client can execute tasks synchronously") {
        auto [client_transport, server_transport] = transport::create_in_memory_pair();
        
        // Setup server
        Implementation server_impl{"test-server", "1.0.0"};
        Server server(server_transport, server_impl);
        server.start();
        
        // Setup client
        Client client(client_transport);
        AsyncClient async_client(client);
        client.start();
        
        // Use sync_wait to execute async task
        auto task = [&]() -> Task<std::string> {
            co_await async_client.initialize_async(
                Implementation{"client", "1.0.0"},
                ClientCapabilities{}
            );
            co_return "Initialization complete";
        };
        
        std::string result = sync_wait_client(task());
        REQUIRE(result == "Initialization complete");
        REQUIRE(client.is_initialized());
    }
}
