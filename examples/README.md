# C++ MCP SDK Examples

This directory contains comprehensive examples demonstrating the C++ MCP SDK, including Phase 2 enhancements with async operations, streaming, and RAII file handling.

## Examples Overview

### Phase 1: Basic MCP Protocol

1. **simple_server.cpp** - Minimal MCP server with calculator tool
   - Demonstrates: Basic tool registration, JSON-RPC handling
   - Uses: Server, StdioTransport
   - Complexity: ⭐ Beginner

### Phase 2: Advanced Features

2. **async_client_example.cpp** - Async client using Task<T>
   - Demonstrates: Non-blocking operations, parallel execution
   - Uses: AsyncClient, Task<T>, sync_wait()
   - Complexity: ⭐⭐ Intermediate

3. **streaming_server_example.cpp** - Streaming server using Generator<T>
   - Demonstrates: Incremental results, memory efficiency, progress reporting
   - Uses: StreamingServer, Generator<T>, streaming tools
   - Complexity: ⭐⭐⭐ Advanced

4. **file_resource_server_example.cpp** - File server with RAII
   - Demonstrates: Safe file I/O, MIME detection, resource serving
   - Uses: FileResourceServer, RAIIInputFileStream
   - Complexity: ⭐⭐ Intermediate

5. **http_server_example.cpp** - HTTP/SSE server
   - Demonstrates: HTTP transport, Server-Sent Events, web deployment
   - Uses: HttpServerTransport, SSE notifications
   - Complexity: ⭐⭐⭐ Advanced

## Quick Start

### Building All Examples

```bash
# From repository root
g++ -std=c++23 -I./include examples/simple_server.cpp -o simple_server
g++ -std=c++23 -I./include examples/async_client_example.cpp -o async_client
g++ -std=c++23 -I./include examples/streaming_server_example.cpp -o streaming_server
g++ -std=c++23 -I./include examples/file_resource_server_example.cpp -o file_resource_server
g++ -std=c++23 -I./include examples/http_server_example.cpp -o http_server -pthread
```

**Note**: `http_server_example` requires `cpp-httplib` header (optional dependency).

### Running Examples

#### 1. Simple Server (Stdio Transport)

```bash
./simple_server

# Use with Claude Desktop - add to config:
{
  "mcpServers": {
    "calculator": {
      "command": "/path/to/simple_server"
    }
  }
}
```

#### 2. Async Client

```bash
# Connect to an MCP server
./async_client < server_output.txt
```

#### 3. Streaming Server

```bash
./streaming_server

# Test with MCP client:
# Call streaming tool
echo '{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"stream_dataset","arguments":{"count":1000}}}' | ./streaming_server
```

#### 4. File Resource Server

```bash
./file_resource_server

# Serves files from test directory
# Demonstrates RAII file safety and MIME detection
```

#### 5. HTTP Server

```bash
./http_server

# Server runs on http://localhost:8080
# Test with curl:
curl -X POST http://localhost:8080/jsonrpc \
  -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":1,"method":"tools/list"}'

# Subscribe to SSE notifications:
curl -N http://localhost:8080/events
```

## Example Comparison

| Example | Transport | Async | Streaming | RAII | HTTP | Complexity |
|---------|-----------|-------|-----------|------|------|------------|
| simple_server | Stdio | ❌ | ❌ | ❌ | ❌ | ⭐ |
| async_client | Stdio | ✅ Task<T> | ❌ | ❌ | ❌ | ⭐⭐ |
| streaming_server | Stdio | ❌ | ✅ Generator<T> | ❌ | ❌ | ⭐⭐⭐ |
| file_resource_server | Stdio | ❌ | ✅ | ✅ | ❌ | ⭐⭐ |
| http_server | HTTP | ❌ | ✅ SSE | ❌ | ✅ | ⭐⭐⭐ |

## Feature Matrix

### async_client_example.cpp

**Demonstrates:**
- ✅ Task<T> async operations
- ✅ Parallel tool execution
- ✅ sync_wait() for bridging async/sync
- ✅ Async initialization, tool calls, prompts, resources
- ✅ Chaining async operations

**Key Code:**
```cpp
auto task = client.call_tool_async("echo", args);
auto result = sync_wait(task);

// Parallel execution
auto parallel_task = client.execute_parallel_async(calls);
auto results = sync_wait(parallel_task);
```

### streaming_server_example.cpp

**Demonstrates:**
- ✅ Generator<T> for streaming results
- ✅ Memory-efficient large dataset handling
- ✅ Progress reporting during streaming
- ✅ Cancellation support
- ✅ Real-time sensor data simulation

**Key Code:**
```cpp
Generator<json> stream_data(const json& params) {
    for (int i = 0; i < count; ++i) {
        co_yield json{{"index", i}};
        if (is_canceled()) co_return;
    }
}
```

### file_resource_server_example.cpp

**Demonstrates:**
- ✅ RAII file wrappers (RAIIInputFileStream)
- ✅ Automatic MIME type detection
- ✅ Path traversal protection
- ✅ File size limits
- ✅ Streaming large files
- ✅ Exception-safe file operations

**Key Code:**
```cpp
FileResourceServer fs(server, "/path/to/files");
fs.enable_streaming(true);
fs.set_max_file_size(10 * 1024 * 1024); // 10MB
```

### http_server_example.cpp

**Demonstrates:**
- ✅ HTTP/JSON-RPC transport
- ✅ Server-Sent Events (SSE) for notifications
- ✅ Multi-client support
- ✅ Health check endpoint
- ✅ Cross-origin web access

**Key Code:**
```cpp
HttpServerTransport transport(8080);
transport.start();

// Send SSE notification
transport.send_sse_notification({
    {"method", "notification/message"},
    {"params", {...}}
});
```

## Phase 2 Core Utilities

All Phase 2 examples leverage core utilities from `include/mcp/core/`:

### asyncops.hpp
- `Generator<T>`: Lazy evaluation, streaming results
- `Task<T>`: Async/await coroutines
- `GeneratorFactory<T,N>`: Object pooling
- `sync_wait()`: Bridge async/sync code

### raiiiofsw.hpp
- `RAIIInputFileStream`: Auto-closing input files
- `RAIIOutputFileStream`: Auto-closing output files
- Exception-safe file operations

### stringformers.hpp
- String validation and transformations
- URI parsing and normalization

### utilities.hpp
- Diagnostics and debugging helpers
- Performance measurement

## Dependencies

### Required (Bundled)
- nlohmann/json (bundled in `include/mcp/jsonrpc/json.hpp`)

### Optional (Examples Only)
- cpp-httplib (for `http_server_example.cpp` only)
  - Download: https://github.com/yhirose/cpp-httplib
  - Single header: `httplib.h`
  - Place in `include/` or system include path

## Testing Examples

### With Claude Desktop

All stdio-based examples work with Claude Desktop:

```json
{
  "mcpServers": {
    "calculator": {
      "command": "/path/to/simple_server"
    },
    "streaming": {
      "command": "/path/to/streaming_server"
    },
    "files": {
      "command": "/path/to/file_resource_server"
    }
  }
}
```

### With MCP Inspector

```bash
# Install MCP Inspector
npm install -g @modelcontextprotocol/inspector

# Test server
mcp-inspector ./simple_server
```

### Manual Testing

```bash
# Stdio transport (pipe JSON-RPC)
echo '{"jsonrpc":"2.0","id":1,"method":"tools/list"}' | ./simple_server

# HTTP transport (curl)
curl -X POST http://localhost:8080/jsonrpc \
  -H 'Content-Type: application/json' \
  -d '{"jsonrpc":"2.0","id":1,"method":"tools/list"}'
```

## Learning Path

### Beginner
1. Start with `simple_server.cpp`
2. Understand basic MCP protocol
3. Learn tool registration

### Intermediate
1. Study `async_client_example.cpp` for Task<T>
2. Explore `file_resource_server_example.cpp` for RAII
3. Understand sync/async bridging

### Advanced
1. Master `streaming_server_example.cpp` for Generator<T>
2. Deploy `http_server_example.cpp` for production
3. Combine features for custom servers

## Common Patterns

### Pattern 1: Basic Server
```cpp
auto transport = std::make_shared<StdioTransport>();
Server server(transport, impl);
server.register_tool(tool, handler);
server.enable_tools();
// Message loop
```

### Pattern 2: Async Client
```cpp
AsyncClient client(transport);
auto task = client.call_tool_async(name, args);
auto result = sync_wait(task);
```

### Pattern 3: Streaming Server
```cpp
StreamingServer server(transport, impl);
server.register_streaming_tool(tool, [](params) -> Generator<json> {
    for (auto item : dataset) {
        co_yield item;
    }
});
```

### Pattern 4: File Resources
```cpp
FileResourceServer fs(server, root_dir);
fs.enable_streaming(true);
// Files automatically served as resources
```

### Pattern 5: HTTP Deployment
```cpp
HttpServerTransport transport(8080);
Server server(transport, impl);
transport.start();
// HTTP server running
```

## Troubleshooting

### Compile Errors

**Error**: `httplib.h not found`
- **Solution**: Download cpp-httplib or skip HTTP example

**Error**: `<coroutine> not found`
- **Solution**: Use GCC 13+ or Clang 16+ with C++23 support

**Error**: Linker error with pthread
- **Solution**: Add `-pthread` flag for HTTP example

### Runtime Errors

**Error**: "Connection refused"
- **Solution**: Ensure server is running before client connects

**Error**: "File not found"
- **Solution**: Check file paths and permissions for file server

**Error**: "Port already in use"
- **Solution**: Change HTTP server port or kill existing process

## Next Steps

- Combine examples to build custom MCP servers
- Add your own tools, prompts, and resources
- Deploy HTTP server to cloud (AWS, GCP, Azure)
- Integrate with Claude Desktop or other MCP clients
- Contribute examples back to the SDK!

## Support

- See `ALWAYS_REMEMBER_HOW_TO_USE_CORE_HEADERS.md` for detailed utility usage
- Check `README.md` in project root for SDK documentation
- Report issues: https://github.com/pooriayousefi/cpp-sdk/issues
