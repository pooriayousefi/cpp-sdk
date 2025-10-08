#include <mcp/server.hpp>
#include <mcp/transport/transport.hpp>
#include <iostream>

using namespace pooriayousefi::mcp;

int main() {
    // Create stdio transport for command-line communication
    auto transport = std::make_shared<transport::StdioTransport>();

    // Create server with implementation info
    Server server(transport, Implementation{
        "example-server",
        "1.0.0"
    });

    // Set server instructions
    server.set_instructions(
        "This is an example MCP server demonstrating the C++ SDK. "
        "It provides a simple calculator tool."
    );

    // Enable tools capability
    server.enable_tools(true);

    // Register a calculator tool
    Tool calculator_tool{
        "add",
        "Add two numbers together",
        ToolInputSchema{
            "object",
            json{
                {"a", {{"type", "number"}, {"description", "First number"}}},
                {"b", {{"type", "number"}, {"description", "Second number"}}}
            },
            {"a", "b"}
        }
    };

    server.register_tool(calculator_tool, [](const json& args) -> std::vector<ToolResultContent> {
        double a = args.value("a", 0.0);
        double b = args.value("b", 0.0);
        double result = a + b;
        
        return {ToolResultContent::text_content(
            "Result: " + std::to_string(result)
        )};
    });

    // Set error handler
    server.on_error([](const std::string& error) {
        std::cerr << "Server error: " << error << std::endl;
    });

    // Start server
    std::cout << "MCP Server starting..." << std::endl;
    server.start();

    // Keep running
    std::cout << "Server running. Press Ctrl+C to stop." << std::endl;
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
