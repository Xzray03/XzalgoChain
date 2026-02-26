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

#ifndef XZALGOCHAIN_PLATFORM_DETECT_H
#define XZALGOCHAIN_PLATFORM_DETECT_H

/* Include configuration header for platform-independent definitions */
#include "config.h"

/**
 * Platform-specific includes for architecture detection and SIMD support
 * 
 * For x86/x64 platforms: include unistd.h for system calls and POSIX functions
 * This provides basic system interface regardless of compiler
 */
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#include <unistd.h>

/**
 * For ARM platforms: conditionally include ARM NEON intrinsics header
 * NEON is ARM's SIMD (Single Instruction Multiple Data) extension
 * Only include if NEON is available on the target architecture
 */
#elif defined(__ARM_ARCH) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
#if defined(__ARM_NEON) || defined(__ARM_NEON__)
#include <arm_neon.h>  /* ARM NEON SIMD intrinsics */
#endif
#endif

/* ==================== PLATFORM DETECTION ==================== */

/**
 * Get human-readable platform name string
 * Uses compiler preprocessor macros to identify the target platform
 * and architecture at compile time
 * 
 * @return String constant describing the platform
 */
static inline const char* xzalgochain_get_platform_name(void) {
    #if defined(__i386__) || defined(_M_IX86)
    /* Intel/AMD 32-bit x86 architecture
     * __i386__ : GCC/clang for 32-bit x86
     * _M_IX86 : Microsoft Visual C for 32-bit x86
     */
    return "x86 32-bit";
    
    #elif defined(__x86_64__) || defined(_M_X64)
    /* Intel/AMD 64-bit x86_64 architecture (also called AMD64)
     * __x86_64__ : GCC/clang for 64-bit x86
     * _M_X64 : Microsoft Visual C for 64-bit x86
     */
    return "x86 64-bit";
    
    #elif defined(__arm__) || defined(_M_ARM)
    /* ARM 32-bit architecture
     * __arm__ : GCC/clang for ARM
     * _M_ARM : Microsoft Visual C for ARM
     */
    return "ARM 32-bit";
    
    #elif defined(__aarch64__) || defined(_M_ARM64)
    /* ARM 64-bit architecture (ARMv8-A)
     * __aarch64__ : GCC/clang for ARM64
     * _M_ARM64 : Microsoft Visual C for ARM64
     */
    return "ARM 64-bit";
    
    #elif defined(__APPLE__) && defined(__MACH__)
    /* Apple platforms (both macOS and iOS)
     * __APPLE__ : Defined for all Apple products
     * __MACH__ : Defined for Mach kernel (macOS/iOS)
     */
    return "Apple (iOS/macOS)";
    
    #elif defined(__ANDROID__)
    /* Android platform (Linux-based but with specific environment)
     * __ANDROID__ : Defined for Android builds
     */
    return "Android";
    
    #elif defined(__linux__)
    /* Generic Linux platform (non-Android)
     * __linux__ : Defined for Linux systems
     */
    return "Linux";
    
    #elif defined(_WIN32)
    /* Windows platform (both 32-bit and 64-bit)
     * _WIN32 : Defined for all Windows builds
     * Note: _WIN64 is also defined for 64-bit Windows
     */
    return "Windows";
    
    #else
    /* Unknown or unrecognized platform
     * Fallback for unsupported systems
     */
    return "Unknown platform";
    #endif
}

/**
 * Check if the current platform is x86/x64 based
 * Useful for architecture-specific code paths and optimizations
 * 
 * @return 1 if x86 or x86_64, 0 otherwise
 */
static inline int xzalgochain_is_x86(void) {
    #if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
    /* Any x86 or x86_64 macro indicates x86 architecture family */
    return 1;
    #else
    /* Not x86/x64 */
    return 0;
    #endif
}

/**
 * Check if the current platform is ARM based
 * Useful for architecture-specific code paths and NEON optimizations
 * 
 * @return 1 if ARM (any variant), 0 otherwise
 */
static inline int xzalgochain_is_arm(void) {
    #if defined(__arm__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64)
    /* Any ARM macro indicates ARM architecture family
     * Includes both 32-bit (arm, _M_ARM) and 64-bit (aarch64, _M_ARM64)
     */
    return 1;
    #else
    /* Not ARM */
    return 0;
    #endif
}

/**
 * Check if the platform is 64-bit
 * Useful for selecting optimal data types and algorithms
 * 
 * @return 1 if 64-bit architecture, 0 if 32-bit
 */
static inline int xzalgochain_is_64bit(void) {
    #if defined(__x86_64__) || defined(_M_X64) || defined(__aarch64__) || defined(_M_ARM64)
    /* 64-bit architectures:
     * x86_64 / _M_X64 : x86 64-bit
     * aarch64 / _M_ARM64 : ARM 64-bit
     */
    return 1;
    #else
    /* Assume 32-bit if no 64-bit macro is defined */
    return 0;
    #endif
}

/**
 * Implementation function for platform information string
 * Simple wrapper around xzalgochain_get_platform_name()
 * Maintained for API consistency and potential future extensions
 * 
 * @return String constant describing the platform
 */
static inline const char* xzalgochain_platform_info_impl(void) {
    return xzalgochain_get_platform_name();
}

#endif /* XZALGOCHAIN_PLATFORM_DETECT_H */