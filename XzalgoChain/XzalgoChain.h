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

#ifndef XZALGOCHAIN_H
#define XZALGOCHAIN_H

/* Include all necessary headers for configuration, utilities, platform detection,
 * SIMD detection, and algorithm implementations (scalar and SIMD versions)
 */
#include "config.h"
#include "utils.h"
#include "platform_detect.h"
#include "simd_detect.h"
#include "algorithm.h"
#include "algorithm_scalar.h"
#include "algorithm_simd.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations for SIMD-specific execution functions
 * These are conditionally defined based on architecture support
 */
#if defined(__AVX2__) && (defined(__x86_64__) || defined(__i386__))
static inline void little_box_execute_simd4(uint64_t *input, uint64_t salt_simd, uint64_t round_base, size_t num_blocks);
#endif

#if defined(__ARM_NEON) && (defined(__arm__) || defined(__aarch64__))
static inline void little_box_execute_neon4(uint64_t *input, uint64_t salt_simd, uint64_t round_base, size_t num_blocks);
#endif

/* ==================== FORWARD DECLARATIONS ==================== */

/* Forward declarations for scalar mode control functions
 * These are defined later with atomic or non-atomic implementations
 * depending on compiler support for atomics
 */
static inline void xzalgochain_force_scalar(int force);
static inline int xzalgochain_is_forced_scalar(void);
static inline void xzalgochain_auto_detect(void);

/* ==================== STATE STRUCTURE ==================== */

/**
 * Main context structure for XzalgoChain hash computation
 * Holds all internal state during multi-part hashing
 */
typedef struct {
    uint64_t h[5];                                                          /* Current hash state (5 x 64-bit = 320 bits) */
    uint64_t little_box_state[LITTLE_BOX_COUNT][LITTLE_BOX_PROCESSES];      /* State of each LITTLE box */
    uint64_t big_box_state[BIG_BOX_COUNT][5];                               /* State of each BIG box (5 words per box) */
    uint8_t buffer[128];                                                    /* Input buffer for partial blocks (128 bytes) */
    size_t buffer_len;                                                      /* Number of bytes currently in buffer */
    uint64_t total_bits;                                                    /* Total bits processed (for padding) */
    uint8_t simd_type;                                                      /* Detected SIMD type for this context */
} XzalgoChain_CTX;

/* ==================== BLOCK TRANSFORMATION ==================== */

/**
 * Process a single 1024-bit (16 x 64-bit) block
 * Core compression function that updates the hash state
 * 
 * @param h Current hash state (5 words)
 * @param block Input block data (16 words)
 */
static inline void process_block(uint64_t h[5], const uint64_t block[16]) {
    for (int i = 0; i < 5; i++) {
        uint64_t a = h[i], b = block[i], c = block[i+5], d = block[i+10];

        /* ARX (Add-Rotate-XOR) operations with constants derived from SHA-2 initial values */
        a += b ^ 0x6A09E667BB67AE85ULL; a = rotl64(a,13);
        a ^= c + 0x3C6EF372A54FF53AULL; a = rotl64(a,29);
        a += d ^ 0x510E527F9B05688CULL; a = rotl64(a,37);

        /* Mix with neighboring hash words */
        a ^= h[(i+1)%5]; a += h[(i+4)%5]; a = rotl64(a,17);

        /* Additional diffusion and multiplication by carefully chosen constant */
        a ^= a >> 32; a ^= a << 21; a *= 0x1F83D9AB5BE0CD19ULL;
        a ^= a >> 29; a ^= a << 17;

        h[i] = a;
    }
}

/* ========================== EXECUTOR ============================ */

/**
 * Adapter function to call SIMD execution with single block
 * Wraps little_box_execute_simd to match scalar adapter signature
 */
static inline void little_box_execute_simd_adapter(uint64_t input[10],
                                                       uint64_t salt_simd,
                                                       uint64_t round_base) {
    little_box_execute_simd(input, salt_simd, round_base, 1);
}

/**
 * Adapter function to call scalar execution with single block
 * Wraps little_box_execute_scalar for consistent interface
 */
static inline void little_box_execute_scalar_adapter(uint64_t input[10],
                                                     uint64_t salt_scalar,
                                                     uint64_t round_base) {
    little_box_execute_scalar(input, salt_scalar, round_base, 1);
}

/* ==================== RANDOM SALT GENERATION ==================== */

/**
 * Generate salt values from current hash state
 * Uses a combination of constants, rotations, and mixing to produce
 * unique salt values for each round
 * 
 * @param input Current hash state (5 words)
 * @param salt Output salt array (5 words)
 */
static inline void generate_salt(const uint64_t input[5], uint64_t salt[5]) {
    /* Initialize with well-known constants from SHA-2, SHA-3, and other sources */
    uint64_t s[32] = {
        0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
        0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
        0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
        0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL,
        0xcbbb9d5dc1059ed8ULL, 0x629a292a367cd507ULL,
        0x9159015a3070dd17ULL, 0x152fecd8f70e5939ULL,
        0x67332667ffc00b31ULL, 0x8eb44a8768581511ULL,
        0xdb0c2e0d64f98fa7ULL, 0x47b5481dbefa4fa4ULL,
        0x243f6a8885a308d3ULL, 0x13198a2e03707344ULL,
        0xa4093822299f31d0ULL, 0x082efa98ec4e6c89ULL,
        0x452821e638d01377ULL, 0xbe5466cf34e90c6cULL,
        0xc0ac29b7c97c50ddULL, 0x3f84d5b5b5470917ULL,
        0x8367E295D4C1B8A3ULL, 0xF4E6D2C5B1A79860ULL,
        0x2B5D7C9F8E4A3617ULL, 0xC8D4E2F6B9A31750ULL,
        0x7E3F9A2C5D8B6419ULL, 0xA6D2F8C4E1B79530ULL,
        0x4B7F9E2D5C8A6318ULL, 0xD5F2E7C4B9A16830ULL
    };
    uint64_t counter = 0;

    /* Mix input into salt array */
    for (int i = 0; i < 5; i++) s[i] ^= input[i];

    /* Multiple rounds of mixing with rotations and counter */
    for (int round=0; round<7; round++) {
        for (int j=0;j<32;j++) {
            s[j] ^= rotl64(s[j], (j*7+round*3)%64) ^ rotr64(s[(j+3)&7], (j*5+round*2)%64);
            s[j] += counter;
        }
        counter += 0x7C5F8E4D3B2A6917ULL;  /* Increment counter with constant */
    }

    /* Final reduction to 5 salt words with additional mixing */
    for (int i = 0; i < 5; i++) {
        uint64_t v = s[i] ^ s[(i+3)&7];
        v ^= v >> 31; v *= 0x3A8F7E6D5C4B2918ULL;
        v ^= v >> 29; v *= 0x276D9C5F8E3B41A2ULL;
        salt[i] = v;
    }
}

/* ==================== ENHANCED FINAL MIXING ==================== */

/**
 * Additional mixing to improve distribution
 */
static inline uint64_t extra_mix(uint64_t x) {
    x ^= x >> 27;
    x *= 0x9E3779B97F4A7C15ULL;
    x ^= x >> 31;
    x *= 0xBF58476D1CE4E5B9ULL;
    x ^= x >> 29;
    x += rotl64(x, 41);
    return x;
}

/* ==================== LITTLE BOX COMPLETION CHECK ==================== */

/**
 * Check if a LITTLE box has completed all its processes
 * All process outputs should be non-zero
 * 
 * @param lb LITTLE box state array
 * @return true if all processes are complete, false otherwise
 */
static inline bool little_box_complete(uint64_t lb[LITTLE_BOX_PROCESSES]) {
    for (int i = 0; i < LITTLE_BOX_PROCESSES; i++) if (lb[i] == 0) return false;
    return true;
}

/* ==================== BIG BOX EXECUTION ==================== */

/**
 * Execute a BIG box transformation
 * Processes all LITTLE boxes within a BIG box and updates state
 * 
 * @param ctx Hash context
 * @param box_index Index of the BIG box to execute
 * @param round_base Base round number for constant selection
 */
static inline void big_box_execute(XzalgoChain_CTX *ctx, int box_index, uint64_t round_base) {
    /* Select appropriate executor based on SIMD type */
    void (*executor)(uint64_t[10], uint64_t, uint64_t);

    if (ctx->simd_type == SIMD_AVX2) {
        #if defined(__AVX2__) && (defined(__x86_64__) || defined(__i386__))
        executor = little_box_execute_simd_adapter;
        #else
        executor = little_box_execute_scalar_adapter;
        #endif
    } else if (ctx->simd_type == SIMD_NEON) {
        #if defined(__ARM_NEON) && (defined(__arm__) || defined(__aarch64__))
        executor = little_box_execute_simd_adapter;
        #else
        executor = little_box_execute_scalar_adapter;
        #endif
    } else {
        executor = little_box_execute_scalar_adapter;
    }

    /* Generate salt from current hash state */
    uint64_t salt[5] = {0};
    generate_salt((uint64_t*)ctx->h, salt);

    /* Process each LITTLE box */
    for (int lb=0; lb<LITTLE_BOX_COUNT; lb++) {
        uint64_t little_input[10];
        
        /* Prepare input for LITTLE box: mix hash with salt and round constants */
        for (int i=0;i<5;i++) {
            little_input[i] = ctx->h[i] ^ salt[i];
            little_input[i+5] = ctx->h[i] ^ ROUND_CONSTANTS[(lb*10+i)&(ROUND_CONSTANTS_SIZE-1)];
        }
        
        /* Create salt variation for this LITTLE box */
        uint64_t salt_variation = salt[lb%5] ^ ROUND_CONSTANTS[(lb * 10) & (ROUND_CONSTANTS_SIZE-1)];
        
        /* Execute LITTLE box processing */
        executor(little_input, salt_variation, round_base + lb*10);
        
        /* Update salt variation for next box */
        salt_variation = rotr64(salt_variation, 13) ^ rotl64(little_input[9], 23);
        
        /* Store LITTLE box state */
        for (int i=0;i<LITTLE_BOX_PROCESSES;i++) ctx->little_box_state[lb][i] = little_input[i];
    }

    /* Aggregate LITTLE box results into BIG box state */
    for (int i=0;i<5;i++) {
        ctx->big_box_state[box_index][i]=0;
        for (int lb=0; lb<LITTLE_BOX_COUNT; lb++) {
            ctx->big_box_state[box_index][i] ^= ctx->little_box_state[lb][i*2];
            ctx->big_box_state[box_index][i] += ctx->little_box_state[lb][i*2+1];
        }
        
        /* Apply gamma mixing to final BIG box state */
        ctx->big_box_state[box_index][i] = gamma_mix(
            ctx->big_box_state[box_index][i],
            salt[i],
            ROUND_CONSTANTS[(box_index*100+i)&(ROUND_CONSTANTS_SIZE-1)],
            round_base+1000);
    }
}

/* ==================== INITIALIZATION ==================== */

/**
 * Initialize a new hash context
 * Sets initial hash values, clears state, and detects SIMD capabilities
 * 
 * @param ctx Context to initialize
 */
static inline void xzalgochain_init(XzalgoChain_CTX *ctx) {
    if (!ctx) return;

    /* Detect SIMD type unless scalar mode is forced */
    if (!xzalgochain_is_forced_scalar()) {
        ctx->simd_type = xzalgochain_get_simd_type();
    } else {
        ctx->simd_type = SIMD_NONE;
    }

    /* Initialize hash with non-zero constants (fractional parts of sqrt of primes) */
    ctx->h[0] = 0xBB67AE854A7D9E31ULL;
    ctx->h[1] = 0x5BE0CD19B7F3A69CULL;
    ctx->h[2] = 0x6A09E667F2B5C8D3ULL;
    ctx->h[3] = 0x3C6EF372D8B4F1A6ULL;
    ctx->h[4] = 0x510E527F4D8C3A92ULL;

    /* Initialize dengan nilai acak tambahan */
    ctx->h[0] ^= 0x9E3779B97F4A7C15ULL;  /* Golden ratio */
    ctx->h[1] ^= 0xBF58476D1CE4E5B9ULL;
    ctx->h[2] ^= 0x94D049BB133111EBULL;

    /* Mix the initial values for pattern elimination */
    for (int i = 0; i < 5; i++) {
        ctx->h[i] ^= ROUND_CONSTANTS[i * 10];
        ctx->h[i] = rotl64(ctx->h[i], 17 + i*7);
        ctx->h[i] *= 0x9E3779B97F4A7C15ULL;
        ctx->h[i] ^= ctx->h[(i+2)%5];
    }
    
    /* Clear all state arrays and buffer */
    memset(ctx->little_box_state,0,sizeof(ctx->little_box_state));
    memset(ctx->big_box_state,0,sizeof(ctx->big_box_state));
    memset(ctx->buffer,0,sizeof(ctx->buffer));
    ctx->buffer_len=0; 
    ctx->total_bits=0;
}

/* ==================== UPDATE ==================== */

/**
 * Update hash context with additional data
 * Processes data in 128-byte blocks, buffering partial blocks
 * 
 * @param ctx Hash context
 * @param data Input data bytes
 * @param len Length of input data
 */
static inline void xzalgochain_update(XzalgoChain_CTX *ctx, const uint8_t *data, size_t len) {
    if (!ctx||!data||len==0) return;
    
    /* Update total bits processed */
    ctx->total_bits += len*8;
    size_t offset=0;

    /* Handle existing data in buffer */
    if (ctx->buffer_len>0) {
        size_t copy_len = len < (128-ctx->buffer_len) ? len : (128-ctx->buffer_len);
        memcpy(ctx->buffer+ctx->buffer_len, data, copy_len);
        ctx->buffer_len += copy_len; 
        offset += copy_len;
        
        /* If buffer is full, process as block */
        if (ctx->buffer_len==128) {
            uint64_t block[16]; 
            for(int i=0;i<16;i++) block[i]=bytes_to_u64(ctx->buffer+i*8);
            process_block(ctx->h, block);
            ctx->buffer_len=0;
        }
    }

    /* Process complete blocks directly from input */
    while (offset+128<=len) {
        uint64_t block[16]; 
        for(int i=0;i<16;i++) block[i]=bytes_to_u64(data+offset+i*8);
        process_block(ctx->h, block); 
        offset+=128;
    }

    /* Store remaining data in buffer */
    if (offset<len) { 
        memcpy(ctx->buffer, data+offset, len-offset); 
        ctx->buffer_len=len-offset; 
    }
}

/* ==================== FINAL ==================== */

/**
 * Finalize hash computation and produce output
 * Applies padding, processes remaining data, and performs final mixing
 * 
 * @param ctx Hash context
 * @param output Output buffer (must be at least XZALGOCHAIN_HASH_SIZE bytes)
 */
static inline void xzalgochain_final(XzalgoChain_CTX *ctx, uint8_t output[XZALGOCHAIN_HASH_SIZE]) {
    if (!ctx||!output) return;

    /* Apply padding: add 0x80 byte followed by zeros */
    ctx->buffer[ctx->buffer_len]=0x80; 
    ctx->buffer_len++;
    memset(ctx->buffer+ctx->buffer_len,0,128-ctx->buffer_len);
    
    /* Process final block */
    uint64_t block[16]; 
    for(int i=0;i<16;i++) block[i]=bytes_to_u64(ctx->buffer+i*8);
    process_block(ctx->h, block);

    /* Generate salt and execute BIG boxes */
    uint64_t salt[5]; 
    generate_salt((uint64_t*)ctx->h, salt);

    for (int bb=0; bb<BIG_BOX_COUNT; bb++) 
        big_box_execute(ctx, bb, bb*2000);

    /* Final mixing of hash state */
    const uint8_t rot_params[5] = {31, 27, 33, 23, 29};

    for (int i = 0; i < 5; i++) {
        uint64_t x = ctx->h[i];
        x ^= rotr64(x, rot_params[i]);
        x *= 0x510E9BB7927522F5ULL;
        x += 0x243F6A8885A308D3ULL;
        x ^= rotr64(x, rot_params[(i + 1) % 5]);
        x *= 0xA0761D647ABD642FULL;
        x ^= (x >> 23);
        x ^= (x >> 38);
        ctx->h[i] = x;
    }

    /* Combine BIG box states with final mixing */
    uint64_t final_mix[5];
    for (int i = 0; i < 5; i++) {
        uint64_t acc = ctx->h[i];
        for (int bb = 0; bb < BIG_BOX_COUNT; bb++) {
            acc ^= ctx->big_box_state[bb][i];
            acc = rotr64(acc, 19) ^ rotl64(acc, 37);
            acc += ctx->big_box_state[bb][(i+2)%5];
            acc *= 0x9E3779B97F4A7C15ULL;   /* Golden ratio constant */
        }
        acc ^= acc >> 29;
        acc *= 0xBF58476D1CE4E5B9ULL;       /* Constants from splitmix64 */
        acc ^= acc >> 27;
        acc *= 0x94D049BB133111EBULL;       /* More splitmix64 constants */
        acc ^= acc >> 31;
        final_mix[i] = acc;
    }
    memcpy(ctx->h, final_mix, sizeof(final_mix));

    /* Additional final mixing rounds */
    for (int round = 0; round < 3; round++) {
        for (int i = 0; i < 5; i++) {
            ctx->h[i] = extra_mix(ctx->h[i]);
            ctx->h[i] ^= ctx->big_box_state[round % BIG_BOX_COUNT][i];
            ctx->h[i] = rotl64(ctx->h[i], 17 + round*5);
        }
    }

    /* Multiple final mixes to even out the distribution */
    for (int round = 0; round < 5; round++) {
        /* Mix all state */
        uint64_t mix = 0;
        for (int i = 0; i < 5; i++) {
            mix ^= ctx->h[i];
            mix = rotl64(mix, 17) ^ ctx->h[(i+2)%5];
        }

        /* Feedback to all hash */
        for (int i = 0; i < 5; i++) {
            ctx->h[i] ^= rotl64(mix, i * 13);
            ctx->h[i] *= 0x9E3779B97F4A7C15ULL;
            ctx->h[i] ^= ctx->h[(i+1)%5] >> (i*7 + 3);
            ctx->h[i] = rotr64(ctx->h[i], 23 + i*5);
        }
    }

    /* Convert final hash state to bytes */
    for (int i=0;i<5;i++)
        u64_to_bytes(ctx->h[i], output+i*8);
}

/* ==================== SINGLE-SHOT HASH ==================== */

/**
 * Compute hash of a complete message in one call
 * Convenience function for simple use cases
 * 
 * @param data Input data bytes
 * @param len Length of input data
 * @param output Output buffer (must be at least XZALGOCHAIN_HASH_SIZE bytes)
 */
static inline void xzalgochain(const uint8_t *data, size_t len, uint8_t output[XZALGOCHAIN_HASH_SIZE]) {
    XzalgoChain_CTX ctx;
    xzalgochain_init(&ctx);
    xzalgochain_update(&ctx, data, len);
    xzalgochain_final(&ctx, output);

    /* Additional mixing on the output for dependency elimination */
    uint64_t *out = (uint64_t*)output;
    for (int mix = 0; mix < 3; mix++) {
        uint64_t acc = 0;
        for (int i = 0; i < 5; i++) {
            acc ^= out[i];
            out[i] = rotr64(out[i], 19) ^ rotl64(acc, 37);
            out[i] *= 0xBF58476D1CE4E5B9ULL;
            out[i] ^= out[(i+2)%5] >> 27;
        }
    }
    /* Convert back to bytes */
    for (int i = 0; i < 5; i++)
        u64_to_bytes(out[i], output + i*8);

    /* One more mixing pass on output */
    uint64_t *out64 = (uint64_t*)output;
    for (int i = 0; i < 5; i++) {
        out64[i] = extra_mix(out64[i]);
        out64[i] ^= out64[(i+2)%5];
    }
    /* Convert back to bytes */
    for (int i = 0; i < 5; i++)
        u64_to_bytes(out64[i], output + i*8);

    memset(&ctx,0,sizeof(ctx));  /* Wipe context for security */
}

/* ==================== CONTEXT MANAGEMENT ==================== */

/**
 * Reset context to initial state (same as re-initializing)
 */
static inline void xzalgochain_ctx_reset(XzalgoChain_CTX *ctx){ 
    if(ctx) xzalgochain_init(ctx); 
}

/**
 * Securely wipe context to clear sensitive data
 */
static inline void xzalgochain_ctx_wipe(XzalgoChain_CTX *ctx){ 
    if(ctx) memset(ctx,0,sizeof(XzalgoChain_CTX)); 
}

/* ==================== INFO ==================== */

/**
 * Get version string
 */
static inline const char* xzalgochain_version(void){ 
    return "XzalgoChain 0.0.1 - 320-bit";
}

/**
 * Get platform information string
 */
static inline const char* xzalgochain_platform_info(void){ 
    return xzalgochain_get_platform_name(); 
}

/* ==================== FORCE SCALAR MODE ==================== */

/**
 * Global flag to force scalar mode (disable SIMD)
 * Uses atomic operations if available for thread safety
 */

/* Fallback for compilers without atomics */
#ifdef __STDC_NO_ATOMICS__
static int _xz_force_scalar = 0;

static inline void xzalgochain_force_scalar(int force) {
    _xz_force_scalar = force ? 1 : 0;
}

static inline int xzalgochain_is_forced_scalar(void) {
    return _xz_force_scalar;
}

/* Use C11 atomics for thread-safe access */
#else
#include <stdatomic.h>
static atomic_int _xz_force_scalar = 0;

static inline void xzalgochain_force_scalar(int force) {
    atomic_store(&_xz_force_scalar, force ? 1 : 0);
}

static inline int xzalgochain_is_forced_scalar(void) {
    return atomic_load(&_xz_force_scalar);
}
#endif

#ifdef __cplusplus
}
#endif

#endif /* XZALGOCHAIN_H */
