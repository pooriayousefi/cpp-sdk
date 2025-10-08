# Dependencies Strategy

This SDK follows a **minimal, header-only dependencies** approach to maximize portability and ease of integration.

## Core Philosophy
- **Header-only libraries only**: No linking, no ABI issues
- **Minimal external dependencies**: Reduce integration friction
- **C++23 standard**: Leverage modern C++ features (coroutines, ranges, modules)
- **Zero Boost**: Avoid heavy dependencies

## Required Header-Only Libraries

### 1. JSON Processing: nlohmann/json
- **Repository**: https://github.com/nlohmann/json
- **Version**: >= 3.11.3
- **Stars**: 42k+
- **Why**: Industry standard, header-only, excellent performance
- **License**: MIT
- **Integration**: Single header include

```cpp
#include <nlohmann/json.hpp>
using json = nlohmann::json;
```

### 2. Async I/O: Standalone Asio
- **Repository**: https://github.com/chriskohlhoff/asio
- **Version**: >= 1.30.2
- **Why**: Header-only mode available, no Boost dependency
- **License**: Boost Software License
- **Integration**: Header-only with ASIO_STANDALONE

```cpp
#define ASIO_STANDALONE
#include <asio.hpp>
```

### 3. HTTP/WebSocket: cpp-httplib
- **Repository**: https://github.com/yhirose/cpp-httplib
- **Version**: >= 0.18.0
- **Stars**: 12k+
- **Why**: Single header, supports HTTP/HTTPS/WebSocket, header-only
- **License**: MIT
- **Integration**: Single header include

```cpp
#include <httplib.h>
```

## Optional Dependencies

### Testing: Catch2 (v3)
- **Repository**: https://github.com/catchorg/Catch2
- **Version**: >= 3.7.1
- **Why**: Header-only test framework
- **License**: Boost Software License

## Installation Methods

### Method 1: Git Submodules (Recommended)
```bash
git submodule add https://github.com/nlohmann/json.git external/json
git submodule add https://github.com/chriskohlhoff/asio.git external/asio
git submodule add https://github.com/yhirose/cpp-httplib.git external/httplib
```

### Method 2: CMake FetchContent
```cmake
include(FetchContent)

FetchContent_Declare(json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3)

FetchContent_Declare(asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG asio-1-30-2)

FetchContent_Declare(httplib
  GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
  GIT_TAG v0.18.0)

FetchContent_MakeAvailable(json asio httplib)
```

### Method 3: vcpkg
```bash
vcpkg install nlohmann-json asio cpp-httplib
```

### Method 4: Manual Download
Download single headers and place in `include/external/`:
- nlohmann/json: `json.hpp`
- asio: `asio.hpp` + `asio/` directory
- cpp-httplib: `httplib.h`

## Compiler Requirements

- **GCC**: >= 13.0 (C++23 support)
- **Clang**: >= 16.0 (C++23 support)
- **MSVC**: >= 19.34 (Visual Studio 2022 17.4+)
- **C++ Standard**: C++23

## Build Flags

```bash
# Linux/macOS
-std=c++23 -DASIO_STANDALONE -pthread

# Windows (MSVC)
/std:c++latest /DASIO_STANDALONE
```

## Why No Boost?

While Boost is excellent, we avoid it because:
- ✅ **Size**: Boost is massive (~200MB), our approach is ~5MB
- ✅ **Compile time**: Faster compilation without Boost
- ✅ **Integration**: Easier for users to integrate
- ✅ **Portability**: Standalone Asio works everywhere
- ✅ **Modern C++**: C++23 provides many features Boost pioneered

## License Compatibility

All dependencies are MIT or Boost License compatible:
- ✅ Commercial use allowed
- ✅ No copyleft restrictions
- ✅ Attribution required (handled in LICENSES.md)

## Bundled Components

### JSON-RPC 2.0 Library
- **Source**: Custom implementation (adapted from jsonrpc20 by Pooria Yousefi)
- **Location**: `include/mcp/jsonrpc/jsonrpc.hpp`
- **Why Bundled**: 
  - Core protocol requirement for MCP
  - Header-only, lightweight (~500 lines)
  - MCP-specific features (progress, cancellation, initialize)
  - Zero external dependencies beyond nlohmann/json
- **License**: MIT
- **Integration**: Automatically included with `#include <mcp/client.hpp>` or `#include <mcp/server.hpp>`

```cpp
// No separate installation needed - bundled with SDK
#include <mcp/jsonrpc/jsonrpc.hpp>
using namespace pooriayousefi::mcp::jsonrpc;
```

**Features:**
- ✅ JSON-RPC 2.0 specification compliant
- ✅ Request/response/notification handling
- ✅ Batch request support
- ✅ Method dispatcher with error handling
- ✅ Progress reporting (`$/progress`)
- ✅ Request cancellation (`$/cancelRequest`)
- ✅ Thread-local context for handlers
- ✅ MCP initialize protocol support

The JSON-RPC library is bundled because:
1. **Protocol Foundation**: MCP is built on JSON-RPC 2.0
2. **MCP Extensions**: Includes MCP-specific notifications and context
3. **Header-Only**: Fits our zero-dependency philosophy
4. **Maintenance**: Direct control over protocol implementation
5. **Size**: Small enough to bundle (~500 lines + nlohmann/json which we already require)

### nlohmann/json (Bundled)
- **Location**: `include/mcp/jsonrpc/json.hpp`
- **Version**: 3.11.3
- **Why Bundled**: Single ~1MB header, eliminates external dependency management
- **License**: MIT

The complete MCP SDK requires **zero external dependencies** to install - everything needed is bundled.
