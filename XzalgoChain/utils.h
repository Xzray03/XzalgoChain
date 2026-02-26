/*
 * XzalgoChain - 320-bit Cryptographic Hash Function
 * Copyright 2026 Xzrayツ
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef XZALGOCHAIN_UTILS_H
#define XZALGOCHAIN_UTILS_H

/* Include configuration for hash size constants and standard string header for memcpy/memcmp */
#include "config.h"
#include <string.h>

/* ==================== ENDIAN DETECTION ==================== */

/**
 * Byte order (endianness) detection for the target platform
 * Determines whether the system uses big-endian or little-endian byte ordering
 * 
 * Big-endian: Most significant byte stored at lowest memory address
 * Little-endian: Least significant byte stored at lowest memory address (most common)
 * 
 * Detection uses compiler predefined macros in order of preference:
 * 1. Modern GCC/clang __BYTE_ORDER__ macro
 * 2. Traditional BIG_ENDIAN macros
 * 3. Default to little-endian if no big-endian macros detected
 */

/* Check for modern GCC/clang endian macros */
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
/* GCC 4.6+ and clang define __BYTE_ORDER__ and __ORDER_BIG_ENDIAN__ */
#define XZALGOCHAIN_BIG_ENDIAN 1

/* Check for traditional big-endian macros */
#elif defined(__BIG_ENDIAN__) || defined(_BIG_ENDIAN) || defined(BYTE_ORDER) && (BYTE_ORDER == BIG_ENDIAN)
/* Various compilers use these macros for big-endian detection */
#define XZALGOCHAIN_BIG_ENDIAN 1

#else
/* If no big-endian macros are defined, assume little-endian (most common) */
#define XZALGOCHAIN_BIG_ENDIAN 0
#endif

/**
 * Compiler-specific 64-bit byte swap intrinsics
 * Swaps the byte order of a 64-bit value (endian conversion)
 * 
 * Different compilers provide different built-in functions:
 * - GCC/clang: __builtin_bswap64
 * - MSVC: _byteswap_uint64
 * - Other: Error out (requires implementation)
 */
#if defined(__GNUC__) || defined(__clang__)
/* GCC and clang provide __builtin_bswap64 for efficient byte swapping */
#define XZALGOCHAIN_BSWAP64(x) __builtin_bswap64(x)

#elif defined(_MSC_VER)
/* Microsoft Visual C++ provides _byteswap_uint64 in <stdlib.h> */
#define XZALGOCHAIN_BSWAP64(x) _byteswap_uint64(x)

#else
/* Unsupported compiler - need to implement byte swap manually */
#error "Unsupported compiler: need __builtin_bswap64 or _byteswap_uint64"
#endif

/* ==================== ROTATION ==================== */

/**
 * Rotate left (circular left shift) a 64-bit value
 * @param x Value to rotate
 * @param n Number of bits to rotate left (0-63, automatically masked)
 * @return Rotated value: (x << n) | (x >> (64-n))
 */
static inline uint64_t rotl64(uint64_t x, uint64_t n) {
    /* Mask n to ensure it's within 0-63 range
     * This handles cases where n >= 64 by taking only lower 6 bits
     */
    n &= 63u;

    /* Use compiler built-in rotation if available
     * __has_builtin is a clang extension, also supported by recent GCC
     * Built-in rotations can be optimized to a single rotate instruction
     */
    #if defined(__has_builtin)
    #  if __has_builtin(__builtin_rotateleft64)
    /* Compiler provides built-in 64-bit rotate left */
    return __builtin_rotateleft64(x, n);
    #  endif
    #endif

    /* Fallback implementation using shifts and OR
     * This works on all compilers but may be less efficient
     * The mask (64u - n) & 63u ensures correct behavior when n=0
     */
    return (x << n) | (x >> ((64u - n) & 63u));
}

/**
 * Rotate right (circular right shift) a 64-bit value
 * @param x Value to rotate
 * @param n Number of bits to rotate right (0-63, automatically masked)
 * @return Rotated value: (x >> n) | (x << (64-n))
 */
static inline uint64_t rotr64(uint64_t x, uint64_t n) {
    /* Mask n to 0-63 range */
    n &= 63u;

    /* Use compiler built-in rotation if available */
    #if defined(__has_builtin)
    #  if __has_builtin(__builtin_rotateright64)
    /* Compiler provides built-in 64-bit rotate right */
    return __builtin_rotateright64(x, n);
    #  endif
    #endif

    /* Fallback implementation using shifts and OR */
    return (x >> n) | (x << ((64u - n) & 63u));
}

/* ==================== ENDIAN CONVERSION ==================== */

/**
 * Convert 8 bytes from memory to a 64-bit integer
 * Handles endianness differences between platforms
 * 
 * On little-endian systems: Direct memory copy is efficient
 * On big-endian systems: Manual byte assembly ensures correct interpretation
 * 
 * @param b Pointer to 8-byte array
 * @return 64-bit integer value read from bytes
 */
static inline uint64_t bytes_to_u64(const uint8_t *b) {
    uint64_t v;
    
    #if XZALGOCHAIN_BIG_ENDIAN
    /* Big-endian system: manually assemble bytes in little-endian order
     * This ensures consistent interpretation regardless of host endianness
     * Byte 0 becomes least significant, byte 7 becomes most significant
     */
    v = ((uint64_t)b[0]) |
        ((uint64_t)b[1] << 8) |
        ((uint64_t)b[2] << 16) |
        ((uint64_t)b[3] << 24) |
        ((uint64_t)b[4] << 32) |
        ((uint64_t)b[5] << 40) |
        ((uint64_t)b[6] << 48) |
        ((uint64_t)b[7] << 56);
    #else
    /* Little-endian system: direct memory copy is correct
     * Bytes are already stored in little-endian order
     */
    memcpy(&v, b, 8);
    #endif
    
    return v;
}

/**
 * Convert a 64-bit integer to 8 bytes in memory
 * Handles endianness differences between platforms
 * 
 * @param v 64-bit integer value to write
 * @param b Pointer to 8-byte array to receive the bytes
 */
static inline void u64_to_bytes(uint64_t v, uint8_t *b) {
    #if XZALGOCHAIN_BIG_ENDIAN
    /* Big-endian system: swap bytes to little-endian order before copying
     * This ensures consistent byte ordering in memory
     */
    uint64_t le = XZALGOCHAIN_BSWAP64(v);
    memcpy(b, &le, 8);
    #else
    /* Little-endian system: direct memory copy is correct */
    memcpy(b, &v, 8);
    #endif
}

/* ==================== HASH UTILITIES ==================== */

/**
 * Copy a complete XzalgoChain hash from source to destination
 * Safe: does nothing if either pointer is NULL
 * 
 * @param dst Destination buffer (must have at least XZALGOCHAIN_HASH_SIZE bytes)
 * @param src Source buffer (must have at least XZALGOCHAIN_HASH_SIZE bytes)
 */
static inline void xzalgochain_copy(uint8_t *dst, const uint8_t *src) {
    if (dst && src)
        memcpy(dst, src, XZALGOCHAIN_HASH_SIZE);
}

/**
 * Compare two XzalgoChain hashes for equality
 * Constant-time comparison is not required here as hash values
 * are not secrets (they're the output, not input)
 * 
 * @param h1 First hash value
 * @param h2 Second hash value
 * @return 1 if hashes are equal, 0 otherwise (also returns 0 if either pointer is NULL)
 */
static inline int xzalgochain_equals(const uint8_t *h1, const uint8_t *h2) {
    return (h1 && h2) &&
    (memcmp(h1, h2, XZALGOCHAIN_HASH_SIZE) == 0);
}

#endif /* XZALGOCHAIN_UTILS_H */