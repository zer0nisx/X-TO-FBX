#!/bin/bash

# X2FBX Converter Build Script
# This script automates the build process for the X2FBX converter

set -e  # Exit on any error

# Configuration
BUILD_TYPE=${1:-Release}
BUILD_DIR="build"
INSTALL_PREFIX=""
VERBOSE=false
CLEAN=false
RUN_TESTS=false
ENABLE_FBX_SDK=true

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to display usage
show_usage() {
    echo "Usage: $0 [BUILD_TYPE] [OPTIONS]"
    echo ""
    echo "BUILD_TYPE:"
    echo "  Release    Build optimized release version (default)"
    echo "  Debug      Build debug version with symbols"
    echo "  RelWithDebInfo  Release with debug info"
    echo ""
    echo "OPTIONS:"
    echo "  --clean              Clean build directory before building"
    echo "  --verbose            Enable verbose build output"
    echo "  --test               Run tests after building"
    echo "  --install PREFIX     Install to specified prefix"
    echo "  --no-fbx-sdk         Disable FBX SDK support"
    echo "  --help               Show this help message"
    echo ""
    echo "Examples:"
    echo "  $0                   # Build release version"
    echo "  $0 Debug --test      # Build debug and run tests"
    echo "  $0 Release --clean   # Clean build and build release"
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        --clean)
            CLEAN=true
            shift
            ;;
        --verbose)
            VERBOSE=true
            shift
            ;;
        --test)
            RUN_TESTS=true
            shift
            ;;
        --install)
            INSTALL_PREFIX="$2"
            shift 2
            ;;
        --no-fbx-sdk)
            ENABLE_FBX_SDK=false
            shift
            ;;
        --help)
            show_usage
            exit 0
            ;;
        Debug|Release|RelWithDebInfo)
            BUILD_TYPE="$1"
            shift
            ;;
        *)
            print_error "Unknown option: $1"
            show_usage
            exit 1
            ;;
    esac
done

# Print build configuration
print_status "X2FBX Converter Build Script"
echo "================================"
echo "Build Type: $BUILD_TYPE"
echo "Build Directory: $BUILD_DIR"
echo "FBX SDK: $([ "$ENABLE_FBX_SDK" = true ] && echo "Enabled" || echo "Disabled")"
echo "Clean Build: $([ "$CLEAN" = true ] && echo "Yes" || echo "No")"
echo "Run Tests: $([ "$RUN_TESTS" = true ] && echo "Yes" || echo "No")"
if [ -n "$INSTALL_PREFIX" ]; then
    echo "Install Prefix: $INSTALL_PREFIX"
fi
echo "================================"

# Check for required tools
print_status "Checking build requirements..."

# Check for CMake
if ! command -v cmake &> /dev/null; then
    print_error "CMake is not installed or not in PATH"
    exit 1
fi

CMAKE_VERSION=$(cmake --version | head -n1 | cut -d' ' -f3)
print_success "CMake found: version $CMAKE_VERSION"

# Check for compiler
if command -v g++ &> /dev/null; then
    GCC_VERSION=$(g++ --version | head -n1)
    print_success "GCC found: $GCC_VERSION"
elif command -v clang++ &> /dev/null; then
    CLANG_VERSION=$(clang++ --version | head -n1)
    print_success "Clang found: $CLANG_VERSION"
else
    print_error "No C++ compiler found (g++ or clang++)"
    exit 1
fi

# Check for FBX SDK if enabled
if [ "$ENABLE_FBX_SDK" = true ]; then
    if [ -d "third_party/fbx_sdk" ]; then
        print_success "FBX SDK found in third_party/fbx_sdk"
    elif [ -n "$FBX_SDK_ROOT" ]; then
        print_success "FBX SDK found at: $FBX_SDK_ROOT"
    else
        print_warning "FBX SDK not found. Building without FBX SDK support."
        print_warning "Download FBX SDK and place in third_party/fbx_sdk/ for full functionality."
        ENABLE_FBX_SDK=false
    fi
fi

# Clean build directory if requested
if [ "$CLEAN" = true ]; then
    print_status "Cleaning build directory..."
    rm -rf "$BUILD_DIR"
    print_success "Build directory cleaned"
fi

# Create build directory
print_status "Creating build directory..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Prepare CMake arguments
CMAKE_ARGS="-DCMAKE_BUILD_TYPE=$BUILD_TYPE"

if [ -n "$INSTALL_PREFIX" ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX"
fi

if [ "$ENABLE_FBX_SDK" = false ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DFBX_SDK_ROOT="
fi

# Configure project
print_status "Configuring project..."
if [ "$VERBOSE" = true ]; then
    cmake .. $CMAKE_ARGS
else
    cmake .. $CMAKE_ARGS > /dev/null
fi
print_success "Project configured successfully"

# Build project
print_status "Building project..."
if [ "$VERBOSE" = true ]; then
    cmake --build . --config $BUILD_TYPE
else
    cmake --build . --config $BUILD_TYPE > /dev/null
fi
print_success "Project built successfully"

# Check if executable was created
EXECUTABLE_NAME="x2fbx-converter"
if [ -f "bin/$EXECUTABLE_NAME" ]; then
    EXECUTABLE_PATH="bin/$EXECUTABLE_NAME"
elif [ -f "$EXECUTABLE_NAME" ]; then
    EXECUTABLE_PATH="$EXECUTABLE_NAME"
else
    print_error "Executable not found after build"
    exit 1
fi

print_success "Executable created: $EXECUTABLE_PATH"

# Display executable info
EXECUTABLE_SIZE=$(du -h "$EXECUTABLE_PATH" | cut -f1)
print_status "Executable size: $EXECUTABLE_SIZE"

# Run tests if requested
if [ "$RUN_TESTS" = true ]; then
    print_status "Running tests..."

    if [ -f "bin/x2fbx-tests" ]; then
        print_status "Running unit tests..."
        ./bin/x2fbx-tests
        print_success "Unit tests completed"
    else
        print_warning "Test executable not found, skipping tests"
    fi

    # Run CTest if available
    if command -v ctest &> /dev/null; then
        print_status "Running CTest..."
        if [ "$VERBOSE" = true ]; then
            ctest --verbose
        else
            ctest
        fi
        print_success "CTest completed"
    fi
fi

# Install if prefix specified
if [ -n "$INSTALL_PREFIX" ]; then
    print_status "Installing to $INSTALL_PREFIX..."
    cmake --install . --config $BUILD_TYPE
    print_success "Installation completed"
fi

# Final success message
cd ..
print_success "Build completed successfully!"
echo ""
echo "Executable location: $BUILD_DIR/$EXECUTABLE_PATH"
echo ""
echo "Usage examples:"
echo "  ./$BUILD_DIR/$EXECUTABLE_PATH --help"
echo "  ./$BUILD_DIR/$EXECUTABLE_PATH example.x"
echo "  ./$BUILD_DIR/$EXECUTABLE_PATH --verbose --output ./output example.x"
echo ""

# Show next steps
print_status "Next steps:"
echo "1. Test the converter with a sample .x file"
echo "2. Check the generated log file for detailed information"
echo "3. Review the output directory for converted FBX files"

if [ "$ENABLE_FBX_SDK" = false ]; then
    echo ""
    print_warning "Note: FBX SDK was not found. The converter will create placeholder"
    print_warning "FBX files. For full functionality, install FBX SDK and rebuild."
fi
