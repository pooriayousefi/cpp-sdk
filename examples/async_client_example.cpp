#include <mcp/client_async.hpp>
#include <mcp/transport/transport.hpp>
#include <iostream>
#include <vector>

/**
 * @file async_client_example.cpp
 * @brief Demonstrates AsyncClient using Task<T> for non-blocking operations
 * 
 * This example shows how to use the async client API for parallel tool execution
 * and concurrent MCP operations without blocking.
 * 
 * Compile:
 *   g++ -std=c++23 -I../include async_client_example.cpp -o async_client
 */

using namespace pooriayousefi::mcp;
using json = nlohmann::json;

int main() {
    try {
        // Create stdio transport for connecting to MCP server
        auto transport = std::make_shared<StdioTransport>();
        
        // Create async client
        AsyncClient client(transport);

        std::cout << "=== MCP Async Client Example ===\n\n";

        // Example 1: Initialize server connection (async)
        std::cout << "1. Initializing connection...\n";
        auto init_task = client.initialize_async(
            Implementation{
                "async-client-example",
                "1.0.0"
            },
            ClientCapabilities{}
        );
        
        auto server_info = core::sync_wait(init_task);
        std::cout << "   Connected to: " << server_info.name 
                  << " v" << server_info.version << "\n\n";

        // Example 2: List available tools (async)
        std::cout << "2. Listing tools...\n";
        auto tools_task = client.list_tools_async();
        auto tools = core::sync_wait(tools_task);
        
        std::cout << "   Found " << tools.size() << " tools:\n";
        for (const auto& tool : tools) {
            std::cout << "   - " << tool.name << ": " << tool.description << "\n";
        }
        std::cout << "\n";

        // Example 3: Execute single tool call (async)
        std::cout << "3. Calling tool 'echo' asynchronously...\n";
        auto call_task = client.call_tool_async("echo", {
            {"message", "Hello from async client!"}
        });
        
        auto result = core::sync_wait(call_task);
        std::cout << "   Result: " << result.dump(2) << "\n\n";

        // Example 4: Parallel tool execution
        std::cout << "4. Executing multiple tools in parallel...\n";
        
        std::vector<std::pair<std::string, json>> parallel_calls = {
            {"echo", {{"message", "First parallel call"}}},
            {"echo", {{"message", "Second parallel call"}}},
            {"echo", {{"message", "Third parallel call"}}}
        };

        auto parallel_task = client.execute_parallel_async(parallel_calls);
        auto parallel_results = core::sync_wait(parallel_task);

        std::cout << "   Completed " << parallel_results.size() << " parallel calls:\n";
        for (size_t i = 0; i < parallel_results.size(); ++i) {
            std::cout << "   [" << i << "] " << parallel_results[i].dump() << "\n";
        }
        std::cout << "\n";

        // Example 5: List prompts (async)
        std::cout << "5. Listing prompts...\n";
        auto prompts_task = client.list_prompts_async();
        auto prompts = core::sync_wait(prompts_task);
        
        std::cout << "   Found " << prompts.size() << " prompts:\n";
        for (const auto& prompt : prompts) {
            std::cout << "   - " << prompt.name << ": " << prompt.description << "\n";
        }
        std::cout << "\n";

        // Example 6: Get specific prompt (async)
        if (!prompts.empty()) {
            std::cout << "6. Getting prompt '" << prompts[0].name << "'...\n";
            auto prompt_task = client.get_prompt_async(prompts[0].name, {});
            auto prompt = core::sync_wait(prompt_task);
            
            std::cout << "   Description: " << prompt.description << "\n";
            std::cout << "   Messages: " << prompt.messages.size() << "\n\n";
        }

        // Example 7: List resources (async)
        std::cout << "7. Listing resources...\n";
        auto resources_task = client.list_resources_async();
        auto resources = core::sync_wait(resources_task);
        
        std::cout << "   Found " << resources.size() << " resources:\n";
        for (const auto& resource : resources) {
            std::cout << "   - " << resource.uri << " (" << resource.mimeType.value_or("unknown") << ")\n";
        }
        std::cout << "\n";

        // Example 8: Read resource (async)
        if (!resources.empty()) {
            std::cout << "8. Reading resource '" << resources[0].uri << "'...\n";
            auto content_task = client.read_resource_async(resources[0].uri);
            auto contents = core::sync_wait(content_task);
            
            std::cout << "   Read " << contents.size() << " content items:\n";
            for (const auto& content : contents) {
                std::cout << "   - Type: " << content.mimeType.value_or("text/plain") << "\n";
                std::cout << "     Size: " << content.text.value_or("").size() << " bytes\n";
            }
            std::cout << "\n";
        }

        // Example 9: Chain async operations
        std::cout << "9. Chaining async operations...\n";
        
        // This demonstrates how async operations can be composed
        auto chained_task = client.list_tools_async()
            .then([&client](std::vector<Tool> tools) -> core::Task<json> {
                if (!tools.empty()) {
                    // Call first available tool
                    return client.call_tool_async(tools[0].name, {});
                }
                return core::Task<json>::from_value(json::object());
            });
        
        auto chained_result = core::sync_wait(chained_task);
        std::cout << "   Chained result: " << chained_result.dump(2) << "\n\n";

        std::cout << "=== All async operations completed successfully! ===\n";

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}

/**
 * @section Key Features Demonstrated
 * 
 * 1. **Non-blocking Operations**: All MCP operations return Task<T> for async execution
 * 2. **Parallel Execution**: Multiple tool calls can run concurrently
 * 3. **Composability**: Tasks can be chained and composed
 * 4. **sync_wait**: Bridge between async and sync code
 * 5. **Error Handling**: Exceptions propagate through async boundaries
 * 
 * @section Performance Benefits
 * 
 * - No thread blocking during I/O operations
 * - Better CPU utilization with parallel calls
 * - Lower latency for multiple operations
 * - Efficient resource usage with coroutines
 * 
 * @section Usage Patterns
 * 
 * ```cpp
 * // Pattern 1: Single async call
 * auto task = client.call_tool_async("tool_name", args);
 * auto result = sync_wait(task);
 * 
 * // Pattern 2: Parallel execution
 * auto parallel_task = client.execute_parallel_async(calls);
 * auto results = sync_wait(parallel_task);
 * 
 * // Pattern 3: Chaining operations
 * auto chained = client.list_tools_async()
 *     .then([](tools) { return process_tools(tools); });
 * ```
 */
