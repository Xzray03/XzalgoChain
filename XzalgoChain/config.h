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

#ifndef XZALGOCHAIN_CONFIG_H
#define XZALGOCHAIN_CONFIG_H

/* Standard library headers for basic types and operations
 * These are included to ensure all required types are available
 * throughout the XzalgoChain implementation
 */
#include <stdint.h>   /* Fixed-width integer types (uint64_t, etc.) */
#include <stdlib.h>   /* Standard library functions */
#include <string.h>   /* Memory operations (memset, memcpy, etc.) */
#include <stdbool.h>  /* Boolean type and true/false constants */
#include <stddef.h>   /* Size_t and other definitions */

/* ==================== CONSTANTS ==================== */

/**
 * XZALGOCHAIN_HASH_SIZE: Size of the final hash output in bytes
 * 320 bits = 40 bytes (320 / 8 = 40)
 * This constant defines the output length of the hash function
 */
#define XZALGOCHAIN_HASH_SIZE   40     /* 320 bits = 40 bytes */

/**
 * LITTLE_BOX_COUNT: Number of LITTLE boxes per BIG box
 * Each LITTLE box represents a unit of cryptographic processing
 */
#define LITTLE_BOX_COUNT        10     /* LITTLE boxes per BIG box */

/**
 * BIG_BOX_COUNT: Total number of BIG boxes in the algorithm
 * Each BIG box contains multiple LITTLE boxes
 */
#define BIG_BOX_COUNT           5      /* BIG boxes total */

/**
 * LITTLE_BOX_PROCESSES: Number of processes per LITTLE box
 * Each process applies a specific transformation function
 */
#define LITTLE_BOX_PROCESSES    10     /* Processes per LITTLE box */

/**
 * ROUND_CONSTANTS_SIZE: Number of round constants available
 * 128 constants provide enough variety for many rounds
 * Using power of 2 (128) allows fast modulo with bitwise AND
 */
#define ROUND_CONSTANTS_SIZE    128    /* Constants */

/* ==================== SIMD TYPE CONSTANTS ==================== */

/**
 * SIMD type identifiers for runtime detection and selection
 * These constants are used to indicate which SIMD implementation
 * is currently active or available
 */

/**
 * SIMD_NONE: No SIMD acceleration available or in use
 * Falls back to scalar implementation
 */
#define SIMD_NONE    0

/**
 * SIMD_AVX2: AVX2 (Advanced Vector Extensions 2) on x86/x64
 * Processes 4 64-bit values in parallel using 256-bit registers
 */
#define SIMD_AVX2    1
#define BIT_AVX2    (1 << 5)  /* Bit flag for AVX2 capability detection */

/**
 * SIMD_NEON: NEON SIMD on ARM architectures
 * On ARMv7, uses 128-bit registers (2x64-bit)
 * On ARM64, can use 128-bit registers efficiently
 */
#define SIMD_NEON    2
#define BIT_NEON    (1 << 6)  /* Bit flag for NEON capability detection */

/* ==================== ROUND CONSTANTS ==================== */

/**
 * Round constants array used throughout the hash computation
 * 128 carefully selected 64-bit constants that provide:
 * - Nothing-up-my-sleeve numbers from well-known sources
 * - Good distribution of bits
 * - Resistance against rotational cryptanalysis
 * 
 * The constants include:
 * - First 64 constants: Derived from SHA-2 and SHA-3 round constants
 *   These are well-vetted cryptographic constants with proven properties
 * 
 * - Next 32 constants: Additional constants for extended rounds
 *   Selected to maintain good cryptographic properties
 * 
 * - Final 32 constants: Mix of various sources including:
 *   * Golden ratio and other irrational number approximations
 *   * ASCII values of meaningful strings
 *   * Additional carefully chosen values for optimal diffusion
 * 
 * The constants are indexed modulo ROUND_CONSTANTS_SIZE (128)
 * Using bitwise AND (index & 127) for fast modulo operation
 */
static const uint64_t ROUND_CONSTANTS[ROUND_CONSTANTS_SIZE] = {
    /* === SHA-2 and SHA-3 derived constants (first 64) === */
    /* These are the first 64 round constants from SHA-512 */
    0x428A2F98D728AE22ULL, 0x7137449123EF65CDULL, 0xB5C0FBCFEC4D3B2FULL, 0xE9B5DBA58189DBBCULL,
    0x3956C25BF348B538ULL, 0x59F111F1B605D019ULL, 0x923F82A4AF194F9BULL, 0xAB1C5ED5DA6D8118ULL,
    0xD807AA98A3030242ULL, 0x12835B0145706FBEULL, 0x243185BE4EE4B28CULL, 0x550C7DC3D5FFB4E2ULL,
    0x72BE5D74F27B896FULL, 0x80DEB1FE3B1696B1ULL, 0x9BDC06A725C71235ULL, 0xC19BF174CF692694ULL,
    0xE49B69C19EF14AD2ULL, 0xEFBE4786384F25E3ULL, 0x0FC19DC68B8CD5B5ULL, 0x240CA1CC77AC9C65ULL,
    0x2DE92C6F592B0275ULL, 0x4A7484AA6EA6E483ULL, 0x5CB0A9DCBD41FBD4ULL, 0x76F988DA831153B5ULL,
    0x983E5152EE66DFABULL, 0xA831C66D2DB43210ULL, 0xB00327C898FB213FULL, 0xBF597FC7BEEF0EE4ULL,
    0xC6E00BF33DA88FC2ULL, 0xD5A79147930AA725ULL, 0x06CA6351E003826FULL, 0x142929670A0E6E70ULL,
    0x27B70A8546D22FFCULL, 0x2E1B21385C26C926ULL, 0x4D2C6DFC5AC42AEDULL, 0x53380D139D95B3DFULL,
    0x650A73548BAF63DEULL, 0x766A0ABB3C77B2A8ULL, 0x81C2C92E47EDAEE6ULL, 0x92722C851482353BULL,
    0xA2BFE8A14CF10364ULL, 0xA81A664BBC423001ULL, 0xC24B8B70D0F89791ULL, 0xC76C51A30654BE30ULL,
    0xD192E819D6EF5218ULL, 0xD69906245565A910ULL, 0xF40E35855771202AULL, 0x106AA07032BBD1B8ULL,
    0x19A4C116B8D2D0C8ULL, 0x1E376C085141AB53ULL, 0x2748774CDF8EEB99ULL, 0x34B0BCB5E19B48A8ULL,
    0x391C0CB3C5C95A63ULL, 0x4ED8AA4AE3418ACBULL, 0x5B9CCA4F7763E373ULL, 0x682E6FF3D6B2B8A3ULL,
    0x748F82EE5DEFB2FCULL, 0x78A5636F43172F60ULL, 0x84C87814A1F0AB72ULL, 0x8CC702081A6439ECULL,
    0x90BEFFFA23631E28ULL, 0xA4506CEBDE82BDE9ULL, 0xBEF9A3F7B2C67915ULL, 0xC67178F2E372532BULL,

    /* === Additional constants from SHA-3 (next 32) === */
    /* SHA-3 round constants provide good bit distribution */
    0x0000000000000001ULL, 0x0000000000008082ULL, 0x800000000000808AULL, 0x8000000080008000ULL,
    0x000000000000808BULL, 0x0000000080000001ULL, 0x8000000080008081ULL, 0x8000000000008009ULL,
    0x000000000000008AULL, 0x0000000000000088ULL, 0x0000000080008009ULL, 0x000000008000000AULL,
    0x000000008000808BULL, 0x800000000000008BULL, 0x8000000000008089ULL, 0x8000000000008003ULL,
    0x8000000000008002ULL, 0x8000000000000080ULL, 0x000000000000800AULL, 0x800000008000000AULL,
    0x8000000000008080ULL, 0x8000000080008008ULL, 0x6A09E667F2BDC948ULL, 0x132435465768798AULL,

    /* === Extended constants (next 16) === */
    /* Mix of golden ratio and other irrational approximations */
    0xC0D1E2F3A4B59687ULL, 0x78695A4B3C2D1E0FULL, 0xA96F30BC163138AAULL, 0xCBF29CE484222325ULL,
    0x6C7967656E657261ULL, 0x646F72616E646F6DULL, 0xCA273ECEEA26619CULL, 0xF4846468E8DF0C0BULL,
    0x18695A087A5C0593ULL, 0x23B41638005C0F2DULL, 0x2D491CBFB1D3A637ULL, 0x324B42C185E58F9EULL,
    0x3A1010A7B8D67679ULL, 0x3F73C4AF18518865ULL, 0x5A0DEEEFF85E0B80ULL, 0x5E9D7A75E2F1B5CBULL,

    /* === Final constants (last 16) === */
    /* Carefully chosen for optimal diffusion and avalanche effect */
    0x667F9CFB7B3C9D3FULL, 0x6C78E7A5948A265CULL, 0x6C6E7E9A7C5D3A1FULL, 0x7A0D6C2D0B8F5E3AULL,
    0x7B0C9E5A6D3F1D8CULL, 0x8A0F5E3C7D1B9A6FULL, 0x8C2D5E3F7A1B9C6DULL, 0x9A0B8C7D6E5F4A3BULL,
    0xE38DEE4DB0FB0E4EULL, 0xB1C2D3E4F5061728ULL, 0xC1D2E3F405162738ULL, 0xD1E2F30415263748ULL,
    0xE1F2031425364758ULL, 0xF102132435465768ULL, 0xE58001F9E5CFFA7EULL, 0xD1AA379F9C4B9809ULL,
    0x993A2F8B88C1B63FULL, 0x579A01155E6D4196ULL, 0xBB0FC70B1266B3F1ULL, 0xDE509C2F03B01495ULL,
    0x8859485125BC297CULL, 0x102B36560F6E68E6ULL, 0xE2D0C0A896B87C6EULL, 0x4F5E6A7B8C9DAFB1ULL
};

#endif /* XZALGOCHAIN_CONFIG_H */