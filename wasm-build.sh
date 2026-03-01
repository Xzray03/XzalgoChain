#!/bin/bash

# XzalgoChain - WebAssembly Build Script
# This script compiles the XzalgoChain library to WebAssembly using Emscripten

set -e

# Configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="${SCRIPT_DIR}"
WASM_OUTPUT_DIR="${PROJECT_ROOT}/wasm"
WASM_BUILD_DIR="${PROJECT_ROOT}/wasm_build"
SOURCE_WASM_WRAPPER="${PROJECT_ROOT}/xzalgochain_wasm.c"
XZALGOCHAIN_MAIN_SOURCE="${PROJECT_ROOT}/xzalgochain.c"
XZALGOCHAIN_INCLUDE_DIR="${PROJECT_ROOT}/XzalgoChain"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Print banner
echo -e "${GREEN}"
echo "╔═══════════════════════════════════════════════════════════════╗"
echo "║           XzalgoChain - WebAssembly Build Script              ║"
echo "║                     Copyright 2026 Xzrayツ                    ║"
echo "╚═══════════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# Check if emcc is available
if ! command -v emcc &> /dev/null; then
    echo -e "${RED}Error: emcc (Emscripten compiler) not found in PATH${NC}"
    echo "Please install Emscripten SDK (emsdk) and activate it first:"
    echo "  git clone https://github.com/emscripten-core/emsdk.git"
    echo "  cd emsdk"
    echo "  ./emsdk install latest"
    echo "  ./emsdk activate latest"
    echo "  source ./emsdk_env.sh"
    exit 1
fi

# Check if source files exist
if [ ! -f "${XZALGOCHAIN_MAIN_SOURCE}" ]; then
    echo -e "${RED}Error: Main source file not found: ${XZALGOCHAIN_MAIN_SOURCE}${NC}"
    exit 1
fi

if [ ! -f "${SOURCE_WASM_WRAPPER}" ]; then
    echo -e "${RED}Error: WASM wrapper source file not found: ${SOURCE_WASM_WRAPPER}${NC}"
    exit 1
fi

if [ ! -d "${XZALGOCHAIN_INCLUDE_DIR}" ]; then
    echo -e "${RED}Error: Include directory not found: ${XZALGOCHAIN_INCLUDE_DIR}${NC}"
    exit 1
fi

# Display Emscripten version
echo -e "${YELLOW}Using Emscripten:${NC}"
emcc --version | head -n 1
echo ""

# Create output directories
echo -e "${YELLOW}Creating output directories...${NC}"
mkdir -p "$WASM_OUTPUT_DIR"
mkdir -p "$WASM_BUILD_DIR"
echo -e "${GREEN}✓ Directories created${NC}\n"

# Clean previous builds
echo -e "${YELLOW}Cleaning previous WASM builds...${NC}"
rm -f "${WASM_OUTPUT_DIR}/XzalgoChain.wasm"
rm -f "${WASM_OUTPUT_DIR}/XzalgoChain.js"
rm -f "${WASM_BUILD_DIR}"/*.o
echo -e "${GREEN}✓ Cleaned${NC}\n"

# Set compilation flags
echo -e "${YELLOW}Configuring compilation flags...${NC}"

# Base flags
BASE_CFLAGS=(
    "-O3"
    "-flto"
    "-DNDEBUG"
    "-DXZALGOCHAIN_STATIC=1"
    "-DXZALGOCHAIN_ENABLE_SIMD=0"
    "-DXZALGOCHAIN_USE_OPENMP=0"
    "-I${XZALGOCHAIN_INCLUDE_DIR}"
)

# Platform definitions
BASE_CFLAGS+=(
    "-D__WASM__=1"
    "-D__EMSCRIPTEN__=1"
)

# Base flags for linking
BASE_LDFLAGS=(
    "-O3"
    "-flto"
    "--closure 1"
    "-s WASM=1"
    "-s WASM_ASYNC_COMPILATION=1"
    "-s MODULARIZE=1"
    "-s EXPORT_NAME='XzalgoChain'"
    "-s EXPORTED_FUNCTIONS=['_malloc','_free','_xzalgochain_wasm','_xzalgochain_init_wasm','_xzalgochain_update_wasm','_xzalgochain_final_wasm','_xzalgochain_ctx_reset_wasm','_xzalgochain_ctx_wipe_wasm','_xzalgochain_copy_wasm','_xzalgochain_equals_wasm','_xzalgochain_version_wasm']"
    "-s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','getValue','setValue','UTF8ToString','stringToUTF8','lengthBytesUTF8']"
    "-s ALLOW_MEMORY_GROWTH=1"
    "-s MAXIMUM_MEMORY=512MB"
    "-s FILESYSTEM=0"
    "-s ENVIRONMENT='web'"
    "-s STACK_SIZE=5MB"
    "-s INITIAL_MEMORY=16777216"
    "-s SINGLE_FILE=0"
    "-s BINARYEN_ASYNC_COMPILATION=1"
)

# Add optimization flags
if [[ "$1" == "--size" ]] || [[ "$1" == "-s" ]]; then
    echo -e "${YELLOW}Size optimization mode enabled${NC}"
    BASE_CFLAGS=("${BASE_CFLAGS[@]/#-O3/-Os}")
    BASE_LDFLAGS=("${BASE_LDFLAGS[@]/#-O3/-Os}")
fi

# Add debug flags if requested
if [[ "$1" == "--debug" ]] || [[ "$1" == "-d" ]] || [[ "$2" == "--debug" ]] || [[ "$2" == "-d" ]]; then
    echo -e "${YELLOW}Debug mode enabled${NC}"
    BASE_CFLAGS+=("-g2" "-DDEBUG")
    BASE_LDFLAGS+=("-g2" "-s DEMANGLE_SUPPORT=1" "-s ASSERTIONS=2")
else
    BASE_CFLAGS+=("-g0")
    BASE_LDFLAGS+=("-g0")
fi

echo -e "${GREEN}✓ Configuration complete${NC}\n"

# ==================== COMPILE XZALGOCHAIN.C TO LLVM BITCODE ====================
echo -e "${YELLOW}Step 1: Compiling xzalgochain.c to LLVM bitcode...${NC}"

emcc "${XZALGOCHAIN_MAIN_SOURCE}" \
    ${BASE_CFLAGS[@]} \
    -c \
    -o "${WASM_BUILD_DIR}/xzalgochain.bc"

if [ $? -ne 0 ]; then
    echo -e "${RED}✗ Failed to compile xzalgochain.c${NC}"
    exit 1
fi
echo -e "${GREEN}✓ xzalgochain.c compiled${NC}\n"

# ==================== COMPILE WASM WRAPPER ====================
echo -e "${YELLOW}Step 2: Compiling WASM wrapper...${NC}"

emcc "${SOURCE_WASM_WRAPPER}" \
    ${BASE_CFLAGS[@]} \
    -c \
    -o "${WASM_BUILD_DIR}/xzalgochain_wasm.bc"

if [ $? -ne 0 ]; then
    echo -e "${RED}✗ Failed to compile WASM wrapper${NC}"
    exit 1
fi
echo -e "${GREEN}✓ WASM wrapper compiled${NC}\n"

# ==================== LINK TO FINAL WASM ====================
echo -e "${YELLOW}Step 3: Linking to final WebAssembly module...${NC}"

emcc \
    "${WASM_BUILD_DIR}/xzalgochain.bc" \
    "${WASM_BUILD_DIR}/xzalgochain_wasm.bc" \
    ${BASE_LDFLAGS[@]} \
    -o "${WASM_OUTPUT_DIR}/XzalgoChain.js"

if [ $? -ne 0 ]; then
    echo -e "${RED}✗ Failed to link WASM module${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Linking complete${NC}\n"

# ==================== VERIFY OUTPUT ====================
echo -e "${YELLOW}Step 4: Verifying output...${NC}"

if [ -f "${WASM_OUTPUT_DIR}/XzalgoChain.wasm" ] && [ -f "${WASM_OUTPUT_DIR}/XzalgoChain.js" ]; then
    echo -e "${GREEN}✓ Build successful!${NC}"

    # Display output files
    echo -e "\n${YELLOW}Generated files:${NC}"
    ls -lh "${WASM_OUTPUT_DIR}" | grep -E "XzalgoChain\.(wasm|js)"

    # Show file sizes
    echo -e "\n${YELLOW}File sizes:${NC}"
    if [ -f "${WASM_OUTPUT_DIR}/XzalgoChain.wasm" ]; then
        WASM_SIZE=$(du -h "${WASM_OUTPUT_DIR}/XzalgoChain.wasm" | cut -f1)
        echo "  WASM: ${WASM_SIZE}"
    fi
    if [ -f "${WASM_OUTPUT_DIR}/XzalgoChain.js" ]; then
        JS_SIZE=$(du -h "${WASM_OUTPUT_DIR}/XzalgoChain.js" | cut -f1)
        echo "  JS:   ${JS_SIZE}"
    fi

# Create README for the WASM output
cat > "${WASM_OUTPUT_DIR}/README.md" << EOF
# XzalgoChain WebAssembly Module

This directory contains the WebAssembly build of XzalgoChain cryptographic hash function.

## Files

- \`XzalgoChain.wasm\` - WebAssembly binary module
- \`XzalgoChain.js\` - JavaScript glue code for loading the module

## Usage

\`\`\`javascript
// Load the module
import XzalgoChain from './XzalgoChain.js';

// Initialize
const xzalgochain = await XzalgoChain();

// Hash a string
function stringToUTF8Array(str) {
    const utf8 = unescape(encodeURIComponent(str));
    const arr = new Uint8Array(utf8.length);
    for (let i = 0; i < utf8.length; i++) {
        arr[i] = utf8.charCodeAt(i);
    }
    return arr;
}

async function calculateHash(input) {
    // Convert string to bytes
    const data = stringToUTF8Array(input);
    const output = new Uint8Array(40); // 320 bits = 40 bytes

    // Allocate memory
    const dataPtr = xzalgochain._malloc(data.length);
    const outputPtr = xzalgochain._malloc(40);

    try {
        // Write data to WASM memory using setValue
        for (let i = 0; i < data.length; i++) {
            xzalgochain.setValue(dataPtr + i, data[i], 'i8');
        }

        // Call hash function
        xzalgochain._xzalgochain_wasm(dataPtr, data.length, outputPtr);

        // Read result back using getValue
        for (let i = 0; i < 40; i++) {
            output[i] = xzalgochain.getValue(outputPtr + i, 'i8');
        }

        // Convert to hex string (handle negative values)
        return Array.from(output)
            .map(b => (b + 256) % 256)
            .map(b => b.toString(16).padStart(2, '0'))
            .join('');
    } finally {
        // Free memory
        xzalgochain._free(dataPtr);
        xzalgochain._free(outputPtr);
    }
}

// Example usage
const hash = await calculateHash("Hello, XzalgoChain!");
console.log("Hash:", hash);
\`\`\`

## API Reference

### Memory Management Functions
- \`_malloc(size)\` - Allocate memory in WASM heap
- \`_free(ptr)\` - Free allocated memory
- \`setValue(ptr, value, type)\` - Write value to memory (type: 'i8', 'i16', 'i32', etc.)
- \`getValue(ptr, type)\` - Read value from memory

### Core Hash Functions
- \`_xzalgochain_wasm(dataPtr, dataLength, outputPtr)\` - One-shot hash calculation
  - \`dataPtr\`: Pointer to input data
  - \`dataLength\`: Length of input data in bytes
  - \`outputPtr\`: Pointer to output buffer (40 bytes)

### Context Management Functions
- \`_xzalgochain_init_wasm(ctxPtr)\` - Initialize hash context
- \`_xzalgochain_update_wasm(ctxPtr, dataPtr, dataLength)\` - Update hash with data
- \`_xzalgochain_final_wasm(ctxPtr, outputPtr)\` - Finalize hash
- \`_xzalgochain_ctx_reset_wasm(ctxPtr)\` - Reset context
- \`_xzalgochain_ctx_wipe_wasm(ctxPtr)\` - Securely wipe context

### Utility Functions
- \`_xzalgochain_copy_wasm(dstPtr, srcPtr)\` - Copy hash value
- \`_xzalgochain_equals_wasm(h1Ptr, h2Ptr)\` - Constant-time hash comparison (returns 1 if equal, 0 if not)
- \`_xzalgochain_version_wasm()\` - Get library version (returns pointer to string)

### String Helper Functions
- \`UTF8ToString(ptr)\` - Convert WASM string to JavaScript string
- \`stringToUTF8(str, ptr, maxBytes)\` - Convert JavaScript string to WASM memory
- \`lengthBytesUTF8(str)\` - Get byte length of UTF8 string

## Example: Context API Usage

\`\`\`javascript
// Hash large data in chunks
async function hashLargeData(data) {
    const ctxPtr = xzalgochain._malloc(256); // Allocate context (size depends on implementation)
    const output = new Uint8Array(40);
    const outputPtr = xzalgochain._malloc(40);

    try {
        // Initialize context
        xzalgochain._xzalgochain_init_wasm(ctxPtr);

        // Process data in chunks
        const chunkSize = 64 * 1024; // 64KB chunks
        for (let i = 0; i < data.length; i += chunkSize) {
            const chunk = data.slice(i, i + chunkSize);
            const chunkPtr = xzalgochain._malloc(chunk.length);

            // Copy chunk to WASM memory
            for (let j = 0; j < chunk.length; j++) {
                xzalgochain.setValue(chunkPtr + j, chunk[j], 'i8');
            }

            // Update hash with chunk
            xzalgochain._xzalgochain_update_wasm(ctxPtr, chunkPtr, chunk.length);

            xzalgochain._free(chunkPtr);
        }

        // Finalize hash
        xzalgochain._xzalgochain_final_wasm(ctxPtr, outputPtr);

        // Read result
        for (let i = 0; i < 40; i++) {
            output[i] = xzalgochain.getValue(outputPtr + i, 'i8');
        }

        return Array.from(output)
            .map(b => (b + 256) % 256)
            .map(b => b.toString(16).padStart(2, '0'))
            .join('');
    } finally {
        xzalgochain._free(ctxPtr);
        xzalgochain._free(outputPtr);
    }
}
\`\`\`

## Example: Compare Two Hashes

\`\`\`javascript
function compareHashes(hash1, hash2) {
    // Convert hex strings to bytes
    const bytes1 = new Uint8Array(40);
    const bytes2 = new Uint8Array(40);

    for (let i = 0; i < 40; i++) {
        bytes1[i] = parseInt(hash1.substr(i * 2, 2), 16);
        bytes2[i] = parseInt(hash2.substr(i * 2, 2), 16);
    }

    // Allocate memory
    const ptr1 = xzalgochain._malloc(40);
    const ptr2 = xzalgochain._malloc(40);

    try {
        // Copy bytes to WASM memory
        for (let i = 0; i < 40; i++) {
            xzalgochain.setValue(ptr1 + i, bytes1[i], 'i8');
            xzalgochain.setValue(ptr2 + i, bytes2[i], 'i8');
        }

        // Compare (returns 1 if equal, 0 if not)
        return xzalgochain._xzalgochain_equals_wasm(ptr1, ptr2) === 1;
    } finally {
        xzalgochain._free(ptr1);
        xzalgochain._free(ptr2);
    }
}
\`\`\`

## Build Configuration

- **Build Date**: $(date)
- **Emscripten Version**: $(emcc --version | head -n 1)
- **Optimization**: $([[ "$1" == "--size" ]] || [[ "$1" == "-s" ]] && echo "Size (-Os)" || echo "Speed (-O3)")

## Notes

- All memory allocated with \`_malloc\` must be freed with \`_free\` to prevent memory leaks
- Output buffer for hash must be exactly 40 bytes (320 bits)
- Hash comparison function (\`_xzalgochain_equals_wasm\`) is constant-time to prevent timing attacks
- Use \`setValue\` and \`getValue\` with type 'i8' for byte-level access
- Byte values from WASM may be negative; use \`(b + 256) % 256\` to convert to unsigned

## License

Apache 2.0 - See LICENSE file for details
EOF

    echo -e "\n${GREEN}Build complete! Files are in ${WASM_OUTPUT_DIR}${NC}"
    echo -e "${YELLOW}README.md created with usage instructions${NC}"
else
    echo -e "\n${RED}✗ Build failed: Output files not found${NC}"
    exit 1
fi

# Clean up build directory
echo -e "\n${YELLOW}Cleaning up build directory...${NC}"
rm -rf "${WASM_BUILD_DIR}"
echo -e "${GREEN}✓ Cleanup complete${NC}"
