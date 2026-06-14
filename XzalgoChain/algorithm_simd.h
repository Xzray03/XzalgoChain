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

#ifndef XZALGOCHAIN_ALGORITHM_SIMD_H
#define XZALGOCHAIN_ALGORITHM_SIMD_H

/* Include configuration for platform detection, base algorithm header for
 * cryptographic primitives and round constants, and scalar implementation
 * for fallback operations
 */
#include "config.h"
#include "algorithm.h"
#include "algorithm_scalar.h"
#include <stdint.h>
#include <stddef.h>

/* OpenMP header for parallel processing support if enabled */
#ifdef _OPENMP
    #include <omp.h>
#endif

/* ==================== FORWARD DECLARATION ==================== */
/**
 * Forward declaration of function to check if scalar mode is forced
 * This function is defined elsewhere (likely in XzalgoChain.c)
 */
static inline int xzalgochain_is_forced_scalar(void);

/* ==================== INCLUDE SIMD IMPLEMENTATIONS ==================== */
/**
 * Include AVX2 implementation for x86/x64 platforms
 */
#if defined(XZALGOCHAIN_AVX2_SUPPORT) || (defined(__AVX2__) && (defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86)))
    #include "algorithm_simd-avx2.h"
#endif

/**
 * Include NEON implementation for ARM platforms
 */
#if (defined(XZALGOCHAIN_NEON_SUPPORT) ||               \
     ((defined(__ARM_NEON) || defined(__ARM_NEON__)) && \
      (defined(__arm__) || defined(__aarch64__) || defined(_M_ARM) || defined(_M_ARM64))))
    #include "algorithm_simd-neon.h"
#endif

/* ==================== WRAPPER ==================== */

/**
 * Universal SIMD execution wrapper
 * Automatically selects the best available SIMD implementation
 * Falls back to scalar if SIMD is unavailable or disabled
 *
 * @param input Array of input blocks
 * @param salt_scalar Salt value
 * @param round_base Round base
 * @param num_blocks Number of blocks to process
 */
static inline void little_box_execute_simd(uint64_t* input,
                                           uint64_t salt_scalar,
                                           uint64_t round_base,
                                           size_t num_blocks) {
    /* If scalar mode is forced, use scalar implementation */
    if (xzalgochain_is_forced_scalar()) {
        for (size_t i = 0; i < num_blocks; i++) {
            little_box_execute_scalar(&input[i * 10],
                                      salt_scalar,
                                      round_base,
                                      1);
        }
        return;
    }

/* Select best available SIMD implementation */
#if defined(XZALGOCHAIN_HAVE_AVX2)
    /* AVX2 available on x86/x64 */
    little_box_execute_simd_avx2(input, salt_scalar, round_base, num_blocks);
#elif defined(XZALGOCHAIN_HAVE_NEON)
    /* NEON available on ARM */
    little_box_execute_simd_neon(input, salt_scalar, round_base, num_blocks);
#else
    /* No SIMD available - use scalar with optional OpenMP parallelization */
    #pragma omp for
    for (size_t i = 0; i < num_blocks; i++) {
        little_box_execute_scalar(&input[i * 10],
                                  salt_scalar,
                                  round_base,
                                  1);
    }
#endif
}

#endif /* XZALGOCHAIN_ALGORITHM_SIMD_H */
