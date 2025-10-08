# ALWAYS REMEMBER: How to Use Core Headers in MCP SDK

This document explains **exactly how** to use the core utilities (`asyncops.hpp`, `raiiiofsw.hpp`, `stringformers.hpp`, `utilities.hpp`) within the MCP SDK. These utilities are **essential** for building production-ready, high-performance MCP servers and clients.

---

## 📋 Table of Contents

1. [asyncops.hpp - Asynchronous Operations](#asyncopshpp---asynchronous-operations)
2. [raiiiofsw.hpp - RAII File I/O](#raiiiofswhpp---raii-file-io)
3. [stringformers.hpp - String Transformations](#stringformershpp---string-transformations)
4. [utilities.hpp - General Utilities](#utilitieshpp---general-utilities)
5. [Integration Patterns](#integration-patterns)
6. [Best Practices](#best-practices)

---

## 1. asyncops.hpp - Asynchronous Operations

**Location**: `include/mcp/core/asyncops.hpp`  
**Namespace**: `pooriayousefi::core`

### 🔥 Critical Components

#### **1.1 Generator&lt;T&gt; - Coroutine-Based Lazy Generation**

**When to Use**:
- Streaming large tool results incrementally
- Processing files line-by-line for resources
- Generating paginated results
- Avoiding memory overhead for large datasets

**MCP Applications**:

##### **Streaming Tool Results**
```cpp
#include <mcp/server.hpp>
#include <mcp/core/asyncops.hpp>

using namespace pooriayousefi::mcp;
using namespace pooriayousefi::core;

// Tool that streams results instead of building entire response in memory
Generator<ToolResultContent> search_large_database(const std::string& query) {
    // Open connection to database
    auto results = database.execute(query);
    
    // Yield results one at a time (lazy evaluation)
    for (const auto& row : results) {
        std::string formatted = format_result(row);
        co_yield ToolResultContent::text_content(formatted);
        
        // Check cancellation between results
        if (jsonrpc::is_canceled()) {
            co_return;
        }
    }
}

// Register with streaming server
server.register_streaming_tool("search", search_large_database);
```

##### **Streaming File Resources**
```cpp
Generator<std::string> stream_log_file(const std::string& path) {
    std::ifstream file(path);
    std::string line;
    
    while (std::getline(file, line)) {
        co_yield line;
    }
}

// Use in resource handler
server.register_resource(log_resource, [](const std::string& uri) {
    std::string content;
    for (const auto& line : stream_log_file(uri)) {
        content += line + "\n";
        
        // Report progress while reading
        if (auto ctx = jsonrpc::current_context()) {
            jsonrpc::report_progress({{"lines", content.size()}});
        }
    }
    return std::vector<ResourceContent>{{uri, "text/plain", content, std::nullopt}};
});
```

##### **Paginated Tool Results**
```cpp
Generator<json> paginate_api_results(const std::string& endpoint, int page_size = 100) {
    int page = 0;
    bool has_more = true;
    
    while (has_more) {
        auto response = http_get(endpoint + "?page=" + std::to_string(page));
        auto items = response["items"];
        
        for (const auto& item : items) {
            co_yield item;
        }
        
        has_more = response.value("has_more", false);
        page++;
    }
}
```

#### **1.2 GeneratorFactory&lt;T, N&gt; - Object Pool Management**

**When to Use**:
- Managing reusable HTTP connections
- Pool of database connections
- Reusable parser objects
- Buffer pools for large data processing

**MCP Applications**:

##### **HTTP Connection Pool**
```cpp
#include <mcp/core/asyncops.hpp>

using namespace pooriayousefi::core;

// Pool of 10 HTTP clients for parallel requests
GeneratorFactory<HttpClient, 10> http_pool;

// In tool handler - get client from pool
Generator<std::shared_ptr<HttpClient>> get_http_client() {
    co_return co_await http_pool.generate();
}

// Tool that uses pooled connection
server.register_tool("fetch_api", [](const json& args) {
    auto client = http_pool.acquire(); // Get from pool
    auto response = client->get(args["url"]);
    // Client automatically returns to pool when destroyed
    return std::vector<ToolResultContent>{
        ToolResultContent::text_content(response.body())
    };
});
```

##### **Database Connection Pool**
```cpp
GeneratorFactory<DatabaseConnection, 5> db_pool;

server.register_tool("query_db", [&db_pool](const json& args) {
    // Pool ensures we never exceed 5 concurrent connections
    for (auto conn : db_pool.generate()) {
        auto results = conn->execute(args["query"].get<std::string>());
        return format_results(results);
    }
});
```

#### **1.3 Task&lt;T&gt; - Awaitable Asynchronous Tasks**

**When to Use**:
- Async client operations (non-blocking API)
- Concurrent tool execution
- Parallel resource fetching
- Async I/O operations

**MCP Applications**:

##### **Async Client API**
```cpp
#include <mcp/client.hpp>
#include <mcp/core/asyncops.hpp>

using namespace pooriayousefi::mcp;
using namespace pooriayousefi::core;

// Async version of client operations
class AsyncClient {
public:
    // Non-blocking tool listing
    Task<std::vector<Tool>> list_tools_async() {
        std::promise<std::vector<Tool>> promise;
        auto future = promise.get_future();
        
        client_.list_tools(
            [&promise](const std::vector<Tool>& tools) {
                promise.set_value(tools);
            },
            [&promise](const std::string& error) {
                promise.set_exception(std::make_exception_ptr(
                    std::runtime_error(error)
                ));
            }
        );
        
        co_return future.get();
    }
    
    // Non-blocking tool call
    Task<std::vector<ToolResultContent>> call_tool_async(
        const std::string& name, 
        const json& args
    ) {
        std::promise<std::vector<ToolResultContent>> promise;
        auto future = promise.get_future();
        
        client_.call_tool(
            name, 
            args,
            [&promise](const std::vector<ToolResultContent>& result) {
                promise.set_value(result);
            },
            [&promise](const std::string& error) {
                promise.set_exception(std::make_exception_ptr(
                    std::runtime_error(error)
                ));
            }
        );
        
        co_return future.get();
    }

private:
    Client& client_;
};

// Usage with async/await syntax
Task<void> example_async_usage(AsyncClient& client) {
    // Await multiple operations concurrently
    auto tools = co_await client.list_tools_async();
    
    std::cout << "Found " << tools.size() << " tools\n";
    
    // Call first tool
    if (!tools.empty()) {
        auto result = co_await client.call_tool_async(
            tools[0].name, 
            json::object()
        );
        
        for (const auto& content : result) {
            if (content.text) {
                std::cout << "Result: " << *content.text << "\n";
            }
        }
    }
}
```

##### **Parallel Tool Execution**
```cpp
Task<std::vector<json>> execute_tools_parallel(
    Client& client,
    const std::vector<std::string>& tool_names
) {
    std::vector<Task<ToolResult>> tasks;
    
    // Launch all tools concurrently
    for (const auto& name : tool_names) {
        tasks.push_back(client.call_tool_async(name, json::object()));
    }
    
    // Wait for all to complete
    std::vector<json> results;
    for (auto& task : tasks) {
        results.push_back(co_await task);
    }
    
    co_return results;
}
```

#### **1.4 sync_wait() - Synchronous Waiting on Async Tasks**

**When to Use**:
- Bridge between async and sync code
- Testing async functions
- Main function that needs to wait for async tasks

**MCP Applications**:

```cpp
#include <mcp/core/asyncops.hpp>

using namespace pooriayousefi::core;

int main() {
    Client client(transport);
    AsyncClient async_client(client);
    
    // Synchronously wait for async operation
    auto result = sync_wait(async_client.list_tools_async());
    
    std::cout << "Got " << result.size() << " tools\n";
    
    return 0;
}
```

---

## 2. raiiiofsw.hpp - RAII File I/O

**Location**: `include/mcp/core/raiiiofsw.hpp`  
**Namespace**: `pooriayousefi::core`

### 🔥 When to Use

- **File-based resources**: Serve files as MCP resources
- **Configuration loading**: Read server config files safely
- **Logging**: MCP server logging with automatic flush
- **Data persistence**: Save/load server state

### MCP Applications

#### **2.1 File Resource Server**
```cpp
#include <mcp/server.hpp>
#include <mcp/core/raiiiofsw.hpp>

using namespace pooriayousefi::mcp;
using namespace pooriayousefi::core;

class FileResourceServer {
public:
    FileResourceServer(Server& server, const std::string& root_dir) 
        : server_(server), root_dir_(root_dir) 
    {
        // Register file:// URI pattern
        ResourceTemplate file_template{
            "file://" + root_dir_ + "/{path}",
            "Local files",
            "Access files from local filesystem",
            std::nullopt
        };
        
        server_.register_resource_template(file_template);
    }
    
    std::vector<ResourceContent> read_file_resource(const std::string& uri) {
        // Parse URI to get file path
        std::string path = parse_file_uri(uri);
        
        // Use RAII file wrapper - automatically closes on exception or return
        RAIIInputFileStream file(path);
        
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + path);
        }
        
        // Read entire file safely
        std::string content = file.read_all();
        
        // Detect MIME type
        std::string mime = detect_mime_type(path);
        
        return {ResourceContent{uri, mime, content, std::nullopt}};
        
        // File automatically closed here (RAII)
    }

private:
    Server& server_;
    std::string root_dir_;
};

// Usage
Server server(transport, impl);
FileResourceServer file_server(server, "/home/user/documents");
```

#### **2.2 Configuration Loading**
```cpp
#include <mcp/core/raiiiofsw.hpp>

using namespace pooriayousefi::core;

class ServerConfig {
public:
    static ServerConfig load(const std::string& config_path) {
        RAIIInputFileStream file(config_path);
        
        if (!file.is_open()) {
            // Use default config
            return ServerConfig();
        }
        
        std::string json_str = file.read_all();
        auto config_json = json::parse(json_str);
        
        return ServerConfig::from_json(config_json);
        
        // File automatically closed
    }
    
    void save(const std::string& config_path) {
        RAIIOutputFileStream file(config_path);
        
        if (!file.is_open()) {
            throw std::runtime_error("Failed to save config");
        }
        
        file.write(to_json().dump(2));
        
        // File automatically flushed and closed
    }
};
```

#### **2.3 Server Logging**
```cpp
#include <mcp/core/raiiiofsw.hpp>

class MCPLogger {
public:
    MCPLogger(const std::string& log_path) 
        : log_file_(log_path, std::ios::app) // Append mode
    {
        if (!log_file_.is_open()) {
            std::cerr << "Warning: Could not open log file\n";
        }
    }
    
    void log(const std::string& level, const std::string& message) {
        if (!log_file_.is_open()) return;
        
        auto timestamp = get_timestamp();
        log_file_.write(timestamp + " [" + level + "] " + message + "\n");
        log_file_.flush(); // Ensure written
    }

private:
    RAIIOutputFileStream log_file_;
};

// Use in server
Server server(transport, impl);
MCPLogger logger("/var/log/mcp-server.log");

server.on_error([&logger](const std::string& error) {
    logger.log("ERROR", error);
});
```

---

## 3. stringformers.hpp - String Transformations

**Location**: `include/mcp/core/stringformers.hpp`  
**Namespace**: `pooriayousefi::core`

### 🔥 When to Use

- **URI parsing**: Parse `file://`, `http://`, custom URI schemes
- **Parameter validation**: Clean and validate tool arguments
- **Response formatting**: Format tool results for display
- **Path manipulation**: Handle file paths in resources

### MCP Applications

#### **3.1 URI Parsing for Resources**
```cpp
#include <mcp/core/stringformers.hpp>

using namespace pooriayousefi::core;

class URIParser {
public:
    static std::map<std::string, std::string> parse_uri(const std::string& uri) {
        // Use stringformers utilities to parse
        std::map<std::string, std::string> parts;
        
        // Extract scheme (e.g., "file://")
        parts["scheme"] = extract_scheme(uri);
        
        // Extract path
        parts["path"] = extract_path(uri);
        
        // Extract query parameters
        parts["query"] = extract_query(uri);
        
        return parts;
    }
};

// Use in resource handler
server.register_resource(resource, [](const std::string& uri) {
    auto parts = URIParser::parse_uri(uri);
    
    if (parts["scheme"] == "file") {
        return read_file(parts["path"]);
    } else if (parts["scheme"] == "http") {
        return fetch_http(uri);
    }
    
    throw std::runtime_error("Unsupported URI scheme");
});
```

#### **3.2 Tool Parameter Validation**
```cpp
#include <mcp/core/stringformers.hpp>

class ParameterValidator {
public:
    static std::string validate_email(const std::string& email) {
        // Use stringformers to clean and validate
        std::string cleaned = trim(to_lowercase(email));
        
        if (!is_valid_email(cleaned)) {
            throw std::invalid_argument("Invalid email format");
        }
        
        return cleaned;
    }
    
    static std::string validate_path(const std::string& path) {
        // Normalize path separators
        std::string normalized = normalize_path(path);
        
        // Remove dangerous sequences like "../"
        if (contains(normalized, "..")) {
            throw std::invalid_argument("Path traversal not allowed");
        }
        
        return normalized;
    }
};

// Use in tool handler
server.register_tool("send_email", [](const json& args) {
    std::string email = ParameterValidator::validate_email(
        args["to"].get<std::string>()
    );
    
    send_email(email, args["subject"], args["body"]);
    
    return std::vector<ToolResultContent>{
        ToolResultContent::text_content("Email sent to " + email)
    };
});
```

#### **3.3 Response Formatting**
```cpp
#include <mcp/core/stringformers.hpp>

class ResponseFormatter {
public:
    static std::string format_table(const std::vector<std::vector<std::string>>& data) {
        if (data.empty()) return "";
        
        // Calculate column widths
        std::vector<size_t> widths = calculate_widths(data);
        
        // Format as ASCII table
        std::string result;
        for (const auto& row : data) {
            for (size_t i = 0; i < row.size(); ++i) {
                result += pad_right(row[i], widths[i]) + " | ";
            }
            result += "\n";
        }
        
        return result;
    }
};

// Use in tool
server.register_tool("list_users", [](const json& args) {
    auto users = database.get_users();
    
    std::vector<std::vector<std::string>> table = {
        {"ID", "Name", "Email"}
    };
    
    for (const auto& user : users) {
        table.push_back({
            std::to_string(user.id),
            user.name,
            user.email
        });
    }
    
    std::string formatted = ResponseFormatter::format_table(table);
    
    return std::vector<ToolResultContent>{
        ToolResultContent::text_content(formatted)
    };
});
```

---

## 4. utilities.hpp - General Utilities

**Location**: `include/mcp/core/utilities.hpp`  
**Namespace**: `pooriayousefi::core`

### 🔥 When to Use

- **Error handling**: Enhanced error messages with context
- **Data validation**: Input sanitization and validation
- **Performance**: Timing and profiling tool execution
- **Debugging**: Diagnostic utilities

### MCP Applications

#### **4.1 Enhanced Error Handling**
```cpp
#include <mcp/core/utilities.hpp>

using namespace pooriayousefi::core;

class ToolErrorHandler {
public:
    static void handle_tool_error(
        const std::string& tool_name,
        const std::exception& e,
        const json& args
    ) {
        // Use utilities to create detailed error message
        std::string error_msg = format_error(
            "Tool execution failed",
            {
                {"tool", tool_name},
                {"error", e.what()},
                {"arguments", args.dump()},
                {"timestamp", get_timestamp()}
            }
        );
        
        // Log with full context
        log_error(error_msg);
        
        // Report to client
        throw jsonrpc::rpc_exception(jsonrpc::error{
            -32603,
            "Tool failed: " + std::string(e.what()),
            json{{"tool", tool_name}, {"details", error_msg}}
        });
    }
};

// Use in server
server.register_tool("risky_tool", [](const json& args) {
    try {
        return execute_risky_operation(args);
    } catch (const std::exception& e) {
        ToolErrorHandler::handle_tool_error("risky_tool", e, args);
        throw; // Unreachable but satisfies compiler
    }
});
```

#### **4.2 Tool Execution Timing**
```cpp
#include <mcp/core/utilities.hpp>

class ToolProfiler {
public:
    template<typename Func>
    static auto profile_tool(const std::string& tool_name, Func&& func) {
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            auto result = func();
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                end - start
            ).count();
            
            // Report timing via progress
            jsonrpc::report_progress({
                {"tool", tool_name},
                {"execution_time_ms", duration}
            });
            
            return result;
            
        } catch (...) {
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                end - start
            ).count();
            
            log_error("Tool " + tool_name + " failed after " + 
                     std::to_string(duration) + "ms");
            throw;
        }
    }
};

// Use with tools
server.register_tool("slow_tool", [](const json& args) {
    return ToolProfiler::profile_tool("slow_tool", [&]() {
        // Expensive operation
        auto result = expensive_computation(args);
        return format_result(result);
    });
});
```

---

## 5. Integration Patterns

### Pattern 1: Async Client with Streaming Results

```cpp
#include <mcp/client.hpp>
#include <mcp/core/asyncops.hpp>

using namespace pooriayousefi::mcp;
using namespace pooriayousefi::core;

// Async client that streams results
Task<void> process_large_query(AsyncClient& client) {
    // Start async tool call
    auto result_stream = co_await client.call_tool_streaming_async(
        "search_database",
        {{"query", "SELECT * FROM large_table"}}
    );
    
    // Process results as they arrive
    for (const auto& chunk : result_stream) {
        process_chunk(chunk);
        
        // Report progress
        jsonrpc::report_progress({
            {"processed", chunk.size()}
        });
    }
}
```

### Pattern 2: Server with File Resources + Async Tools

```cpp
#include <mcp/server.hpp>
#include <mcp/core/asyncops.hpp>
#include <mcp/core/raiiiofsw.hpp>

using namespace pooriayousefi::mcp;
using namespace pooriayousefi::core;

class AdvancedMCPServer {
public:
    AdvancedMCPServer(const std::string& data_dir) 
        : transport_(std::make_shared<transport::StdioTransport>())
        , server_(transport_, Implementation{"advanced-server", "1.0.0"})
        , data_dir_(data_dir)
    {
        setup_resources();
        setup_tools();
    }
    
    void setup_resources() {
        // Use RAII file I/O for file resources
        server_.register_resource(
            Resource{"file://" + data_dir_, "Data Directory", "Server data", std::nullopt},
            [this](const std::string& uri) {
                return read_file_resource(uri);
            }
        );
    }
    
    void setup_tools() {
        // Use Generator for streaming results
        server_.register_streaming_tool(
            Tool{"process_log", "Process log file", {}},
            [this](const json& args) {
                return process_log_file(args["path"]);
            }
        );
    }
    
    Generator<ToolResultContent> process_log_file(const std::string& path) {
        RAIIInputFileStream file(data_dir_ + "/" + path);
        std::string line;
        
        while (std::getline(file.stream(), line)) {
            // Process line
            auto processed = analyze_log_line(line);
            co_yield ToolResultContent::text_content(processed);
        }
    }
    
    std::vector<ResourceContent> read_file_resource(const std::string& uri) {
        std::string path = extract_path(uri);
        RAIIInputFileStream file(path);
        
        return {ResourceContent{
            uri,
            detect_mime(path),
            file.read_all(),
            std::nullopt
        }};
    }

private:
    std::shared_ptr<transport::Transport> transport_;
    Server server_;
    std::string data_dir_;
};
```

### Pattern 3: HTTP Transport with Connection Pool

```cpp
#include <mcp/transport/http_transport.hpp>
#include <mcp/core/asyncops.hpp>

using namespace pooriayousefi::mcp;
using namespace pooriayousefi::core;

class HTTPTransportWithPool : public transport::Transport {
public:
    HTTPTransportWithPool(const std::string& endpoint)
        : endpoint_(endpoint)
    {
        // Pool of 10 HTTP connections
        // (Connection pool implementation using GeneratorFactory)
    }
    
    Task<void> send_async(const json& message) {
        // Get connection from pool
        auto conn = co_await connection_pool_.acquire();
        
        // Send message
        co_await conn->post(endpoint_, message.dump());
        
        // Connection returns to pool automatically
    }

private:
    std::string endpoint_;
    GeneratorFactory<HttpConnection, 10> connection_pool_;
};
```

---

## 6. Best Practices

### ✅ DO:

1. **Use Generator&lt;T&gt; for streaming**
   - Any tool that processes > 1MB of data
   - File reading operations
   - Database query results
   - API pagination

2. **Use Task&lt;T&gt; for async operations**
   - Network I/O (HTTP requests)
   - Database queries
   - File I/O (large files)
   - Parallel tool execution

3. **Use RAII file wrappers**
   - Always for file resources
   - Configuration loading
   - Logging operations
   - Temporary file handling

4. **Use stringformers for validation**
   - URI parsing
   - Parameter cleaning
   - Path normalization
   - Input sanitization

5. **Use utilities for diagnostics**
   - Error context building
   - Performance profiling
   - Debug logging

### ❌ DON'T:

1. **Don't load entire files into memory**
   - ❌ `std::string content = read_all(huge_file)`
   - ✅ `Generator<std::string> stream_file(huge_file)`

2. **Don't block on I/O**
   - ❌ `auto result = blocking_http_get(url)`
   - ✅ `auto result = co_await async_http_get(url)`

3. **Don't forget RAII**
   - ❌ `std::ifstream f; /* manual close */ f.close();`
   - ✅ `RAIIInputFileStream f(path); // automatic`

4. **Don't skip validation**
   - ❌ `process_path(user_input)` // Path traversal!
   - ✅ `process_path(validate_path(user_input))`

5. **Don't ignore context**
   - ❌ `throw std::runtime_error("Failed")`
   - ✅ `throw with_context("Failed", {{"tool", name}, {"args", args}})`

---

## 🎯 Quick Reference

| Need to... | Use... | Example |
|------------|--------|---------|
| Stream large results | `Generator<T>` | `Generator<ToolResult> stream_data()` |
| Pool connections | `GeneratorFactory<T, N>` | `GeneratorFactory<HttpClient, 10>` |
| Async client calls | `Task<T>` | `Task<Tools> list_tools_async()` |
| Wait for async | `sync_wait()` | `auto result = sync_wait(async_func())` |
| Read files safely | `RAIIInputFileStream` | `RAIIInputFileStream f(path)` |
| Parse URIs | stringformers | `auto parts = parse_uri(uri)` |
| Validate input | stringformers | `auto clean = validate_path(input)` |
| Add error context | utilities | `throw with_context(err, context)` |
| Profile tools | utilities | `profile_tool(name, func)` |

---

## 🚀 Remember

These core utilities are **not optional** for production MCP servers. They provide:

- ✅ **Performance**: Streaming prevents memory bloat
- ✅ **Safety**: RAII prevents resource leaks
- ✅ **Security**: Validation prevents attacks
- ✅ **Reliability**: Error context aids debugging
- ✅ **Scalability**: Async operations + pooling = high throughput

**Always use them. They're there for a reason.** 💪
