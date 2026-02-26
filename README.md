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
├── LICENSE                             # Project license (MIT/GPL/etc)
├── logo.svg                            # XzalgoChain project logo
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

### Python (using ctypes, You need to compile the shared library yourself)
```python
import ctypes
lib = ctypes.CDLL("./libxzalgochain.so")
hash = (ctypes.c_uint8 * 40)()
lib.xzalgochain(b"Hello", 5, hash)
print(bytes(hash).hex())
```

### Rust (FFI binding)
```rust
extern "C" {
    fn xzalgochain(data: *const u8, len: usize, output: *mut u8);
}

let mut hash = [0u8; 40];
unsafe {
    xzalgochain(b"Hello".as_ptr(), 5, hash.as_mut_ptr());
}
println!("{}", hex::encode(hash));
```

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
