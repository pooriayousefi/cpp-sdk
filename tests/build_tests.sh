#!/bin/bash
# Build script for C++ MCP SDK tests
# Usage: ./build_tests.sh [--clean] [--run] [--verbose]

set -e  # Exit on error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Configuration
CXX=${CXX:-clang++-20}
CXXFLAGS="-std=c++23 -Wall -Wextra -I../include -I../include/external"
BUILD_DIR="build"
TEST_BINARY="$BUILD_DIR/test_runner"

# Parse arguments
CLEAN=false
RUN=false
VERBOSE=false

for arg in "$@"; do
    case $arg in
        --clean)
            CLEAN=true
            ;;
        --run)
            RUN=true
            ;;
        --verbose)
            VERBOSE=true
            CXXFLAGS="$CXXFLAGS -DCATCH_CONFIG_CONSOLE_WIDTH=120"
            ;;
        *)
            echo -e "${YELLOW}Unknown option: $arg${NC}"
            ;;
    esac
done

# Clean if requested
if [ "$CLEAN" = true ]; then
    echo -e "${YELLOW}Cleaning build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"

echo -e "${GREEN}Building C++ MCP SDK Tests${NC}"
echo "Compiler: $CXX"
echo "Flags: $CXXFLAGS"
echo ""

# Compile Catch2 implementation (only once)
echo -e "${YELLOW}Compiling Catch2 framework...${NC}"
$CXX $CXXFLAGS -c ../include/external/catch_amalgamated.cpp -o "$BUILD_DIR/catch_amalgamated.o"

# Compile test runner main
echo -e "${YELLOW}Compiling test runner...${NC}"
$CXX $CXXFLAGS -c main.cpp -o "$BUILD_DIR/main.o"

# Compile test files
echo -e "${YELLOW}Compiling test suites...${NC}"
TEST_FILES=(
    "test_protocol.cpp"
    "test_jsonrpc.cpp"
    "test_transport.cpp"
    "test_server.cpp"
    "test_client.cpp"
    "test_client_async.cpp"
    # Add more test files here as they are created
)

TEST_OBJECTS=()
for test_file in "${TEST_FILES[@]}"; do
    if [ -f "$test_file" ]; then
        echo "  - $test_file"
        obj_file="$BUILD_DIR/$(basename ${test_file%.cpp}.o)"
        $CXX $CXXFLAGS -c "$test_file" -o "$obj_file"
        TEST_OBJECTS+=("$obj_file")
    fi
done

# Link all objects
echo -e "${YELLOW}Linking test executable...${NC}"
$CXX $CXXFLAGS \
    "$BUILD_DIR/main.o" \
    "$BUILD_DIR/catch_amalgamated.o" \
    "${TEST_OBJECTS[@]}" \
    -o "$TEST_BINARY"

echo -e "${GREEN}✓ Build successful!${NC}"
echo "Test binary: $TEST_BINARY"
echo ""

# Run tests if requested
if [ "$RUN" = true ]; then
    echo -e "${GREEN}Running tests...${NC}"
    echo "========================================"
    
    if [ "$VERBOSE" = true ]; then
        "$TEST_BINARY" --success
    else
        "$TEST_BINARY"
    fi
    
    TEST_RESULT=$?
    echo "========================================"
    
    if [ $TEST_RESULT -eq 0 ]; then
        echo -e "${GREEN}✓ All tests passed!${NC}"
    else
        echo -e "${RED}✗ Some tests failed!${NC}"
        exit $TEST_RESULT
    fi
fi

echo ""
echo "To run tests manually:"
echo "  cd tests"
echo "  ./$TEST_BINARY              # Run all tests"
echo "  ./$TEST_BINARY --list-tests # List available tests"
echo "  ./$TEST_BINARY \"[protocol]\" # Run specific tag"
echo "  ./$TEST_BINARY --help       # Show all options"
