# Next Steps - Development Roadmap

This document outlines the immediate next steps for building the C++ MCP SDK.

## ✅ Foundation Complete

The project foundation is now professionally established with:
- Proper naming (cpp-sdk)
- MIT License + AUTHORS file
- Professional README
- Dependencies strategy
- Clear namespace design

## 📋 Immediate Tasks (This Week)

### 1. Add Namespace to Existing Headers

Update all files in `include/core/` to use `pooriayousefi::core` namespace:

```cpp
// Copyright 2025 The C++ MCP SDK Authors. All rights reserved.
// Use of this source code is governed by an MIT-style
// license that can be found in the LICENSE file.

#ifndef POORIAYOUSEFI_CORE_ASYNCOPS_HPP
#define POORIAYOUSEFI_CORE_ASYNCOPS_HPP

namespace pooriayousefi::core
{
    // ... existing code ...
}

#endif // POORIAYOUSEFI_CORE_ASYNCOPS_HPP
```

Files to update:
- [ ] `include/core/asyncops.hpp`
- [ ] `include/core/raiiiofsw.hpp`
- [ ] `include/core/stringformers.hpp`
- [ ] `include/core/utilities.hpp`

### 2. Set Up Dependencies

Choose ONE installation method:

**Option A: Git Submodules (Recommended)**
```bash
mkdir external
git submodule add https://github.com/nlohmann/json.git external/json
git submodule add https://github.com/chriskohlhoff/asio.git external/asio
git submodule add https://github.com/yhirose/cpp-httplib.git external/httplib
```

**Option B: Manual Download**
```bash
mkdir -p external/json external/asio external/httplib
# Download single headers to external/
```

### 3. Create Core MCP Directory Structure

```bash
mkdir -p include/pooriayousefi/mcp/transport
mkdir -p include/pooriayousefi/mcp/internal
```

### 4. Implement Protocol Types

Create `include/pooriayousefi/mcp/types.hpp`:

```cpp
// Copyright 2025 The C++ MCP SDK Authors.
#ifndef POORIAYOUSEFI_MCP_TYPES_HPP
#define POORIAYOUSEFI_MCP_TYPES_HPP

#include <nlohmann/json.hpp>
#include <string>
#include <optional>
#include <variant>

namespace pooriayousefi::mcp
{
    using json = nlohmann::json;
    
    // MCP Protocol Version
    inline constexpr const char* PROTOCOL_VERSION = "2024-11-05";
    
    // JSON-RPC 2.0 Types
    struct JsonRpcRequest
    {
        std::string jsonrpc = "2.0";
        std::variant<std::string, int> id;
        std::string method;
        std::optional<json> params;
    };
    
    struct JsonRpcResponse
    {
        std::string jsonrpc = "2.0";
        std::variant<std::string, int> id;
        std::optional<json> result;
        std::optional<json> error;
    };
    
    // MCP Initialize Types
    struct Implementation
    {
        std::string name;
        std::string version;
    };
    
    struct ServerInfo
    {
        Implementation implementation;
        std::optional<json> capabilities;
    };
    
    // Add more types as needed...
}

#endif // POORIAYOUSEFI_MCP_TYPES_HPP
```

## 📅 Week 2 Tasks

### 5. Transport Interface

Create `include/pooriayousefi/mcp/transport.hpp`:
- Abstract Transport interface
- Message send/receive
- Connection lifecycle

### 6. Stdio Transport Implementation

Create `include/pooriayousefi/mcp/transport/stdio.hpp`:
- Read from stdin
- Write to stdout
- Line-based messaging

### 7. Basic Server Implementation

Create `include/pooriayousefi/mcp/server.hpp`:
- Server builder pattern
- Tool registration
- Message handling
- Initialization handshake

## 📅 Week 3-4 Tasks

### 8. Basic Client Implementation

Create `include/pooriayousefi/mcp/client.hpp`:
- Client connection
- Tool calling
- Resource reading

### 9. First Working Example

Create `examples/basic_server/main.cpp`:
- Simple greeting server
- One tool implementation
- stdio transport

### 10. Testing Infrastructure

```bash
mkdir tests/unit
# Add Catch2 tests
```

## 🎯 Success Criteria

### Milestone 1: Hello World (Week 2)
- [ ] Server can initialize over stdio
- [ ] Server exposes one simple tool
- [ ] Client can connect and call the tool
- [ ] Example compiles and runs

### Milestone 2: Core Features (Week 4)
- [ ] Multiple tools supported
- [ ] Resource reading implemented
- [ ] Error handling works
- [ ] Basic tests passing

### Milestone 3: Community Ready (Month 2)
- [ ] HTTP/SSE transport working
- [ ] 5+ examples
- [ ] Documentation complete
- [ ] Ready for early adopters

## 🚀 Quick Start Development

To start coding immediately:

```bash
cd cpp-sdk

# 1. Add copyright headers to existing files
# 2. Install dependencies (see DEPENDENCIES.md)
# 3. Create first MCP type definitions
# 4. Implement stdio transport
# 5. Build simple server example

# Test the build system
g++ -std=c++23 builder.cpp -o builder
./builder --release --executable
```

## 📚 Study These First

Before coding, read these official SDK implementations:

1. **Go SDK**: https://github.com/modelcontextprotocol/go-sdk
   - Simple, clean architecture
   - Good transport abstraction
   
2. **Rust SDK**: https://github.com/modelcontextprotocol/rust-sdk
   - Type-safe design patterns
   - Async/await patterns

3. **MCP Specification**: https://spec.modelcontextprotocol.io/
   - Protocol details
   - Message formats
   - Required capabilities

## 💡 Development Tips

1. **Start Simple**: Begin with the minimal viable implementation
2. **Header-Only**: Keep everything in headers for now
3. **Test Early**: Write tests alongside code
4. **Document**: Add comments and examples as you go
5. **Iterate**: Don't aim for perfection in first version

## 🤝 When to Share

Share early and often:
- Week 2: Share with C++ communities (Reddit, Discord)
- Week 4: Post in MCP Discussions
- Month 2: Blog post about the project
- Month 3: Propose to modelcontextprotocol org

---

**Remember**: Quality over speed. Build something you're proud of! 🚀
