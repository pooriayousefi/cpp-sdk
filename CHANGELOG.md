# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned (Phase 3)
- Unit tests suite
- Performance benchmarks
- WebSocket transport
- Documentation website
- Community examples

## [0.2.0-alpha] - 2025-10-08

### Phase 2: Production-Ready Enhancements 🚀

This release adds production-ready async operations, streaming capabilities,
and comprehensive examples demonstrating advanced C++23 features.

#### Added

**Core Utilities Integration:**
- `ALWAYS_REMEMBER_HOW_TO_USE_CORE_HEADERS.md` - 927-line comprehensive guide
- Documentation for Generator<T>, Task<T>, RAII wrappers, string utilities
- Integration patterns and best practices for all core headers

**Async Client API:**
- `include/mcp/client_async.hpp` - AsyncClient class (267 lines)
- Task<T>-based async methods for non-blocking operations
- Parallel tool execution with `execute_parallel_async()`
- `sync_wait_client()` helper for bridging async/sync code
- Example: `examples/async_client_example.cpp` (7KB)

**Streaming Server API:**
- `include/mcp/server_streaming.hpp` - StreamingServer class (294 lines)
- Generator<T>-based streaming tools for memory-efficient processing
- Streaming resource support with incremental delivery
- Progress reporting and cancellation helpers
- Helper functions: `stream_file_lines()`, `stream_json_array()`, `stream_paginated_api()`,
  `transform_generator()`, `filter_generator()`
- Example: `examples/streaming_server_example.cpp` (11KB)

**File Resource Server:**
- `include/mcp/helpers/file_resource_server.hpp` - FileResourceServer class (401 lines)
- RAII file wrappers (RAIIInputFileStream) for automatic cleanup
- Automatic MIME type detection for 15+ file formats
- Path traversal security protection
- Streaming large files in 64KB chunks with progress reporting
- StreamingFileResourceServer variant for true line-by-line streaming
- Example: `examples/file_resource_server_example.cpp` (10KB)

**HTTP/SSE Transport:**
- `include/mcp/transport/http_transport.hpp` - HTTP transport implementation (~550 lines)
- HttpClientTransport: Task<T>-based async HTTP requests
- SseClientTransport: Generator<T>-based Server-Sent Events streaming
- HttpServerTransport: HTTP server with SSE notification broadcasting
- Configurable timeouts, custom headers, and connection pooling
- Thread-safe request/response queuing
- SSE keep-alive pings every 30 seconds
- Example: `examples/http_server_example.cpp` (10KB)

**Examples & Documentation:**
- Comprehensive `examples/README.md` with feature matrix and learning path
- Four complete Phase 2 examples (38KB total):
  - Async client with parallel execution
  - Streaming server with real-time data
  - File resource server with RAII safety
  - HTTP/SSE server for web deployment
- Each example includes compile commands, usage patterns, and testing instructions
- Claude Desktop integration examples for all stdio-based servers

#### Changed

**Dependencies:**
- Updated `DEPENDENCIES.md` to clarify optional dependencies
- nlohmann/json: Now bundled (no external installation required)
- cpp-httplib: Marked as optional (only for HTTP transport)
- Standalone Asio: Marked as optional (future use)
- **Zero required external dependencies** for core SDK

**Project Metrics:**
- Total files: 26 → 35 (+9 files)
- Header files: 8 → 13 (+5 headers)
- Example files: 1 → 5 (+4 examples)
- Total lines: ~15K → ~35K (+20K lines)
- Required dependencies: 3 → 0 (-3, now zero!)

#### Fixed
- None (no bugs from Phase 1)

#### Security
- Path traversal protection in FileResourceServer
- File size limits with configurable thresholds
- MIME type validation
- Thread-safe HTTP server request handling

### Key Features (Phase 2)

✅ **Async Operations** - Task<T> coroutines for non-blocking I/O  
✅ **Streaming** - Generator<T> for memory-efficient large datasets  
✅ **RAII Safety** - Automatic resource cleanup with file wrappers  
✅ **HTTP Transport** - Production-ready HTTP/JSON-RPC server  
✅ **SSE Notifications** - Real-time Server-Sent Events  
✅ **Progress Reporting** - Automatic progress updates during operations  
✅ **Cancellation** - Support for cancelling long-running operations  
✅ **MIME Detection** - Automatic content type detection  
✅ **Parallel Execution** - Concurrent tool calls  
✅ **Zero Dependencies** - Completely self-contained SDK

### Migration Notes

No breaking changes from 0.1.0. Phase 2 additions are fully backward-compatible.
Existing code using Phase 1 APIs (Client, Server) continues to work unchanged.

New async/streaming APIs are opt-in enhancements:
- Use `AsyncClient` instead of `Client` for Task<T> async operations
- Use `StreamingServer` instead of `Server` for Generator<T> streaming
- Use `FileResourceServer` helper for automatic file serving

## [0.1.0-alpha] - 2025-10-08

### Phase 1: Foundation Release

This is the initial foundation release establishing the project structure,
documentation, and development strategy.

#### Added

**Project Structure:**
- Initial project structure with C++23 support
- Build system (`builder.cpp`) supporting debug/release builds
- Core utility headers in `include/core/`:
  - `asyncops.hpp` - Async operations & coroutines
  - `raiiiofsw.hpp` - RAII filesystem wrappers
  - `stringformers.hpp` - String formatting utilities
  - `utilities.hpp` - General utility functions
- VSCode configuration and tasks

**MCP Protocol Implementation:**
- `include/mcp/protocol.hpp` - MCP protocol types (415 lines)
- `include/mcp/client.hpp` - MCP client implementation
- `include/mcp/server.hpp` - MCP server implementation
- `include/mcp/transport/transport.hpp` - Transport abstraction
- `include/mcp/jsonrpc/jsonrpc.hpp` - JSON-RPC 2.0 library (487 lines)
- StdioTransport and InMemoryTransport implementations
- MCP 2024-11-05 specification compliance

**Documentation:**
- `README.md` - Project overview and quick start
- `LICENSE` - MIT License
- `AUTHORS` - Contributors list
- `DEPENDENCIES.md` - Dependencies strategy
- `PROJECT_STATUS.md` - Current status and roadmap
- `NEXT_STEPS.md` - Development guide
- `CHANGELOG.md` - This file

**Examples:**
- `examples/simple_server.cpp` - Calculator tool server
- `examples/README.md` - Examples documentation

#### Changed
- Project renamed from `mcp++` to `cpp-sdk`
- All references updated from `mcp__` to `cpp-sdk`
- Adopted professional naming convention matching official MCP SDKs

**Key Highlights:**
- Professional project setup aligned with official MCP SDK standards
- Header-only dependencies strategy (nlohmann/json bundled)
- Clear namespace design (`pooriayousefi::mcp::*`)
- MIT License with community-friendly copyright
- Comprehensive documentation and roadmap
- Full MCP protocol support (tools, prompts, resources)

**Status:** Foundation complete, ready for production enhancements

---

For detailed development status, see [PROJECT_STATUS.md](PROJECT_STATUS.md)
