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

#ifndef XZALGOCHAIN_ALGORITHM_SCALAR_H
#define XZALGOCHAIN_ALGORITHM_SCALAR_H

/* Include configuration for platform detection and base algorithm header
 * for cryptographic primitives and round constants
 */
#include "config.h"
#include "algorithm.h"

/* OpenMP header for parallel processing support if enabled */
#ifdef _OPENMP
#include <omp.h>
#endif

/* ==================== SCALAR IMPLEMENTATION ==================== */

/**
 * This file provides a scalar (non-SIMD) implementation of the XzalgoChain
 * core algorithm. It simulates 256-bit vector operations using 64-bit lanes
 * and provides parallel processing support via OpenMP.
 */

/* ---------------- SCALAR ROTATIONS ---------------- */

/**
 * 64-bit left rotation (scalar implementation)
 * @param v Value to rotate
 * @param r Number of bits to rotate left (0-63)
 * @return Rotated value
 */
static inline uint64_t rotl64_scalar(uint64_t v, int r) {
    return (v << r) | (v >> (64 - r));
}

/**
 * 64-bit right rotation (scalar implementation)
 * @param v Value to rotate
 * @param r Number of bits to rotate right (0-63)
 * @return Rotated value
 */
static inline uint64_t rotr64_scalar(uint64_t v, int r) {
    return (v >> r) | (v << (64 - r));
}

/* ---------------- CONSTANT LOAD ---------------- */

/**
 * Macro to access round constants with circular indexing
 * Uses bitwise AND for fast modulo operation when ROUND_CONSTANTS_SIZE is power of 2
 */
#define RC(b) (ROUND_CONSTANTS[(b) & (ROUND_CONSTANTS_SIZE-1)])

/* ---------------- 256-bit VECTOR SCALAR ---------------- */

/**
 * 256-bit vector type implemented as an array of 4 64-bit lanes
 * This simulates SIMD vector operations using scalar code
 */
typedef struct {
    uint64_t lane[4];  /* Four 64-bit lanes forming a 256-bit vector */
} vec256_t;

/**
 * Create a 256-bit vector from four 64-bit values
 * @param l0 First lane (least significant in some contexts)
 * @param l1 Second lane
 * @param l2 Third lane
 * @param l3 Fourth lane (most significant in some contexts)
 * @return 256-bit vector with specified lane values
 */
static inline vec256_t vec256_set(uint64_t l0, uint64_t l1, uint64_t l2, uint64_t l3) {
    vec256_t v = {{l0, l1, l2, l3}};
    return v;
}

/**
 * Create a 256-bit vector with all lanes set to the same value
 * @param val Value to replicate across all 4 lanes
 * @return 256-bit vector with all lanes = val
 */
static inline vec256_t vec256_set1(uint64_t val) {
    vec256_t v = {{val, val, val, val}};
    return v;
}

/**
 * Add two 256-bit vectors lane-wise
 * @param a First vector
 * @param b Second vector
 * @return Vector where each lane = a.lane[i] + b.lane[i]
 */
static inline vec256_t vec256_add(vec256_t a, vec256_t b) {
    vec256_t r;
    for (int i = 0; i < 4; i++) r.lane[i] = a.lane[i] + b.lane[i];
    return r;
}

/**
 * XOR two 256-bit vectors lane-wise
 * @param a First vector
 * @param b Second vector
 * @return Vector where each lane = a.lane[i] ^ b.lane[i]
 */
static inline vec256_t vec256_xor(vec256_t a, vec256_t b) {
    vec256_t r;
    for (int i = 0; i < 4; i++) r.lane[i] = a.lane[i] ^ b.lane[i];
    return r;
}

/**
 * Left rotate each lane of a 256-bit vector
 * @param v Input vector
 * @param r Rotation amount (bits)
 * @return Vector with each lane rotated left by r bits
 */
static inline vec256_t vec256_rotl(vec256_t v, int r) {
    vec256_t res;
    for (int i = 0; i < 4; i++) res.lane[i] = rotl64_scalar(v.lane[i], r);
    return res;
}

/**
 * Right rotate each lane of a 256-bit vector
 * @param v Input vector
 * @param r Rotation amount (bits)
 * @return Vector with each lane rotated right by r bits
 */
static inline vec256_t vec256_rotr(vec256_t v, int r) {
    vec256_t res;
    for (int i = 0; i < 4; i++) res.lane[i] = rotr64_scalar(v.lane[i], r);
    return res;
}

/**
 * Permute lanes of a 256-bit vector according to immediate value
 * Each 2-bit field in imm selects a source lane:
 * bits 0-1: destination lane 0 source
 * bits 2-3: destination lane 1 source
 * bits 4-5: destination lane 2 source
 * bits 6-7: destination lane 3 source
 * 
 * @param v Input vector
 * @param imm Permutation pattern (8-bit value)
 * @return Vector with lanes permuted as specified
 */
static inline vec256_t vec256_permute(vec256_t v, int imm) {
    vec256_t r;
    r.lane[0] = v.lane[(imm >> 0) & 3];
    r.lane[1] = v.lane[(imm >> 2) & 3];
    r.lane[2] = v.lane[(imm >> 4) & 3];
    r.lane[3] = v.lane[(imm >> 6) & 3];
    return r;
}

/**
 * Multiply each lane of a vector by a constant
 * @param v Input vector
 * @param c Constant multiplier
 * @return Vector with each lane multiplied by c
 */
static inline vec256_t vec256_mul_const(vec256_t v, uint64_t c) {
    vec256_t r;
    for (int i = 0; i < 4; i++) r.lane[i] = v.lane[i] * c;
    return r;
}

/* ---------------- MIX LANES ---------------- */

/**
 * Mix lanes within a vector to provide diffusion
 * Performs permutations and XORs to mix data between lanes
 * 
 * @param v Input vector
 * @return Mixed vector with cross-lane diffusion
 */
static inline vec256_t mix_lanes_vector(vec256_t v) {
    /* Permute lanes: (1,0,3,2) - swap adjacent lane pairs */
    vec256_t p0 = vec256_permute(v, 0x4E);  // (1,0,3,2)

    /* Further permute: (2,3,0,1) - swap the pairs */
    vec256_t p1 = vec256_permute(p0, 0xB1); // (2,3,0,1)
    
    /* XOR the two permuted versions */
    vec256_t x = vec256_xor(p0, p1);

    /* Rotate left by 17 bits and XOR with original */
    vec256_t rotated = vec256_rotl(x, 17);

    return vec256_xor(x, rotated);
}

/* ---------------- ARX MIX VECTOR ---------------- */

/**
 * ARX (Add-Rotate-XOR) mixing function for vectors
 * Core mixing operation combining addition, rotation, XOR, and lane mixing
 * 
 * @param v Input vector to mix
 * @param salt Salt vector for additional entropy
 * @param rc Round constant vector
 * @param r1 First rotation amount
 * @param r2 Second rotation amount
 * @return Mixed vector
 */
static inline vec256_t arx_mix_vector(vec256_t v, vec256_t salt, vec256_t rc, int r1, int r2) {
    /* Add salt for entropy injection */
    v = vec256_add(v, salt);
    
    /* XOR with round constant */
    v = vec256_xor(v, rc);
    
    /* Add rotated version of self (left rotation) */
    v = vec256_add(v, vec256_rotl(v, r1));
    
    /* XOR with rotated version of self (right rotation) */
    v = vec256_xor(v, vec256_rotr(v, r2));
    
    /* Mix lanes for cross-lane diffusion */
    v = mix_lanes_vector(v);
    
    /* Multiply by carefully chosen constant for avalanche effect
     * 0x800000000000808AULL is selected for its cryptographic properties
     */
    return vec256_mul_const(v, 0x800000000000808AULL);
}

/* ---------------- HORIZONTAL XOR ---------------- */

/**
 * Reduce a 256-bit vector to a single 64-bit value
 * Combines all lanes through XOR, permutations, and final mixing
 * 
 * @param v Input vector
 * @return 64-bit hash value derived from all lanes
 */
static inline uint64_t horizontal_xor_vector(vec256_t v) {
    /* Start with lane mixing */
    v = mix_lanes_vector(v);

    /* XOR with permuted version (swap adjacent pairs) */
    v = vec256_xor(v, vec256_permute(v, 0x4E));

    /* XOR with another permuted version */
    vec256_t temp = vec256_permute(v, 0x4E);
    v = vec256_xor(v, temp);

    /* Final permutation and XOR */
    temp = vec256_permute(v, 0xB1);
    v = vec256_xor(v, temp);

    /* XOR all lanes together to get initial 64-bit result */
    uint64_t result = v.lane[0] ^ v.lane[1] ^ v.lane[2] ^ v.lane[3];

    /* Final diffusion sequence:
     * - Shift and XOR for bit mixing
     * - Multiplication by carefully chosen constants
     * - More shifts and XORs
     * - Rotations for additional diffusion
     * - Final multiplication and shift
     */
    result ^= result >> 31;
    result *= 0x0000000000000088ULL;
    result ^= result >> 29;
    result *= 0x8000000000008089ULL;
    result ^= result >> 32;
    result = rotr64_scalar(result, 17) ^ rotl64_scalar(result, 43);
    result *= 0x8000000080008081ULL;
    result ^= result >> 27;

    return result;
}

/* ---------------- EXECUTION ---------------- */

/**
 * Main execution function for scalar implementation
 * Processes blocks in parallel using OpenMP, simulating 256-bit vector operations
 * on groups of 4 blocks at a time
 * 
 * @param input Array of input blocks (each block is 10 64-bit words)
 * @param salt_scalar Salt value for this processing round
 * @param round_base Base round number for constant selection
 * @param num_blocks Total number of blocks to process
 */
static inline void little_box_execute_scalar(uint64_t *input,
                                             uint64_t salt_scalar,
                                             uint64_t round_base,
                                             size_t num_blocks) {

    /* Initialize OpenMP parallel processing if available */
    #ifdef _OPENMP
    int n_threads = omp_get_num_procs();
    omp_set_num_threads(n_threads);
    #endif

    /* OpenMP parallel for loop - processes blocks in groups of 4
     * schedule(static) ensures balanced distribution among threads
     */
    #pragma omp for schedule(static)
    for(size_t blk = 0; blk < num_blocks; blk += 4) {
        /* Pointers to up to 4 blocks (handles edge cases with fewer blocks) */
        uint64_t *in[4] = {NULL, NULL, NULL, NULL};
        for(int i = 0; i < 4; i++) {
            if(blk + i < num_blocks) in[i] = &input[(blk + i) * 10];
        }

        /* Create vector of salt values (all lanes same) */
        vec256_t salt = vec256_set1(salt_scalar);

        /* Load v0 (words 1) from up to 4 blocks */
        vec256_t v0  = vec256_set(
            in[0] ? in[0][1] : 0,
            in[1] ? in[1][1] : 0,
            in[2] ? in[2][1] : 0,
            in[3] ? in[3][1] : 0
        );

        /* Load v0l (words 0) from up to 4 blocks */
        vec256_t v0l = vec256_set(
            in[0] ? in[0][0] : 0,
            in[1] ? in[1][0] : 0,
            in[2] ? in[2][0] : 0,
            in[3] ? in[3][0] : 0
        );

        /* Load v1 (words 5) from up to 4 blocks */
        vec256_t v1  = vec256_set(
            in[0] ? in[0][5] : 0,
            in[1] ? in[1][5] : 0,
            in[2] ? in[2][5] : 0,
            in[3] ? in[3][5] : 0
        );

        /* Load v1l (words 4) from up to 4 blocks */
        vec256_t v1l = vec256_set(
            in[0] ? in[0][4] : 0,
            in[1] ? in[1][4] : 0,
            in[2] ? in[2][4] : 0,
            in[3] ? in[3][4] : 0
        );

        /* Load v2 (words 9) from up to 4 blocks */
        vec256_t v2  = vec256_set(
            in[0] ? in[0][9] : 0,
            in[1] ? in[1][9] : 0,
            in[2] ? in[2][9] : 0,
            in[3] ? in[3][9] : 0
        );

        /* Load v2l (words 8) from up to 4 blocks */
        vec256_t v2l = vec256_set(
            in[0] ? in[0][8] : 0,
            in[1] ? in[1][8] : 0,
            in[2] ? in[2][8] : 0,
            in[3] ? in[3][8] : 0
        );

        /* Load round constants for first set of rounds (rc0: rounds 0-3) */
        vec256_t rc0 = vec256_set(
            ROUND_CONSTANTS[(round_base + 0) & (ROUND_CONSTANTS_SIZE-1)],
            ROUND_CONSTANTS[(round_base + 1) & (ROUND_CONSTANTS_SIZE-1)],
            ROUND_CONSTANTS[(round_base + 2) & (ROUND_CONSTANTS_SIZE-1)],
            ROUND_CONSTANTS[(round_base + 3) & (ROUND_CONSTANTS_SIZE-1)]
        );

        /* Load round constants for second set of rounds (rc1: rounds 4-7) */
        vec256_t rc1 = vec256_set(
            ROUND_CONSTANTS[(round_base + 4) & (ROUND_CONSTANTS_SIZE-1)],
            ROUND_CONSTANTS[(round_base + 5) & (ROUND_CONSTANTS_SIZE-1)],
            ROUND_CONSTANTS[(round_base + 6) & (ROUND_CONSTANTS_SIZE-1)],
            ROUND_CONSTANTS[(round_base + 7) & (ROUND_CONSTANTS_SIZE-1)]
        );

        /* Load round constants for third set of rounds (rc2: rounds 8-11) */
        vec256_t rc2 = vec256_set(
            ROUND_CONSTANTS[(round_base + 8) & (ROUND_CONSTANTS_SIZE-1)],
            ROUND_CONSTANTS[(round_base + 9) & (ROUND_CONSTANTS_SIZE-1)],
            ROUND_CONSTANTS[(round_base + 10) & (ROUND_CONSTANTS_SIZE-1)],
            ROUND_CONSTANTS[(round_base + 11) & (ROUND_CONSTANTS_SIZE-1)]
        );

        /* Apply ARX mixing to all vectors */
        v0  = arx_mix_vector(v0,  salt, rc0, 7, 13);
        v0l = arx_mix_vector(v0l, salt, rc0, 7, 13);
        v1  = arx_mix_vector(v1,  salt, rc1, 11, 17);
        v1l = arx_mix_vector(v1l, salt, rc1, 11, 17);
        v2  = arx_mix_vector(v2,  salt, rc2, 19, 23);
        v2l = arx_mix_vector(v2l, salt, rc2, 19, 23);

        /* Mix lanes for all vectors */
        v0  = mix_lanes_vector(v0);
        v0l = mix_lanes_vector(v0l);
        v1  = mix_lanes_vector(v1);
        v1l = mix_lanes_vector(v1l);
        v2  = mix_lanes_vector(v2);
        v2l = mix_lanes_vector(v2l);

        /* Store results back to block 0 */
        if(in[0]) {
            /* Combine contributions from v0, v1, v2 using permutation 0x00 (all lane 0) */
            vec256_t acc0 = vec256_xor(
                vec256_xor(
                    vec256_permute(v0, 0x00),
                    vec256_permute(v1, 0x00)
                ),
                vec256_permute(v2, 0x00)
            );

            in[0][0] = v0.lane[0];
            in[0][1] = v0.lane[1];
            in[0][4] = v1.lane[0];
            in[0][5] = v1.lane[1];
            in[0][8] = v2.lane[0];
            in[0][9] = horizontal_xor_vector(acc0);
        }

        /* Store results back to block 1 */
        if(in[1]) {
            /* Combine contributions from v0, v1, v2 using permutation 0x55 (all lane 1) */
            vec256_t acc1 = vec256_xor(
                vec256_xor(
                    vec256_permute(v0, 0x55),
                    vec256_permute(v1, 0x55)
                ),
                vec256_permute(v2, 0x55)
            );

            in[1][0] = v0.lane[2];
            in[1][1] = v0.lane[3];
            in[1][4] = v1.lane[2];
            in[1][5] = v1.lane[3];
            in[1][8] = v2.lane[2];
            in[1][9] = horizontal_xor_vector(acc1);
        }

        /* Store results back to block 2 (using v0l/v1l/v2l) */
        if(in[2]) {
            /* Combine using permutation 0xAA (all lane 2) */
            vec256_t acc2 = vec256_xor(
                vec256_xor(
                    vec256_permute(v0l, 0xAA),
                    vec256_permute(v1l, 0xAA)
                ),
                vec256_permute(v2l, 0xAA)
            );

            in[2][0] = v0l.lane[0];
            in[2][1] = v0l.lane[1];
            in[2][4] = v1l.lane[0];
            in[2][5] = v1l.lane[1];
            in[2][8] = v2l.lane[0];
            in[2][9] = horizontal_xor_vector(acc2);
        }

        /* Store results back to block 3 (using v0l/v1l/v2l) */
        if(in[3]) {
            /* Combine using permutation 0xFF (all lane 3) */
            vec256_t acc3 = vec256_xor(
                vec256_xor(
                    vec256_permute(v0l, 0xFF),
                    vec256_permute(v1l, 0xFF)
                ),
                vec256_permute(v2l, 0xFF)
            );

            in[3][0] = v0l.lane[2];
            in[3][1] = v0l.lane[3];
            in[3][4] = v1l.lane[2];
            in[3][5] = v1l.lane[3];
            in[3][8] = v2l.lane[2];
            in[3][9] = horizontal_xor_vector(acc3);
        }

        /* Cross-block mixing if we processed a full group of 4 blocks */
        if(blk + 3 < num_blocks) {
            uint64_t *b0 = &input[(blk + 0) * 10];
            uint64_t *b1 = &input[(blk + 1) * 10];
            uint64_t *b2 = &input[(blk + 2) * 10];
            uint64_t *b3 = &input[(blk + 3) * 10];

            /* XOR all final words together */
            uint64_t mix = b0[9] ^ b1[9] ^ b2[9] ^ b3[9];

            /* Apply diffusion to the mixed value */
            mix = rotr64_scalar(mix, 17) ^ rotl64_scalar(mix, 43);
            mix *= 0x9E3779B97F4A7C15ULL;  /* Golden ratio constant */

            /* Feed back into each block with variations */
            b0[9] ^= mix;
            b1[9] ^= rotr64_scalar(mix, 11);
            b2[9] ^= rotl64_scalar(mix, 23);
            b3[9] ^= mix ^ (mix >> 31);
        }
    }
}

/* Clean up macro to prevent namespace pollution */
#undef RC

#endif /* XZALGOCHAIN_ALGORITHM_SCALAR_H */
