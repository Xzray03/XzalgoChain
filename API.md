# XzalgoChain API Documentation

## Overview

XzalgoChain is a 320-bit cryptographic hash function that processes data in 1024-bit (128-byte) blocks and produces a 320-bit (40-byte, 80-hex) hash output. The library supports SIMD optimizations (AVX2, NEON) and provides both single-shot and incremental hashing interfaces.

---

## Core Header API (XzalgoChain.h)

### Data Structures

#### `XzalgoChain_CTX`
```c
typedef struct {
    uint64_t h[5];                                  /* Current hash state (320 bits) */
    uint64_t little_box_state[8][10];               /* LITTLE box states */
    uint64_t big_box_state[4][5];                   /* BIG box states */
    uint8_t buffer[128];                            /* Input buffer for partial blocks */
    size_t buffer_len;                              /* Bytes currently in buffer */
    uint64_t total_bits;                            /* Total bits processed */
    uint8_t simd_type;                              /* Detected SIMD type */
} XzalgoChain_CTX;
```

**SIMD Type Values:**
- `SIMD_NONE` - No SIMD support
- `SIMD_AVX2` - AVX2 support detected
- `SIMD_NEON` - NEON support detected

---

### Core Functions

#### Initialization and Cleanup

```c
void xzalgochain_init(XzalgoChain_CTX *ctx);
```
Initializes a new hash context. Detects SIMD capabilities, sets initial hash values, and clears all internal state.

**Parameters:**
- `ctx` - Pointer to uninitialized context structure

---

```c
void xzalgochain_ctx_reset(XzalgoChain_CTX *ctx);
```
Resets context to initial state (same as re-initializing).

**Parameters:**
- `ctx` - Pointer to initialized context

---

```c
void xzalgochain_ctx_wipe(XzalgoChain_CTX *ctx);
```
Securely clears sensitive data from context by overwriting with zeros.

**Parameters:**
- `ctx` - Pointer to context to wipe

---

#### Incremental Hashing

```c
void xzalgochain_update(XzalgoChain_CTX *ctx, const uint8_t *data, size_t len);
```
Processes additional data for hashing. Handles buffering of partial blocks.

**Parameters:**
- `ctx` - Initialized context
- `data` - Input data bytes
- `len` - Length of input data

---

```c
void xzalgochain_final(XzalgoChain_CTX *ctx, uint8_t output[XZALGOCHAIN_HASH_SIZE]);
```
Finalizes hash computation, applies padding, performs final mixing, and produces output.

**Parameters:**
- `ctx` - Context with processed data
- `output` - Output buffer (must be at least 40 bytes)

---

#### Single-Shot Hashing

```c
void xzalgochain(const uint8_t *data, size_t len, uint8_t output[XZALGOCHAIN_HASH_SIZE]);
```
Computes hash of a complete message in one call. Convenience function for simple use cases.

**Parameters:**
- `data` - Input data bytes
- `len` - Length of input data
- `output` - Output buffer (must be at least 40 bytes)

---

### Utility Functions

```c
void xzalgochain_copy(uint8_t *dst, const uint8_t *src);
```
Copies a hash value from source to destination.

**Parameters:**
- `dst` - Destination buffer (40 bytes)
- `src` - Source buffer (40 bytes)

---

```c
int xzalgochain_equals(const uint8_t *h1, const uint8_t *h2);
```
Constant-time comparison of two hash values.

**Parameters:**
- `h1` - First hash buffer (40 bytes)
- `h2` - Second hash buffer (40 bytes)

**Returns:** `1` if equal, `0` if not equal

---

### Information Functions

```c
const char* xzalgochain_version(void);
```
Returns version string: `"XzalgoChain 0.0.1.2 - 320-bit"`

---

```c
const char* xzalgochain_platform_info(void);
```
Returns platform name string (via `xzalgochain_get_platform_name()`)

---

### SIMD Control Functions

```c
void xzalgochain_force_scalar(int force);
```
Forces scalar mode (disables SIMD) when `force` is non-zero. Thread-safe with atomic operations when available.

---

```c
int xzalgochain_is_forced_scalar(void);
```
Returns `1` if scalar mode is forced, `0` otherwise.

---

### Internal Helper Functions

> **Note:** These functions are static inline and primarily for internal use:

- `process_block()` - Processes single 1024-bit block
- `generate_salt()` - Generates salt values from hash state
- `big_box_execute()` - Executes BIG box transformation
- `little_box_complete()` - Checks LITTLE box completion status
- `extra_mix()` - Additional mixing for improved distribution

---

## Library API (xzalgochain.c)

All library functions are prefixed with `_lib` suffix and provide exported symbols for dynamic/shared library usage.

### Core Hash Function

```c
void xzalgochain_lib(const uint8_t *data, size_t len, uint8_t output[XZALGOCHAIN_HASH_SIZE]);
```
Single-shot hash computation. Wraps `xzalgochain()` with context cleanup.

---

### Context Management (Library Version)

| Function | Description | Parameters |
|----------|-------------|------------|
| `xzalgochain_init_lib(XzalgoChain_CTX *ctx)` | Initialize context | `ctx` - Context pointer |
| `xzalgochain_update_lib(XzalgoChain_CTX *ctx, const uint8_t *data, size_t len)` | Update hash | `ctx`, `data`, `len` |
| `xzalgochain_final_lib(XzalgoChain_CTX *ctx, uint8_t output[40])` | Finalize hash | `ctx`, `output` |
| `xzalgochain_ctx_reset_lib(XzalgoChain_CTX *ctx)` | Reset context | `ctx` |
| `xzalgochain_ctx_wipe_lib(XzalgoChain_CTX *ctx)` | Wipe context | `ctx` |

---

### Utility Functions (Library Version)

```c
void xzalgochain_copy_lib(uint8_t *dst, const uint8_t *src);
int xzalgochain_equals_lib(const uint8_t *h1, const uint8_t *h2);
```

---

### Information Functions (Library Version)

```c
const char* xzalgochain_version_lib(void);
const char* xzalgochain_platform_info_lib(void);
const char* xzalgochain_get_simd_name_lib(void);
int xzalgochain_get_simd_type_lib(void);
```

---

### SIMD Support Detection (Library Version)

```c
int xzalgochain_avx2_supported_lib(void);
```
Returns `1` if AVX2 is supported on x86/x86_64, `0` otherwise.

---

```c
int xzalgochain_neon_supported_lib(void);
```
Returns `1` if NEON is supported on ARM/ARM64, `0` otherwise.

---

### Scalar Mode Control (Library Version)

```c
void xzalgochain_force_scalar_lib(int force);
int xzalgochain_is_forced_scalar_lib(void);
```

---

## WASM API (xzalgochain_wasm.c)

All WASM functions are prefixed with `_wasm` suffix and are designed for WebAssembly module exports.

### Core Hash Function (WASM Version)

```c
void xzalgochain_wasm(const uint8_t *data, size_t len, uint8_t output[XZALGOCHAIN_HASH_SIZE]);
```
Single-shot hash computation for WASM environments. Automatically wipes context after use.

---

### Context Management (WASM Version)

| Function | Description | Parameters |
|----------|-------------|------------|
| `xzalgochain_init_wasm(XzalgoChain_CTX *ctx)` | Initialize context | `ctx` |
| `xzalgochain_update_wasm(XzalgoChain_CTX *ctx, const uint8_t *data, size_t len)` | Update hash | `ctx`, `data`, `len` |
| `xzalgochain_final_wasm(XzalgoChain_CTX *ctx, uint8_t output[40])` | Finalize hash | `ctx`, `output` |
| `xzalgochain_ctx_reset_wasm(XzalgoChain_CTX *ctx)` | Reset context | `ctx` |
| `xzalgochain_ctx_wipe_wasm(XzalgoChain_CTX *ctx)` | Wipe context | `ctx` |

---

### Utility Functions (WASM Version)

```c
void xzalgochain_copy_wasm(uint8_t *dst, const uint8_t *src);
int xzalgochain_equals_wasm(const uint8_t *h1, const uint8_t *h2);
```

---

### Information Functions (WASM Version)

```c
const char* xzalgochain_version_wasm(void);
```

---

## Usage Examples

### Single-Shot Hashing (Native)
```c
#include "XzalgoChain/XzalgoChain.h"
#include <stdio.h>

int main() {
    uint8_t data[] = "Hello, World!";
    uint8_t hash[40];
    
    xzalgochain(data, strlen((char*)data), hash);
    
    printf("Hash: ");
    for(int i = 0; i < 40; i++) printf("%02x", hash[i]);
    printf("\n");
    
    return 0;
}
```

### Incremental Hashing
```c
XzalgoChain_CTX ctx;
uint8_t hash[40];
uint8_t *chunk1, *chunk2;
size_t len1, len2;

xzalgochain_init(&ctx);
xzalgochain_update(&ctx, chunk1, len1);
xzalgochain_update(&ctx, chunk2, len2);
xzalgochain_final(&ctx, hash);
```

### Using Library Exports
```c
// When linking against shared library
XzalgoChain_CTX ctx;
uint8_t hash[40];

xzalgochain_init_lib(&ctx);
xzalgochain_update_lib(&ctx, data, len);
xzalgochain_final_lib(&ctx, hash);
xzalgochain_ctx_wipe_lib(&ctx);
```

### WASM Usage (JavaScript)
```javascript
// Assuming module is loaded
const data = new Uint8Array([...]);
const hash = new Uint8Array(40);

// Single-shot
Module._xzalgochain_wasm(
    data.byteOffset, 
    data.length, 
    hash.byteOffset
);

// Incremental
const ctx = new Module.XzalgoChain_CTX();
Module._xzalgochain_init_wasm(ctx.ptr);
Module._xzalgochain_update_wasm(ctx.ptr, data.byteOffset, data.length);
Module._xzalgochain_final_wasm(ctx.ptr, hash.byteOffset);
Module._xzalgochain_ctx_wipe_wasm(ctx.ptr);
```

---

## Thread Safety

- Context functions are **not** thread-safe for the same context
- Different contexts can be used safely in different threads
- `xzalgochain_force_scalar()` uses atomic operations when available (C11 atomics)
- Without C11 atomics, scalar mode control is not thread-safe

---

## Return Values

| Function | Return Type | Success | Failure |
|----------|------------|---------|---------|
| `xzalgochain_equals` | `int` | `1` | `0` |
| `xzalgochain_is_forced_scalar` | `int` | `1` (forced) | `0` (not forced) |
| SIMD support functions | `int` | `1` | `0` |
| Version/Info functions | `const char*` | Valid string | N/A |

All other functions return `void` and assume valid parameters. Null pointers are checked internally with early returns.

---

## Information

Hash output size: **40 bytes** (320 bits)  
Block size: **128 bytes** (1024 bits)  
Internal state: **320 bits** (5 × 64-bit words)
