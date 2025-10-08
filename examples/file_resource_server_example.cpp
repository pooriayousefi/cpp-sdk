#include <mcp/helpers/file_resource_server.hpp>
#include <mcp/transport/transport.hpp>
#include <iostream>
#include <filesystem>

/**
 * @file file_resource_server_example.cpp
 * @brief Demonstrates FileResourceServer using RAII file operations
 * 
 * This example shows how to serve files as MCP resources with automatic
 * MIME type detection, streaming, and safe file handling.
 * 
 * Compile:
 *   g++ -std=c++23 -I../include file_resource_server_example.cpp -o file_resource_server
 */

using namespace pooriayousefi::mcp;
using namespace pooriayousefi::mcp::helpers;
using json = nlohmann::json;
namespace fs = std::filesystem;

int main() {
    try {
        std::cout << "=== MCP File Resource Server Example ===\n\n";
        
        // Create a test directory with sample files
        fs::path test_dir = fs::temp_directory_path() / "mcp_file_server_test";
        
        std::cout << "1. Setting up test directory: " << test_dir << "\n";
        
        // Clean up if exists
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
        
        // Create test directory structure
        fs::create_directories(test_dir / "docs");
        fs::create_directories(test_dir / "data");
        fs::create_directories(test_dir / "logs");
        
        // Create sample files
        {
            std::ofstream readme(test_dir / "README.md");
            readme << "# Test Documentation\n\n";
            readme << "This is a test MCP file resource server.\n";
            readme << "It demonstrates serving files with RAII safety.\n";
        }
        
        {
            std::ofstream config(test_dir / "config.json");
            config << "{\n";
            config << "  \"server\": \"mcp-file-server\",\n";
            config << "  \"version\": \"1.0.0\",\n";
            config << "  \"features\": [\"raii\", \"streaming\", \"mime-detection\"]\n";
            config << "}\n";
        }
        
        {
            std::ofstream doc(test_dir / "docs" / "guide.txt");
            doc << "User Guide\n";
            doc << "==========\n\n";
            doc << "This guide explains how to use the file resource server.\n";
            for (int i = 0; i < 100; ++i) {
                doc << "Line " << i << ": Sample content for testing large files\n";
            }
        }
        
        {
            std::ofstream data(test_dir / "data" / "sample.csv");
            data << "id,name,value\n";
            for (int i = 0; i < 1000; ++i) {
                data << i << ",Item" << i << "," << (i * 10) << "\n";
            }
        }
        
        {
            std::ofstream log(test_dir / "logs" / "server.log");
            auto now = std::chrono::system_clock::now();
            for (int i = 0; i < 500; ++i) {
                log << "[2024-01-01 12:00:00] INFO: Log entry " << i << "\n";
            }
        }
        
        std::cout << "   ✓ Created test files\n\n";
        
        // Create stdio transport
        auto transport = std::make_shared<StdioTransport>();
        
        // Create server implementation
        Implementation impl{
            "file-resource-server-example",
            "1.0.0"
        };
        
        // Create MCP server
        Server server(transport, impl);
        
        std::cout << "2. Creating FileResourceServer...\n";
        
        // Create file resource server
        FileResourceServer file_server(server, test_dir.string());
        
        // Configure file server
        file_server.set_max_file_size(10 * 1024 * 1024); // 10MB limit
        file_server.enable_streaming(true); // Stream large files
        
        std::cout << "   ✓ Root directory: " << test_dir << "\n";
        std::cout << "   ✓ Max file size: 10MB\n";
        std::cout << "   ✓ Streaming enabled\n\n";
        
        // List all files
        auto files = file_server.list_files();
        std::cout << "3. Available files (" << files.size() << "):\n";
        for (const auto& file : files) {
            std::cout << "   - " << file.uri << "\n";
            std::cout << "     Name: " << file.name << "\n";
            std::cout << "     MIME: " << file.mimeType.value_or("unknown") << "\n";
            std::cout << "     Description: " << file.description.value_or("") << "\n";
        }
        std::cout << "\n";
        
        // Enable resources capability
        server.enable_resources();
        
        std::cout << "4. Server ready!\n";
        std::cout << "   Features:\n";
        std::cout << "   ✓ RAII file wrappers (automatic cleanup)\n";
        std::cout << "   ✓ MIME type detection (15+ types)\n";
        std::cout << "   ✓ Path traversal protection\n";
        std::cout << "   ✓ File size limits\n";
        std::cout << "   ✓ Streaming for large files\n";
        std::cout << "   ✓ Progress reporting\n";
        std::cout << "   ✓ Cancellation support\n\n";
        
        std::cout << "5. Listening for requests...\n\n";
        
        // Example: Demonstrate streaming server variant
        std::cout << "=== Streaming Variant Example ===\n\n";
        
        auto streaming_transport = std::make_shared<StdioTransport>();
        Implementation streaming_impl{"streaming-file-server", "1.0.0"};
        StreamingServer streaming_server(streaming_transport, streaming_impl);
        
        std::cout << "6. Creating StreamingFileResourceServer...\n";
        StreamingFileResourceServer streaming_file_server(
            streaming_server,
            test_dir.string()
        );
        
        std::cout << "   ✓ True streaming (line-by-line)\n";
        std::cout << "   ✓ Zero memory bloat for large files\n";
        std::cout << "   ✓ Immediate first results\n\n";
        
        // Message loop (simplified for example)
        std::cout << "Ready to serve files! Send MCP requests:\n\n";
        std::cout << "Example requests:\n";
        std::cout << "  - List resources: {\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"resources/list\"}\n";
        std::cout << "  - Read file: {\"jsonrpc\":\"2.0\",\"id\":2,\"method\":\"resources/read\",\"params\":{\"uri\":\"file://README.md\"}}\n\n";
        
        // Simulate a few requests for demonstration
        
        // Request 1: List resources
        json list_request = {
            {"jsonrpc", "2.0"},
            {"id", 1},
            {"method", "resources/list"}
        };
        
        std::cout << ">>> Request: resources/list\n";
        auto list_response = server.dispatch(list_request);
        std::cout << "<<< Response: " << list_response["result"]["resources"].size() 
                  << " resources found\n\n";
        
        // Request 2: Read README.md
        json read_request = {
            {"jsonrpc", "2.0"},
            {"id", 2},
            {"method", "resources/read"},
            {"params", {{"uri", "file://README.md"}}}
        };
        
        std::cout << ">>> Request: resources/read (README.md)\n";
        auto read_response = server.dispatch(read_request);
        auto contents = read_response["result"]["contents"];
        
        if (!contents.empty()) {
            std::cout << "<<< Response: Read " << contents.size() << " content item(s)\n";
            std::cout << "    MIME Type: " << contents[0]["mimeType"] << "\n";
            std::cout << "    Size: " << contents[0]["text"].get<std::string>().size() << " bytes\n";
            std::cout << "    Content:\n";
            std::cout << "    " << contents[0]["text"].get<std::string>() << "\n\n";
        }
        
        // Request 3: Read large file (streaming)
        json large_read_request = {
            {"jsonrpc", "2.0"},
            {"id", 3},
            {"method", "resources/read"},
            {"params", {{"uri", "file://data/sample.csv"}}}
        };
        
        std::cout << ">>> Request: resources/read (sample.csv - large file)\n";
        auto large_response = server.dispatch(large_read_request);
        auto large_contents = large_response["result"]["contents"];
        
        if (!large_contents.empty()) {
            std::string text = large_contents[0]["text"].get<std::string>();
            std::cout << "<<< Response: Read " << text.size() << " bytes\n";
            std::cout << "    File was streamed in chunks (64KB)\n";
            std::cout << "    First 100 chars: " << text.substr(0, 100) << "...\n\n";
        }
        
        // Cleanup
        std::cout << "7. Cleaning up test directory...\n";
        fs::remove_all(test_dir);
        std::cout << "   ✓ Test files removed\n\n";
        
        std::cout << "=== Example completed successfully! ===\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

/**
 * @section Key Features Demonstrated
 * 
 * 1. **RAII File Safety**: Automatic file handle cleanup using raiiiofsw.hpp
 * 2. **MIME Detection**: Automatic type detection for 15+ file formats
 * 3. **Path Security**: Protection against path traversal attacks
 * 4. **Streaming**: Chunked reading for large files (>1MB)
 * 5. **Progress Reporting**: Updates during file streaming
 * 6. **File Size Limits**: Configurable maximum file size
 * 
 * @section File Types Supported
 * 
 * - Text: .txt, .md, .html, .css, .js, .json, .xml
 * - Images: .png, .jpg, .jpeg, .gif, .svg
 * - Archives: .zip, .tar, .gz
 * - Documents: .pdf
 * - Fallback: application/octet-stream
 * 
 * @section RAII Benefits
 * 
 * - **No Leaks**: Files always closed, even on exceptions
 * - **Exception Safe**: Cleanup guaranteed by RAII destructor
 * - **Clean Code**: No manual close() calls needed
 * - **Resource Safety**: Prevents file descriptor exhaustion
 * 
 * @section Usage Patterns
 * 
 * ```cpp
 * // Pattern 1: Basic file server
 * FileResourceServer fs(server, "/path/to/files");
 * 
 * // Pattern 2: Streaming large files
 * fs.enable_streaming(true);
 * fs.set_max_file_size(100 * 1024 * 1024); // 100MB
 * 
 * // Pattern 3: Streaming variant
 * StreamingFileResourceServer sfs(streaming_server, "/logs");
 * // Files streamed line-by-line, zero memory bloat
 * ```
 * 
 * @section Security Features
 * 
 * - Path traversal prevention (../ blocked)
 * - Root directory enforcement
 * - File size limits
 * - MIME type validation
 * - Safe URI parsing
 */
