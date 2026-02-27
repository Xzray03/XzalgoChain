<h1 align="left">
  <img src="logo.svg" alt="Logo" width="75" height="75" style="vertical-align: middle;">
  XzalgoChain - 320-bit Cryptographic Hash Function
</h1>

![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)
![C](https://img.shields.io/badge/C-C17-blue.svg)

XzalgoChain is a high-performance cryptographic hash function that produces 320-bit (40-byte) hash values. It features SIMD-accelerated implementations for both x86 (AVX2) and ARM (NEON) architectures, with automatic runtime detection and fallback to scalar code.


THIS IS STILL AN EARLY VERSION, NOT MEANT TO BE USED FOR ANY CRITICAL PURPOSE!!
USE AT YOUR OWN RISK!

## Features

- **320-bit output** (40 bytes) - Larger than SHA-256 for enhanced security
- **SIMD acceleration**:
  - AVX2 on x86/x86_64 (4-way parallel processing)
  - NEON on ARM (32-bit and 64-bit)
  - Automatic runtime detection
  - Scalar fallback when SIMD unavailable
- **OpenMP support** for parallel processing
- **Streaming API** for large data
- **Cross-platform** (Linux, macOS, Windows, Android, iOS)
- **Single-file implementations** - Easy to integrate

## Quick Start

### Building from Source

```bash
# Clone the repository
git clone https://github.com/Xzray03/XzalgoChain.git
cd XzalgoChain

# Build the utility and tests
make

cd tests
make
```

### Using the Command Line Utility

```bash
# Hash a file
./xzalgo320sum file.txt

# Hash a string
./xzalgo320sum -i "Hello, World!"

# Hash from stdin
echo -n "Hello" | ./xzalgo320sum

# Check mode
./xzalgo320sum -c HASH file.txt

# Verbose output with more info
./xzalgo320sum -V file.txt

# Force scalar mode (disable SIMD)
./xzalgo320sum -f file.txt
```

### Using in C/C++ Projects

```c
#include "XzalgoChain/XzalgoChain.h"

// Single-shot hashing
uint8_t hash[XZALGOCHAIN_HASH_SIZE];
xzalgochain((uint8_t*)"Hello, World!", 13, hash);

// Streaming API for large data
XzalgoChain_CTX ctx;
xzalgochain_init(&ctx);
xzalgochain_update(&ctx, data1, len1);
xzalgochain_update(&ctx, data2, len2);
xzalgochain_final(&ctx, hash);
```

## Performance

XzalgoChain achieves high performance through:
- **SIMD parallelism**: Process 4 blocks simultaneously
- **OpenMP**: Thread-level parallelism for multi-core systems
- **Optimized ARX operations**: Add-Rotate-XOR core design
- **Platform-specific optimizations**: AVX2 for x86, NEON for ARM

Performance metrics and detailed analysis are available in [TEST.md](TEST.md).

## Security

The algorithm has undergone extensive testing:
- **Avalanche effect**: >50% bit change probability
- **Strict Avalanche Criterion (SAC)**: Meets cryptographic requirements
- **Bit Independence Criterion (BIC)**: No linear correlations
- **Entropy analysis**: Near-ideal distribution
- **Differential cryptanalysis**: Resistant to differential attacks

See [ALGORITHM.md](ALGORITHM.md) for complete design documentation and [TEST.md](TEST.md) for test results.

## Project Structure

```
XzalgoChain
├── ALGORITHM.md                        # Complete cryptographic algorithm documentation
├── CONTRIBUTING.md                     # Developer contribution guidelines
├── INTEGRATION.md                      # Examples of integration into other languages
├── LICENSE                             # Project license (MIT/GPL/etc)
├── logo.svg                            # XzalgoChain project logo
├── CMakeLists.txt                      # Static and shared library build system
├── Makefile                            # Main build system
├── NOTICE                              # Legal notices and attributions
├── README.md                           # Main project documentation (This file)
├── TEST.md                             # Test results and documentation
│
├── tests/                              # Complete test suite
│   ├── Makefile                        # Tests-specific build system
│   ├── avalanche_test.c                # Tests avalanche effect (1-bit input changes)
│   ├── benchmark.c                     # Performance benchmarking
│   ├── bic_test.c                      # Bit Independence Criterion testing
│   ├── bit_bias_analyzer.c             # Analyzes bit distribution bias
│   ├── consistent_test.c               # Tests output consistency
│   ├── cross_correlation_test.c        # Cross-correlation analysis between bits
│   ├── differential_test.c             # Differential cryptanalysis tests
│   ├── dot_test.c                      # Generates DOT graphs for visualization
│   ├── entropy_test.c                  # Measures output entropy
│   ├── hash_counter.c                  # Hash counting and distribution
│   ├── linear_correlation_test.c       # Linear correlation analysis
│   ├── permutation_compression_test.c  # Permutation and compression tests
│   └── sac_test.c                      # Strict Avalanche Criterion testing
│
├── xzalgo320sum.c                      # Command-line hashing utility
├── xzalgochain.c                       # Main point for the library
│
└── XzalgoChain/                        # Core header-only library
    ├── algorithm.h                     # Core cryptographic primitives
    ├── algorithm_scalar.h              # Scalar (non-vectorized) implementation
    ├── algorithm_simd.h                # SIMD implementations (AVX2/NEON)
    ├── config.h                        # Configuration constants and macros
    ├── platform_detect.h               # Platform/architecture detection
    ├── simd_detect.h                   # Runtime SIMD capability detection
    ├── utils.h                         # Utility functions (endian, rotate, etc)
    └── XzalgoChain.h                   # Main public header (includes all)
```

## API Reference

### Core Functions

| Function | Description |
|----------|-------------|
| `xzalgochain()` | Single-shot hash computation |
| `xzalgochain_init()` | Initialize hashing context |
| `xzalgochain_update()` | Add data to context |
| `xzalgochain_final()` | Finalize and get hash |
| `xzalgochain_force_scalar()` | Force scalar mode |
| `xzalgochain_get_simd_type()` | Detect available SIMD |

### Utility Functions

| Function | Description |
|----------|-------------|
| `xzalgochain_copy()` | Copy hash value |
| `xzalgochain_equals()` | Compare two hashes |
| `xzalgochain_version()` | Get version string |
| `xzalgochain_platform_info()` | Get platform info |

## Platform Support

| Platform | SIMD Support | Status |
|----------|-------------|--------|
| Linux (x86_64) | AVX2 | ✓ (TESTED) |
| Linux (ARM64) | NEON | ✓ (TESTED) |
| macOS (Intel) | AVX2 | ✓ |
| macOS (Apple Silicon) | NEON | ✓ |
| Windows (x64) | AVX2 | ✓ |
| Windows (ARM64) | NEON | ✓ |
| Android (ARM) | NEON | ✓ (TESTED) |
| iOS | NEON | ✓ |
| 32-bit platforms | Scalar | ✓ |

## Building for Different Platforms

### Linux/macOS
```bash
make

cd tests
make

sudo make install  # Optional
```

### Windows (MSVC)
```bash
nmake -f Makefile.win
```

### Cross-compiling for ARM
```bash
# For ARM64
make CROSS_COMPILE=aarch64-linux-gnu

# For ARM32
make CROSS_COMPILE=arm-linux-gnueabihf
```
## Integration Examples

### Python (using ctypes)
```python
import ctypes
import os
import platform

# Load the appropriate library
if platform.system() == "Windows":
    lib_name = "XzalgoChain.dll"
elif platform.system() == "Darwin":
    lib_name = "libXzalgoChain.dylib"
else:
    lib_name = "libXzalgoChain.so"

lib = ctypes.CDLL(os.path.join(os.path.dirname(__file__), lib_name))

# Constants
XZALGOCHAIN_HASH_SIZE = 40

# Define function prototypes
lib.xzalgochain_lib.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t, ctypes.POINTER(ctypes.c_uint8)]
lib.xzalgochain_lib.restype = None

lib.xzalgochain_version_lib.argtypes = []
lib.xzalgochain_version_lib.restype = ctypes.c_char_p

lib.xzalgochain_platform_info_lib.argtypes = []
lib.xzalgochain_platform_info_lib.restype = ctypes.c_char_p

lib.xzalgochain_get_simd_name_lib.argtypes = []
lib.xzalgochain_get_simd_name_lib.restype = ctypes.c_char_p

lib.xzalgochain_get_simd_type_lib.argtypes = []
lib.xzalgochain_get_simd_type_lib.restype = ctypes.c_int

lib.xzalgochain_avx2_supported_lib.argtypes = []
lib.xzalgochain_avx2_supported_lib.restype = ctypes.c_int

lib.xzalgochain_neon_supported_lib.argtypes = []
lib.xzalgochain_neon_supported_lib.restype = ctypes.c_int

lib.xzalgochain_equals_lib.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.POINTER(ctypes.c_uint8)]
lib.xzalgochain_equals_lib.restype = ctypes.c_int

# Context structure
class XzalgoChainCTX(ctypes.Structure):
    _fields_ = [
        ("h", ctypes.c_uint64 * 5),
        ("little_box_state", (ctypes.c_uint64 * 10) * 10),
        ("big_box_state", (ctypes.c_uint64 * 5) * 5),
        ("buffer", ctypes.c_uint8 * 128),
        ("buffer_len", ctypes.c_size_t),
        ("total_bits", ctypes.c_uint64),
        ("simd_type", ctypes.c_uint8)
    ]

lib.xzalgochain_init_lib.argtypes = [ctypes.POINTER(XzalgoChainCTX)]
lib.xzalgochain_init_lib.restype = None

lib.xzalgochain_update_lib.argtypes = [ctypes.POINTER(XzalgoChainCTX), ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t]
lib.xzalgochain_update_lib.restype = None

lib.xzalgochain_final_lib.argtypes = [ctypes.POINTER(XzalgoChainCTX), ctypes.POINTER(ctypes.c_uint8)]
lib.xzalgochain_final_lib.restype = None

lib.xzalgochain_ctx_wipe_lib.argtypes = [ctypes.POINTER(XzalgoChainCTX)]
lib.xzalgochain_ctx_wipe_lib.restype = None

def hash_data(data):
    """Hash bytes data and return bytes"""
    hash_array = (ctypes.c_uint8 * XZALGOCHAIN_HASH_SIZE)()
    lib.xzalgochain_lib(ctypes.cast(data, ctypes.POINTER(ctypes.c_uint8)), 
                        len(data), hash_array)
    return bytes(hash_array)

def hash_string(text):
    """Hash string and return hex string"""
    return hash_data(text.encode('utf-8')).hex()

def hash_hex(data):
    """Hash bytes and return hex string"""
    return hash_data(data).hex()

class XzalgoHasher:
    def __init__(self):
        self.ctx = XzalgoChainCTX()
        lib.xzalgochain_init_lib(ctypes.byref(self.ctx))
    
    def update(self, data):
        if isinstance(data, str):
            data = data.encode('utf-8')
        lib.xzalgochain_update_lib(ctypes.byref(self.ctx),
                                   ctypes.cast(data, ctypes.POINTER(ctypes.c_uint8)),
                                   len(data))
    
    def finalize(self):
        hash_array = (ctypes.c_uint8 * XZALGOCHAIN_HASH_SIZE)()
        lib.xzalgochain_final_lib(ctypes.byref(self.ctx), hash_array)
        return bytes(hash_array)
    
    def __del__(self):
        if hasattr(self, 'ctx'):
            lib.xzalgochain_ctx_wipe_lib(ctypes.byref(self.ctx))

# Example usage
if __name__ == "__main__":
    # Library information
    print(f"Version: {lib.xzalgochain_version_lib().decode()}")
    print(f"Platform: {lib.xzalgochain_platform_info_lib().decode()}")
    print(f"SIMD: {lib.xzalgochain_get_simd_name_lib().decode()}")
    print(f"AVX2: {bool(lib.xzalgochain_avx2_supported_lib())}")
    print(f"NEON: {bool(lib.xzalgochain_neon_supported_lib())}")
    
    # Single-shot hash
    print(f"\nHash of 'Hello World': {hash_string('Hello World')}")
    
    # Multi-part hashing
    hasher = XzalgoHasher()
    hasher.update("Hello ")
    hasher.update("World")
    print(f"Multi-part hash: {hasher.finalize().hex()}")
    
    # File hashing
    def hash_file(filename):
        hasher = XzalgoHasher()
        with open(filename, 'rb') as f:
            for chunk in iter(lambda: f.read(8192), b''):
                hasher.update(chunk)
        return hasher.finalize()
    
    # Test with small file
    with open('test.txt', 'w') as f:
        f.write('Hello World')
    print(f"File hash: {hash_file('test.txt').hex()}")
```
See [INTEGRATION.md](INTEGRATION.md) for complete examples.

## Contributing

Contributions are welcome! Please ensure:
- Code follows existing style
- Tests pass (`make test`)
- New features include tests
- Performance is maintained

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

Copyright 2026 Xzrayツ

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

## Acknowledgments

- SHA-2 and SHA-3 round constants used for initialization
- Reference implementations for SIMD optimization techniques
- OpenMP for parallel processing support
- The cryptographic community for testing methodologies
