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

#ifndef XZALGOCHAIN_ALGORITHM_SIMD_AVX2_H
#define XZALGOCHAIN_ALGORITHM_SIMD_AVX2_H

/* This file is meant to be included only from algorithm_simd.h
 * when AVX2 is available on x86/x64 platforms.
 */

#include <immintrin.h> /* Intel SIMD intrinsics header */

/* ==================== AVX2 IMPLEMENTATION (x86/x64) ==================== */
/**
 * AVX2 (Advanced Vector Extensions 2) implementation for x86/x64 platforms
 * Processes 4 blocks in parallel using 256-bit SIMD registers
 * Each 256-bit register holds four 64-bit values
 */

#define XZALGOCHAIN_HAVE_AVX2 1

/* ---------------- SIMD ROTATIONS ---------------- */

/**
 * Left rotate each 64-bit lane in a 256-bit AVX2 register
 * @param v 256-bit vector containing four 64-bit values
 * @param r Rotation amount in bits (0-63)
 * @return Vector with each lane rotated left by r bits
 */
static inline __m256i rotl64x4(__m256i v, int r) {
    return _mm256_or_si256(_mm256_slli_epi64(v, r), _mm256_srli_epi64(v, 64 - r));
}

/**
 * Right rotate each 64-bit lane in a 256-bit AVX2 register
 * @param v 256-bit vector containing four 64-bit values
 * @param r Rotation amount in bits (0-63)
 * @return Vector with each lane rotated right by r bits
 */
static inline __m256i rotr64x4(__m256i v, int r) {
    return _mm256_or_si256(_mm256_srli_epi64(v, r), _mm256_slli_epi64(v, 64 - r));
}

/* ---------------- CONSTANT LOAD ---------------- */

/**
 * Macro to load four round constants into a 256-bit AVX2 register
 * Constants are loaded in reverse order (b+3, b+2, b+1, b) due to
 * _mm256_set_epi64x expecting arguments from highest to lowest lane
 *
 * @param b Base index for round constants
 * @return 256-bit vector with four consecutive round constants
 */
#define RC4(b) _mm256_set_epi64x(                            \
    ROUND_CONSTANTS[((b) + 3) & (ROUND_CONSTANTS_SIZE - 1)], \
    ROUND_CONSTANTS[((b) + 2) & (ROUND_CONSTANTS_SIZE - 1)], \
    ROUND_CONSTANTS[((b) + 1) & (ROUND_CONSTANTS_SIZE - 1)], \
    ROUND_CONSTANTS[((b) + 0) & (ROUND_CONSTANTS_SIZE - 1)])

/* ---------------- ARX MIX ---------------- */

/**
 * Mix lanes within a 256-bit vector to provide cross-lane diffusion
 * Performs permutations and XORs to mix data between the four 64-bit lanes
 *
 * @param v Input 256-bit vector
 * @return Mixed vector with cross-lane diffusion
 */
static inline __m256i mix_lanes(__m256i v) {
    /* Permute lanes: (1,0,3,2) - swap adjacent lane pairs */
    v = _mm256_permute4x64_epi64(v, _MM_SHUFFLE(1, 0, 3, 2));

    /* XOR with further permuted version (2,3,0,1) */
    v = _mm256_xor_si256(v, _mm256_permute4x64_epi64(v, _MM_SHUFFLE(2, 3, 0, 1)));

    /* Rotate left by 17 bits and XOR with original */
    __m256i rotated = _mm256_or_si256(
        _mm256_slli_epi64(v, 17),
        _mm256_srli_epi64(v, 47));
    return _mm256_xor_si256(v, rotated);
}

/**
 * 64-bit multiplication for AVX2 vectors
 * Emulates 64-bit multiplication using 32-bit multiplies since AVX2
 * doesn't have direct 64x64->64 multiply instruction
 *
 * @param a First operand vector
 * @param b Second operand vector
 * @return Vector with each lane = a.lane[i] * b.lane[i]
 */
static inline __m256i mullo64(__m256i a, __m256i b) {
    /* Multiply lower 32 bits of each 64-bit lane */
    __m256i lo = _mm256_mul_epu32(a, b);

    /* Multiply upper 32 bits of each 64-bit lane */
    __m256i hi = _mm256_mul_epu32(
        _mm256_srli_epi64(a, 32),
        _mm256_srli_epi64(b, 32));

    /* Combine: (hi << 32) + lo */
    return _mm256_add_epi64(lo, _mm256_slli_epi64(hi, 32));
}

/**
 * ARX (Add-Rotate-XOR) mixing function for AVX2 vectors
 * Core mixing operation combining addition, rotation, XOR, and lane mixing
 *
 * @param v Input vector to mix
 * @param salt Salt vector for additional entropy
 * @param rc Round constant vector
 * @param r1 First rotation amount
 * @param r2 Second rotation amount
 * @return Mixed vector
 */
static inline __m256i arx_mix(__m256i v, __m256i salt, __m256i rc, int r1, int r2) {
    /* Add salt for entropy injection */
    v = _mm256_add_epi64(v, salt);

    /* XOR with round constant */
    v = _mm256_xor_si256(v, rc);

    /* Add rotated version of self (left rotation) */
    v = _mm256_add_epi64(v, rotl64x4(v, r1));

    /* XOR with rotated version of self (right rotation) */
    v = _mm256_xor_si256(v, rotr64x4(v, r2));

    /* Mix lanes for cross-lane diffusion */
    v = mix_lanes(v);

    /* Multiply by carefully chosen constant for avalanche effect */
    return mullo64(v, _mm256_set1_epi64x(0x800000000000808AULL));
}

/* ---------------- HORIZONTAL REDUCTION ---------------- */

/**
 * Reduce a 256-bit AVX2 vector to a single 64-bit value
 * Combines all four lanes through XOR, permutations, and final mixing
 *
 * @param v Input 256-bit vector
 * @return 64-bit hash value derived from all lanes
 */
static inline uint64_t horizontal_xor256(__m256i v) {
    /* Start with lane mixing */
    v = mix_lanes(v);

    /* XOR with permuted version (swap adjacent pairs) */
    v = _mm256_xor_si256(v, _mm256_permute4x64_epi64(v, 0x4E));

    /* XOR with another permuted version */
    __m256i temp = _mm256_permute4x64_epi64(v, _MM_SHUFFLE(1, 0, 3, 2));
    v = _mm256_xor_si256(v, temp);

    /* Final permutation and XOR */
    temp = _mm256_permute4x64_epi64(v, _MM_SHUFFLE(2, 3, 0, 1));
    v = _mm256_xor_si256(v, temp);

    /* Extract and combine the two 128-bit halves */
    __m128i x = _mm_xor_si128(
        _mm256_castsi256_si128(v),
        _mm256_extracti128_si256(v, 1));

    /* Further combine within 128-bit register */
    x = _mm_xor_si128(x, _mm_srli_si128(x, 8));
    x = _mm_xor_si128(x, _mm_slli_si128(x, 4));

    /* Get final 64-bit value */
    uint64_t result = (uint64_t) _mm_cvtsi128_si64(x);

    /* Final diffusion sequence (same as scalar implementation) */
    result ^= result >> 31;
    result *= 0x0000000000000088ULL;
    result ^= result >> 29;
    result *= 0x8000000000008089ULL;
    result ^= result >> 32;
    result = rotr64(result, 17) ^ rotl64(result, 43);
    result *= 0x8000000080008081ULL;
    result ^= result >> 27;

    return result;
}

/* ---------------- HYBRID 4-BLOCK BATCH (AVX2) ---------------- */

/**
 * Main AVX2 execution function
 * Processes blocks in groups of 4 using SIMD instructions for maximum performance
 *
 * @param input Array of input blocks (each block is 10 64-bit words)
 * @param salt_scalar Salt value for this processing round
 * @param round_base Base round number for constant selection
 * @param num_blocks Total number of blocks to process
 */
static inline void little_box_execute_simd_avx2(uint64_t* input,
                                                uint64_t salt_scalar,
                                                uint64_t round_base,
                                                size_t num_blocks) {
    /* Create vector with salt replicated in all lanes */
    __m256i salt = _mm256_set1_epi64x(salt_scalar);

/* Initialize OpenMP parallel processing if available */
#ifdef _OPENMP
    int n_threads = omp_get_num_procs();
    omp_set_num_threads(n_threads);
#endif

/* OpenMP parallel for loop - processes blocks in groups of 4 */
#pragma omp for schedule(static)
    for (size_t blk = 0; blk < num_blocks; blk += 4) {
        /* Pointers to up to 4 blocks (handles edge cases) */
        uint64_t* in[4] = {NULL, NULL, NULL, NULL};
        for (int i = 0; i < 4; i++) {
            if (blk + i < num_blocks) in[i] = &input[(blk + i) * 10];
        }

        /* Load vectors from block data
         * Each vector contains one word from up to 4 blocks
         * Words are loaded in reverse order (block 3,2,1,0) due to AVX2 register layout
         */
        __m256i v0 = _mm256_set_epi64x(
            in[3] ? in[3][1] : 0,
            in[2] ? in[2][1] : 0,
            in[1] ? in[1][1] : 0,
            in[0] ? in[0][1] : 0);
        __m256i v0l = _mm256_set_epi64x(
            in[3] ? in[3][0] : 0,
            in[2] ? in[2][0] : 0,
            in[1] ? in[1][0] : 0,
            in[0] ? in[0][0] : 0);
        __m256i v1 = _mm256_set_epi64x(
            in[3] ? in[3][5] : 0,
            in[2] ? in[2][5] : 0,
            in[1] ? in[1][5] : 0,
            in[0] ? in[0][5] : 0);
        __m256i v1l = _mm256_set_epi64x(
            in[3] ? in[3][4] : 0,
            in[2] ? in[2][4] : 0,
            in[1] ? in[1][4] : 0,
            in[0] ? in[0][4] : 0);
        __m256i v2 = _mm256_set_epi64x(
            in[3] ? in[3][9] : 0,
            in[2] ? in[2][9] : 0,
            in[1] ? in[1][9] : 0,
            in[0] ? in[0][9] : 0);
        __m256i v2l = _mm256_set_epi64x(
            in[3] ? in[3][8] : 0,
            in[2] ? in[2][8] : 0,
            in[1] ? in[1][8] : 0,
            in[0] ? in[0][8] : 0);

        /* Apply ARX mixing to all vectors with appropriate round constants */
        v0 = arx_mix(v0, salt, RC4(round_base + 0), 7, 13);
        v0l = arx_mix(v0l, salt, RC4(round_base + 0), 7, 13);
        v1 = arx_mix(v1, salt, RC4(round_base + 4), 11, 17);
        v1l = arx_mix(v1l, salt, RC4(round_base + 4), 11, 17);
        v2 = arx_mix(v2, salt, RC4(round_base + 8), 19, 23);
        v2l = arx_mix(v2l, salt, RC4(round_base + 8), 19, 23);

        /* Mix lanes for all vectors */
        v0 = mix_lanes(v0);
        v0l = mix_lanes(v0l);
        v1 = mix_lanes(v1);
        v1l = mix_lanes(v1l);
        v2 = mix_lanes(v2);
        v2l = mix_lanes(v2l);

        /* Store results back to block 0
         * Permute patterns:
         * 0x00 = all lanes from position 0
         * 0x55 = all lanes from position 1
         * 0xAA = all lanes from position 2
         * 0xFF = all lanes from position 3
         */
        if (in[0]) {
            __m256i acc0 = _mm256_xor_si256(
                _mm256_xor_si256(
                    _mm256_permute4x64_epi64(v0, 0x00),
                    _mm256_permute4x64_epi64(v1, 0x00)),
                _mm256_permute4x64_epi64(v2, 0x00));
            in[0][0] = (uint64_t) _mm256_extract_epi64(v0, 0);
            in[0][1] = (uint64_t) _mm256_extract_epi64(v0, 1);
            in[0][4] = (uint64_t) _mm256_extract_epi64(v1, 0);
            in[0][5] = (uint64_t) _mm256_extract_epi64(v1, 1);
            in[0][8] = (uint64_t) _mm256_extract_epi64(v2, 0);
            in[0][9] = horizontal_xor256(acc0);
        }

        /* Store results back to block 1 */
        if (in[1]) {
            __m256i acc1 = _mm256_xor_si256(
                _mm256_xor_si256(
                    _mm256_permute4x64_epi64(v0, 0x55),
                    _mm256_permute4x64_epi64(v1, 0x55)),
                _mm256_permute4x64_epi64(v2, 0x55));
            in[1][0] = (uint64_t) _mm256_extract_epi64(v0, 2);
            in[1][1] = (uint64_t) _mm256_extract_epi64(v0, 3);
            in[1][4] = (uint64_t) _mm256_extract_epi64(v1, 2);
            in[1][5] = (uint64_t) _mm256_extract_epi64(v1, 3);
            in[1][8] = (uint64_t) _mm256_extract_epi64(v2, 2);
            in[1][9] = horizontal_xor256(acc1);
        }

        /* Store results back to block 2 (using v0l/v1l/v2l) */
        if (in[2]) {
            __m256i acc2 = _mm256_xor_si256(
                _mm256_xor_si256(
                    _mm256_permute4x64_epi64(v0l, 0xAA),
                    _mm256_permute4x64_epi64(v1l, 0xAA)),
                _mm256_permute4x64_epi64(v2l, 0xAA));
            in[2][0] = (uint64_t) _mm256_extract_epi64(v0l, 0);
            in[2][1] = (uint64_t) _mm256_extract_epi64(v0l, 1);
            in[2][4] = (uint64_t) _mm256_extract_epi64(v1l, 0);
            in[2][5] = (uint64_t) _mm256_extract_epi64(v1l, 1);
            in[2][8] = (uint64_t) _mm256_extract_epi64(v2l, 0);
            in[2][9] = horizontal_xor256(acc2);
        }

        /* Store results back to block 3 (using v0l/v1l/v2l) */
        if (in[3]) {
            __m256i acc3 = _mm256_xor_si256(
                _mm256_xor_si256(
                    _mm256_permute4x64_epi64(v0l, 0xFF),
                    _mm256_permute4x64_epi64(v1l, 0xFF)),
                _mm256_permute4x64_epi64(v2l, 0xFF));
            in[3][0] = (uint64_t) _mm256_extract_epi64(v0l, 2);
            in[3][1] = (uint64_t) _mm256_extract_epi64(v0l, 3);
            in[3][4] = (uint64_t) _mm256_extract_epi64(v1l, 2);
            in[3][5] = (uint64_t) _mm256_extract_epi64(v1l, 3);
            in[3][8] = (uint64_t) _mm256_extract_epi64(v2l, 2);
            in[3][9] = horizontal_xor256(acc3);
        }

        /* Cross-block mixing if we processed a full group of 4 blocks */
        if (blk + 3 < num_blocks) {
            uint64_t* b0 = &input[(blk + 0) * 10];
            uint64_t* b1 = &input[(blk + 1) * 10];
            uint64_t* b2 = &input[(blk + 2) * 10];
            uint64_t* b3 = &input[(blk + 3) * 10];

            /* XOR all final words together */
            uint64_t mix = b0[9] ^ b1[9] ^ b2[9] ^ b3[9];

            /* Apply diffusion to the mixed value */
            mix = rotr64(mix, 17) ^ rotl64(mix, 43);
            mix *= 0x9E3779B97F4A7C15ULL; /* Golden ratio constant */

            /* Feed back into each block with variations */
            b0[9] ^= mix;
            b1[9] ^= rotr64(mix, 11);
            b2[9] ^= rotl64(mix, 23);
            b3[9] ^= mix ^ (mix >> 31);
        }
    }
}

/* Clean up macro to prevent namespace pollution */
#undef RC4

#endif /* XZALGOCHAIN_ALGORITHM_SIMD_AVX2_H */
