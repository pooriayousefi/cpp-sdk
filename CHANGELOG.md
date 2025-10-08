# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Initial project structure with C++23 support
- Build system (`builder.cpp`) supporting debug/release builds
- Core utility headers in `include/core/`:
  - `asyncops.hpp` - Async operations & coroutines
  - `raiiiofsw.hpp` - RAII filesystem wrappers
  - `stringformers.hpp` - String formatting utilities
  - `utilities.hpp` - General utility functions
- VSCode configuration and tasks
- Professional documentation:
  - `README.md` - Project overview and quick start
  - `LICENSE` - MIT License
  - `AUTHORS` - Contributors list
  - `DEPENDENCIES.md` - Dependencies strategy
  - `PROJECT_STATUS.md` - Current status and roadmap
  - `NEXT_STEPS.md` - Development guide
  - `CHANGELOG.md` - This file

### Changed
- Project renamed from `mcp++` to `cpp-sdk`
- All references updated from `mcp__` to `cpp-sdk`
- Adopted professional naming convention matching official MCP SDKs

### Planned
- MCP protocol types implementation
- Transport layer (stdio, HTTP/SSE)
- Client and server implementations
- Resource and tool management
- Comprehensive examples and tests

## [0.1.0-alpha] - 2025-10-08

### Foundation Release

This is the initial foundation release establishing the project structure,
documentation, and development strategy.

**Key Highlights:**
- Professional project setup aligned with official MCP SDK standards
- Header-only dependencies strategy (nlohmann/json, asio, cpp-httplib)
- Clear namespace design (`pooriayousefi::mcp::*`)
- MIT License with community-friendly copyright
- Comprehensive documentation and roadmap

**Status:** Foundation complete, ready for core implementation phase

---

For detailed development status, see [PROJECT_STATUS.md](PROJECT_STATUS.md)  
For next steps, see [NEXT_STEPS.md](NEXT_STEPS.md)
