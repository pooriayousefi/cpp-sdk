# C++ MCP SDK

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++23](https://img.shields.io/badge/C++-23-blue.svg)](https://en.cppreference.com/w/cpp/23)
[![Header-Only](https://img.shields.io/badge/header--only-yes-success.svg)](DEPENDENCIES.md)

A modern, header-only C++ implementation of the [Model Context Protocol](https://modelcontextprotocol.io/) (MCP).

> **Note**: This SDK is currently in early development. It aims to become the official C++ SDK for MCP through community collaboration.

## What is MCP?

The Model Context Protocol (MCP) is an open protocol that enables seamless integration between LLM applications and external data sources and tools. Whether you're building an AI-powered IDE, enhancing a chat interface, or creating custom AI workflows, MCP provides a standardized way to connect LLMs with the context they need.

## Why C++ MCP SDK?

- 🚀 **Performance**: Native C++ performance for latency-critical applications
- 🎮 **Native Integration**: Perfect for game engines, CAD software, scientific computing
- 🔧 **System-Level Control**: Fine-grained control over memory, threading, and I/O
- 📦 **Header-Only**: Minimal dependencies, easy integration
- 🌐 **Cross-Platform**: Works on Linux, macOS, Windows, and embedded systems
- ⚡ **Modern C++23**: Leverages coroutines, ranges, and modules

## Features

### Current Status (v0.1.0-alpha)

- [x] Project structure and build system
- [x] Core utilities (async ops, RAII wrappers, string formatting)
- [x] Pythonic naming conventions
- [ ] MCP Protocol Types (JSON-RPC 2.0)
- [ ] Transport Layer (stdio, HTTP/SSE)
- [ ] Client Implementation
- [ ] Server Implementation
- [ ] Resource Management
- [ ] Tool/Prompt Support

### Planned Features

- [ ] WebSocket transport
- [ ] Sampling support
- [ ] Comprehensive examples
- [ ] Full test coverage
- [ ] Documentation site

## Quick Start

### Prerequisites

- **Compiler**: GCC 13+, Clang 16+, or MSVC 19.34+ (C++23 support)
- **Platform**: Linux, macOS, or Windows
- **Dependencies**: See [DEPENDENCIES.md](DEPENDENCIES.md)

### Installation

```bash
# Clone the repository
git clone https://github.com/pooriayousefi/cpp-sdk.git
cd cpp-sdk

# Initialize dependencies (git submodules)
git submodule update --init --recursive

# Build the project
g++ -std=c++23 builder.cpp -o builder
./builder --release --executable
```

### Basic Usage (Coming Soon)

```cpp
#include <pooriayousefi/mcp/server.hpp>
#include <pooriayousefi/mcp/transport/stdio.hpp>

using namespace pooriayousefi::mcp;

int main() {
    // Create MCP server
    auto server = Server::builder()
        .name("my-cpp-server")
        .version("1.0.0")
        .build();
    
    // Add a tool
    server.add_tool("greet", [](const json& args) {
        return json{{"message", "Hello, " + args["name"].get<std::string>()}};
    });
    
    // Run over stdio
    server.run(transport::Stdio{});
    
    return 0;
}
```

## Project Structure

```
cpp-sdk/
├── include/
│   ├── pooriayousefi/
│   │   └── mcp/
│   │       ├── client.hpp          # MCP client
│   │       ├── server.hpp          # MCP server
│   │       ├── protocol.hpp        # Protocol types
│   │       ├── transport/          # Transport implementations
│   │       │   ├── stdio.hpp       # Stdio transport
│   │       │   └── sse.hpp         # HTTP/SSE transport
│   │       └── types.hpp           # Core MCP types
│   └── core/                       # Utility headers
│       ├── asyncops.hpp            # Async operations & coroutines
│       ├── raiiiofsw.hpp           # RAII filesystem wrappers
│       ├── stringformers.hpp       # String formatting
│       └── utilities.hpp           # General utilities
├── examples/                       # Example implementations
├── tests/                          # Unit tests
├── external/                       # External dependencies (submodules)
├── docs/                           # Documentation
├── AUTHORS                         # Project contributors
├── LICENSE                         # MIT License
├── DEPENDENCIES.md                 # Dependency information
└── README.md                       # This file
```

## Design Philosophy

### Namespace Structure

All code is organized under the `pooriayousefi` namespace:

```cpp
pooriayousefi::mcp::core      // Core protocol types
pooriayousefi::mcp::transport // Transport implementations  
pooriayousefi::mcp::client    // Client implementation
pooriayousefi::mcp::server    // Server implementation
pooriayousefi::core           // General utilities
```

### Coding Conventions

- **Classes/Structs**: `PascalCase` (e.g., `McpServer`)
- **Functions/Variables**: `snake_case` (e.g., `add_tool()`)
- **Constants**: `UPPER_SNAKE_CASE` (e.g., `MAX_BUFFER_SIZE`)
- **Namespaces**: lowercase (e.g., `pooriayousefi::mcp`)
- **Indentation**: Allman style (braces on new lines)

### Dependencies Strategy

- **Header-only libraries only**: No linking complexity
- **Minimal dependencies**: Only nlohmann/json, asio, cpp-httplib
- **Zero Boost**: Using standalone alternatives
- See [DEPENDENCIES.md](DEPENDENCIES.md) for details

## Contributing

We welcome contributions! This project aims to become the official C++ SDK for MCP.

### How to Contribute

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

See [CONTRIBUTING.md](CONTRIBUTING.md) for detailed guidelines (coming soon).

## Roadmap to Official SDK

1. **Phase 1** (Current): Core implementation and architecture
2. **Phase 2**: Community adoption and feedback
3. **Phase 3**: Proposal to [modelcontextprotocol](https://github.com/modelcontextprotocol) organization
4. **Phase 4**: Official SDK status with potential corporate partnership

## Related Projects

- [MCP Specification](https://spec.modelcontextprotocol.io/)
- [MCP Go SDK](https://github.com/modelcontextprotocol/go-sdk)
- [MCP Rust SDK](https://github.com/modelcontextprotocol/rust-sdk)
- [MCP Python SDK](https://github.com/modelcontextprotocol/python-sdk)
- [MCP TypeScript SDK](https://github.com/modelcontextprotocol/typescript-sdk)

## Community

- **Discussions**: [GitHub Discussions](https://github.com/pooriayousefi/cpp-sdk/discussions)
- **Issues**: [GitHub Issues](https://github.com/pooriayousefi/cpp-sdk/issues)
- **MCP Community**: [Official MCP Discussions](https://github.com/orgs/modelcontextprotocol/discussions)

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

Copyright (c) 2025 The C++ MCP SDK Authors

## Acknowledgements

- Created and maintained by [Pooria Yousefi](https://github.com/pooriayousefi)
- Inspired by the official [Go](https://github.com/modelcontextprotocol/go-sdk) and [Rust](https://github.com/modelcontextprotocol/rust-sdk) MCP SDKs
- Built with support from the MCP community

---

**Status**: Early Development | **Version**: 0.1.0-alpha | **Last Updated**: October 2025
