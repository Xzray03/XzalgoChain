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

#ifndef XZALGOCHAIN_ALGORITHM_H
#define XZALGOCHAIN_ALGORITHM_H

/* Include configuration and utility headers for platform detection,
 * compiler-specific attributes, and common helper functions
 */
#include "config.h"
#include "utils.h"

/* ==================== CRYPTOGRAPHIC PRIMITIVES ==================== */

/**
 * Gamma mixing function - core non-linear transformation
 * Combines three 64-bit inputs with round constant using:
 * - XOR operations for diffusion
 * - Rotations for bit position mixing
 * - AND operations for non-linearity
 * - Multiplication by constants for avalanche effect
 * 
 * @param x First input word
 * @param y Second input word
 * @param z Third input word
 * @param round Round constant/input
 * @return Mixed 64-bit output
 */
static inline uint64_t gamma_mix(uint64_t x, uint64_t y, uint64_t z, uint64_t round) {
    /* Basic XOR mixing of inputs */
    uint64_t r = x ^ y ^ z;
    
    /* Add rotated versions with different rotation amounts for diffusion */
    r += rotl64(x, 13) ^ rotr64(y, 7) ^ rotl64(z, 29);
    
    /* Non-linear mixing using majority function: (x & y) | (z & ~x)
     * This creates an asymmetric, non-linear combination
     */
    r ^= (x & y) | (z & ~x);
    
    /* Add round constant for round-dependent behavior */
    r += round;
    
    /* Double rotation to mix bits across the word */
    r = rotr64(r, 17) ^ rotl64(r, 23);
    
    /* Self-mixing: XOR with shifted version of itself */
    r ^= (r << 19) | (r >> 45);
    
    /* Add multiplication by carefully chosen constants
     * 0x8000000080008009ULL and 0x8000000000008081ULL are
     * selected for their cryptographic properties
     */
    r += (x * 0x8000000080008009ULL) ^ (y * 0x8000000000008081ULL);
    
    return r;
}

/**
 * Sigma transformation - variant of the Σ function from SHA-2/3
 * Provides four different transformation modes using combinations
 * of rotations and shifts for different diffusion patterns
 * 
 * @param x Input word to transform
 * @param v Variant selector (0-3) determining which transformation to apply
 * @return Transformed word
 */
static inline uint64_t sigma_transform(uint64_t x, int v) {
    switch (v) {
        case 0:
            /* Sigma0: rotation by 28, 34, and 39 bits
             * Used for higher-order diffusion
             */
            return rotr64(x, 28) ^ rotr64(x, 34) ^ rotr64(x, 39);
            
        case 1:
            /* Sigma1: rotation by 14, 18, and 41 bits
             * Different rotation set for varied diffusion
             */
            return rotr64(x, 14) ^ rotr64(x, 18) ^ rotr64(x, 41);
            
        case 2:
            /* Small sigma0: mixture of rotations and shift
             * Combines 1-bit and 8-bit rotations with right shift
             */
            return rotr64(x, 1)  ^ rotr64(x, 8)  ^ (x >> 7);
            
        case 3:
            /* Small sigma1: rotation by 19 and 61 bits with shift
             * Complement to sigma2 for different diffusion pattern
             */
            return rotr64(x, 19) ^ rotr64(x, 61) ^ (x >> 6);
            
        default:
            /* Fallback for invalid variant (should not occur) */
            return x;
    }
}

/* ==================== LITTLE BOX PROCESSES ==================== */

/**
 * Macro to access round constants with circular indexing
 * RC(i) returns ROUND_CONSTANTS[i mod ROUND_CONSTANTS_SIZE]
 * Different syntax for C vs C++ to handle language-specific scoping
 */
#if defined(__cplusplus)
#define RC(i) (ROUND_CONSTANTS[(i) & (ROUND_CONSTANTS_SIZE - 1)])
#else
#define RC(i) ROUND_CONSTANTS[(i) & (ROUND_CONSTANTS_SIZE - 1)]
#endif

/**
 * Little Box Process 1
 * Applies gamma mixing with salt and round constant
 * 
 * @param in Input word
 * @param salt Salt value for added entropy
 * @param round Current round number
 * @return Mixed output
 */
static inline uint64_t little_box_process1(uint64_t in, uint64_t salt, uint64_t round) {
    return gamma_mix(in, salt, round, RC(round));
}

/**
 * Little Box Process 2
 * Applies rotation, XOR with self, sigma transform, and round constant
 * 
 * @param x Input word
 * @param round Current round number
 * @return Mixed output
 */
static inline uint64_t little_box_process2(uint64_t x, uint64_t round) {
    /* XOR with rotated self for diffusion */
    x ^= rotr64(x, 19) ^ rotl64(x, 42);
    /* Apply sigma transform variant 0 */
    x += sigma_transform(x, 0);
    /* Add round constant with offset +1 */
    return x ^ RC(round + 1);
}

/**
 * Little Box Process 3
 * Applies rotations, sigma transform, and round constant
 * 
 * @param x Input word
 * @param round Current round number
 * @return Mixed output
 */
static inline uint64_t little_box_process3(uint64_t x, uint64_t round) {
    /* Rotate left and right with different amounts */
    x = rotl64(x, 27) ^ rotr64(x, 31);
    /* Apply sigma transform variant 1 */
    x ^= sigma_transform(x, 1);
    /* Add round constant with offset +2 */
    return x + RC(round + 2);
}

/**
 * Little Box Process 4
 * Applies shift-rotation, sigma transform, and round constant
 * 
 * @param x Input word
 * @param round Current round number
 * @return Mixed output
 */
static inline uint64_t little_box_process4(uint64_t x, uint64_t round) {
    /* Mix using combined shift and rotation */
    x ^= (x << 23) | (x >> 41);
    /* Apply sigma transform variant 2 */
    x += sigma_transform(x, 2);
    /* Add round constant with offset +3 */
    return x ^ RC(round + 3);
}

/**
 * Little Box Process 5
 * Applies multiplication, rotation, sigma transform, and round constant
 * 
 * @param x Input word
 * @param round Current round number
 * @return Mixed output
 */
static inline uint64_t little_box_process5(uint64_t x, uint64_t round) {
    /* Multiply by maximum 64-bit value (0xFFFFFFFFFFFFFFFF)
     * This is equivalent to computing x * (2^64 - 1)
     */
    x *= 0xFFFFFFFFFFFFFFFFULL;
    /* XOR with rotated self */
    x ^= rotr64(x, 33);
    /* Apply sigma transform variant 3 */
    x += sigma_transform(x, 3);
    /* Add round constant with offset +4 */
    return x ^ RC(round + 4);
}

/**
 * Little Box Process 6
 * Applies rotations, sigma transform, and round constant
 * (Similar to process2 but with different rotation amounts)
 * 
 * @param x Input word
 * @param round Current round number
 * @return Mixed output
 */
static inline uint64_t little_box_process6(uint64_t x, uint64_t round) {
    /* XOR with rotated self (different rotation amounts) */
    x ^= rotl64(x, 37) ^ rotr64(x, 29);
    /* Apply sigma transform variant 0 */
    x += sigma_transform(x, 0);
    /* Add round constant with offset +5 */
    return x ^ RC(round + 5);
}

/**
 * Little Box Process 7
 * Applies shift-rotation, sigma transform, and round constant
 * (Similar to process4 but with different shift amounts)
 * 
 * @param x Input word
 * @param round Current round number
 * @return Mixed output
 */
static inline uint64_t little_box_process7(uint64_t x, uint64_t round) {
    /* Mix using shift right and left with different amounts */
    x ^= (x >> 17) ^ (x << 47);
    /* Apply sigma transform variant 1 */
    x += sigma_transform(x, 1);
    /* Add round constant with offset +6 */
    return x ^ RC(round + 6);
}

/**
 * Little Box Process 8
 * Applies rotations, sigma transform, and round constant
 * (Similar to process6 but with different rotation amounts)
 * 
 * @param x Input word
 * @param round Current round number
 * @return Mixed output
 */
static inline uint64_t little_box_process8(uint64_t x, uint64_t round) {
    /* XOR with rotated self (rotation by 11 and 53) */
    x ^= rotr64(x, 11) ^ rotl64(x, 53);
    /* Apply sigma transform variant 2 */
    x += sigma_transform(x, 2);
    /* Add round constant with offset +7 */
    return x ^ RC(round + 7);
}

/**
 * Little Box Process 9
 * Applies gamma mixing to a single input by using
 * rotated versions of the same word as additional inputs
 * 
 * @param x Input word
 * @param round Current round number
 * @return Mixed output
 */
static inline uint64_t little_box_process9(uint64_t x, uint64_t round) {
    return gamma_mix(
        x,                       /* First input */
        rotr64(x, 31),           /* Rotated right version as second input */
        rotl64(x, 29),           /* Rotated left version as third input */
        RC(round + 8)            /* Round constant with offset +8 */
    );
}

/**
 * Little Box Process 10
 * Multi-word mixing function that combines up to 9 words
 * using XOR, rotation, and gamma mixing
 * 
 * @param d Array of up to 9 64-bit words to mix
 * @param round Current round number
 * @return Combined and mixed output
 */
static inline uint64_t little_box_process10(uint64_t *d, uint64_t round) {
    uint64_t r = 0;

    /* Combine all input words with varying rotation amounts
     * Each word contributes:
     * - XOR for basic combination
     * - Rotated version with word-dependent rotation amount
     * - XOR of another rotated version with different amount
     */
    for (int i = 0; i < 9; ++i) {
        uint64_t v = d[i];
        r ^= v;                               /* Basic XOR accumulation */
        r += rotl64(v, i * 7);                /* Rotate left by multiples of 7 */
        r ^= rotr64(v, i * 13);               /* Rotate right by multiples of 13 */
    }

    /* Apply gamma mixing to the accumulated result
     * Uses rotated versions of r as additional inputs
     */
    r = gamma_mix(r, rotr64(r, 23), rotl64(r, 41), RC(round + 9));
    
    /* Final sigma transform variant 3 for additional diffusion */
    return r ^ sigma_transform(r, 3);
}

/* Clean up the RC macro to prevent namespace pollution */
#undef RC

#endif /* XZALGOCHAIN_ALGORITHM_H */
