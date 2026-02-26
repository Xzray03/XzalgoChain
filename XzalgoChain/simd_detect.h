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

#ifndef XZALGOCHAIN_SIMD_DETECT_H
#define XZALGOCHAIN_SIMD_DETECT_H

/* Include configuration for SIMD bit flags and platform detection headers */
#include "config.h"
#include "platform_detect.h"

/**
 * Platform-specific includes for CPU feature detection
 * For x86/x64 platforms with GCC/clang: include cpuid headers
 * immintrin.h provides Intel SIMD intrinsics
 * cpuid.h provides CPUID instruction wrappers for GCC/clang
 */
#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#if defined(__GNUC__) || defined(__clang__)
#include <immintrin.h>
#include <cpuid.h>
#endif
#endif

/* ==================== INTERNAL DETECTION FUNCTIONS ==================== */

/**
 * Internal function to detect AVX2 support on x86/x64 platforms
 * Uses CPUID instruction to query processor features
 * Multiple implementations for different compilers:
 * - GCC/clang: __cpuid_count() intrinsic
 * - MSVC: __cpuidex() intrinsic
 * - Generic: inline assembly fallback
 * 
 * @return 1 if AVX2 is supported, 0 otherwise
 */
static inline int _detect_avx2_x86(void) {
    #if defined(__i386__) || defined(__x86_64__)

    /* GCC/clang implementation using built-in CPUID functions */
    #if defined(__GNUC__) || defined(__clang__)
    unsigned int eax, ebx, ecx, edx;
    
    /* Check maximum CPUID level first */
    if (__get_cpuid_max(0, NULL) < 7)
        return 0;  /* CPUID leaf 7 not supported */
    
    /* Query leaf 7, subleaf 0 for extended features */
    __cpuid_count(7, 0, eax, ebx, ecx, edx);
    
    /* BIT_AVX2 (1 << 5) indicates AVX2 support in EBX */
    return (ebx & BIT_AVX2) != 0 ? 1 : 0;

    /* Microsoft Visual C++ implementation */
    #elif defined(_MSC_VER)
    int cpuInfo[4];
    
    /* Check maximum CPUID level */
    __cpuid(cpuInfo, 0);
    if (cpuInfo[0] < 7)
        return 0;
    
    /* Query leaf 7 using __cpuidex */
    __cpuidex(cpuInfo, 7, 0);
    return (cpuInfo[1] & BIT_AVX2) != 0 ? 1 : 0;

    #else
    /* Generic inline assembly implementation for compilers without built-ins */
    uint32_t eax, ebx, ecx, edx;
    
    /* First, check if CPUID is supported by toggling ID bit in EFLAGS */
    #if defined(__x86_64__)
    /* 64-bit version */
    __asm__ volatile (
        "pushfq\n\t"                 /* Push RFLAGS onto stack */
        "popq %%rax\n\t"             /* Pop RFLAGS into RAX */
        "movq %%rax, %%rcx\n\t"      /* Save original flags in RCX */
        "xorq $0x200000, %%rax\n\t"  /* Toggle ID bit (bit 21) */
        "pushq %%rax\n\t"            /* Push modified flags */
        "popfq\n\t"                  /* Pop into RFLAGS */
        "pushfq\n\t"                 /* Push RFLAGS again */
        "popq %%rax\n\t"             /* Pop into RAX */
        "xorq %%rcx, %%rax"          /* XOR with original - if ID bit changed, CPUID supported */
        : "=a"(eax)
        :
        : "rcx"
    );
    #else
    /* 32-bit version */
    __asm__ volatile (
        "pushfl\n\t"                 /* Push EFLAGS onto stack */
        "popl %%eax\n\t"             /* Pop EFLAGS into EAX */
        "movl %%eax, %%ecx\n\t"      /* Save original flags in ECX */
        "xorl $0x200000, %%eax\n\t"  /* Toggle ID bit (bit 21) */
        "pushl %%eax\n\t"            /* Push modified flags */
        "popfl\n\t"                  /* Pop into EFLAGS */
        "pushfl\n\t"                 /* Push EFLAGS again */
        "popl %%eax\n\t"             /* Pop into EAX */
        "xorl %%ecx, %%eax"          /* XOR with original - if ID bit changed, CPUID supported */
        : "=a"(eax)
        :
        : "ecx"
    );
    #endif

    /* If ID bit couldn't be toggled, CPUID not supported */
    if (!(eax & 0x200000))
        return 0;

    /* Get maximum CPUID level */
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0), "c"(0)
    );

    if (eax < 7)
        return 0;  /* Leaf 7 not supported */

    /* Query leaf 7 for AVX2 support */
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(7), "c"(0)
    );

    /* Check AVX2 bit (bit 5 in EBX) */
    return (ebx & (1u << 5)) != 0 ? 1 : 0;
    #endif

    #else
    /* Not x86/x64 platform - AVX2 not supported */
    return 0;
    #endif
}

/**
 * Internal function to detect NEON support on ARM platforms
 * Uses different detection methods per operating system:
 * - Linux/Android: Check /proc/cpuinfo for "neon" or "asimd" flags
 * - Apple (iOS/macOS): NEON always available on modern devices
 * - Windows: NEON always available on ARM targets
 * - Other: Assume NEON is available if ARM NEON macros are defined
 * 
 * @return 1 if NEON is supported, 0 otherwise
 */
static inline int _detect_neon_arm(void) {
    #if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(_M_ARM) || defined(_M_ARM64)
    
    /* Linux/Android detection via /proc/cpuinfo */
    #if defined(__linux__) || defined(ANDROID) || defined(__ANDROID__)
    FILE *cpuinfo = fopen("/proc/cpuinfo", "r");
    if (cpuinfo) {
        char buffer[1024];
        /* Scan cpuinfo line by line looking for NEON/asimd feature flags */
        while (fgets(buffer, sizeof(buffer), cpuinfo)) {
            if (strstr(buffer, "neon") || strstr(buffer, "asimd")) {
                fclose(cpuinfo);
                return 1;  /* NEON found */
            }
        }
        fclose(cpuinfo);
    }
    return 0;  /* NEON not found in cpuinfo */
    
    /* Apple platforms (iOS, macOS on Apple Silicon) */
    #elif defined(__APPLE__) && (defined(__arm__) || defined(__aarch64__))
    /* All Apple ARM devices have NEON support */
    return 1;
    
    /* Windows on ARM */
    #elif defined(_WIN32) && (defined(_M_ARM) || defined(_M_ARM64))
    /* Windows ARM targets always have NEON */
    return 1;
    
    #else
    /* For other platforms, assume NEON is available if macros are defined */
    return 1;
    #endif
    
    #else
    /* ARM NEON macros not defined - NEON not supported */
    return 0;
    #endif
}

/* ==================== PUBLIC SIMD DETECTION API ==================== */

/**
 * Public API to check if AVX2 is supported on current platform
 * First verifies we're on x86, then calls internal detection
 * 
 * @return 1 if AVX2 is available, 0 otherwise
 */
static inline int xzalgochain_avx2_supported(void) {
    if (!xzalgochain_is_x86())
        return 0;
    return _detect_avx2_x86();
}

/**
 * Public API to check if NEON is supported on current platform
 * First verifies we're on ARM, then calls internal detection
 * 
 * @return 1 if NEON is available, 0 otherwise
 */
static inline int xzalgochain_neon_supported(void) {
    if (!xzalgochain_is_arm())
        return 0;
    return _detect_neon_arm();
}

/**
 * Get the type of SIMD available on current platform
 * Checks for AVX2 first (x86), then NEON (ARM), falls back to SIMD_NONE
 * 
 * @return SIMD type constant: SIMD_AVX2, SIMD_NEON, or SIMD_NONE
 */
static inline int xzalgochain_get_simd_type(void) {
    if (xzalgochain_is_x86() && _detect_avx2_x86())
        return SIMD_AVX2;
    
    if (xzalgochain_is_arm() && _detect_neon_arm())
        return SIMD_NEON;
    
    return SIMD_NONE;
}

/**
 * Get human-readable name of the available SIMD type
 * Useful for logging and version information
 * 
 * @return String constant: "AVX2", "NEON", or "None"
 */
static inline const char* xzalgochain_get_simd_name(void) {
    int simd_type = xzalgochain_get_simd_type();
    
    switch (simd_type) {
        case SIMD_AVX2:
            return "AVX2";
        case SIMD_NEON:
            return "NEON";
        default:
            return "None";
    }
}

/**
 * Legacy/deprecated function for SIMD detection
 * Returns SIMD type as uint8_t for backward compatibility
 * 
 * @return SIMD type as uint8_t (cast from xzalgochain_get_simd_type())
 */
static inline uint8_t detect_simd(void) {
    return (uint8_t)xzalgochain_get_simd_type();
}

/**
 * Legacy/deprecated function for SIMD type
 * Maintains API compatibility with older versions
 * 
 * @return SIMD type (same as xzalgochain_get_simd_type())
 */
static inline int xzalgochain_simd_type(void) {
    return xzalgochain_get_simd_type();
}

#endif /* XZALGOCHAIN_SIMD_DETECT_H */
