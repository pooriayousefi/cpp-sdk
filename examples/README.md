# C++ MCP SDK Examples

This directory contains example applications demonstrating the C++ MCP SDK.

## Simple Server Example

`simple_server.cpp` - A minimal MCP server that provides a calculator tool.

### Building

```bash
# From repository root
./builder --debug --executable
```

### Running

```bash
# Start the server
./build/debug/simple_server

# Or use with an MCP client like Claude Desktop
# Add to Claude Desktop config:
{
  "mcpServers": {
    "calculator": {
      "command": "/path/to/cpp-sdk/build/debug/simple_server"
    }
  }
}
```

## Client Example (Coming Soon)

Example client for connecting to MCP servers.

## More Examples

- **Resource Server**: Serve files and data as MCP resources
- **Prompt Server**: Provide prompt templates
- **Multi-Transport**: HTTP/SSE and WebSocket examples
- **Testing**: Unit test examples with in-memory transport
