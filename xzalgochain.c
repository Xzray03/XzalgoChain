/*
 * XzalgoChain - 320-bit Cryptographic Hash Function
 * Copyright 2026 Xzrayツ
 *
 * xzalgochain.c - Implementation wrapper for XzalgoChain library
 * This file ensures symbols are exported from the compiled library
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

#include "XzalgoChain/XzalgoChain.h"

#ifdef __cplusplus
extern "C" {
    #endif

    /* ==================== CORE HASH FUNCTION ==================== */
    void xzalgochain_lib(const uint8_t *data, size_t len, uint8_t output[XZALGOCHAIN_HASH_SIZE]) {
        XzalgoChain_CTX ctx;
        xzalgochain_init(&ctx);
        xzalgochain_update(&ctx, data, len);
        xzalgochain_final(&ctx, output);
        xzalgochain_ctx_wipe(&ctx);
    }

    /* ==================== CONTEXT MANAGEMENT ==================== */
    void xzalgochain_init_lib(XzalgoChain_CTX *ctx) {
        xzalgochain_init(ctx);
    }

    void xzalgochain_update_lib(XzalgoChain_CTX *ctx, const uint8_t *data, size_t len) {
        xzalgochain_update(ctx, data, len);
    }

    void xzalgochain_final_lib(XzalgoChain_CTX *ctx, uint8_t output[XZALGOCHAIN_HASH_SIZE]) {
        xzalgochain_final(ctx, output);
    }

    void xzalgochain_ctx_reset_lib(XzalgoChain_CTX *ctx) {
        xzalgochain_ctx_reset(ctx);
    }

    void xzalgochain_ctx_wipe_lib(XzalgoChain_CTX *ctx) {
        xzalgochain_ctx_wipe(ctx);
    }

    /* ==================== UTILITY FUNCTIONS ==================== */
    void xzalgochain_copy_lib(uint8_t *dst, const uint8_t *src) {
        xzalgochain_copy(dst, src);
    }

    int xzalgochain_equals_lib(const uint8_t *h1, const uint8_t *h2) {
        return xzalgochain_equals(h1, h2);
    }

    /* ==================== INFO FUNCTIONS ==================== */
    const char* xzalgochain_version_lib(void) {
        return xzalgochain_version();
    }

    const char* xzalgochain_platform_info_lib(void) {
        return xzalgochain_platform_info();
    }

    /* ==================== SIMD FUNCTIONS ==================== */
    const char* xzalgochain_get_simd_name_lib(void) {
        return xzalgochain_get_simd_name();
    }

    int xzalgochain_get_simd_type_lib(void) {
        return xzalgochain_get_simd_type();
    }

    int xzalgochain_avx2_supported_lib(void) {
        #if defined(__AVX2__) && (defined(__x86_64__) || defined(__i386__))
        return 1;
        #else
        return 0;
        #endif
    }

    int xzalgochain_neon_supported_lib(void) {
        #if defined(__ARM_NEON) && (defined(__arm__) || defined(__aarch64__))
        return 1;
        #else
        return 0;
        #endif
    }

    /* ==================== FORCE SCALAR ==================== */
    void xzalgochain_force_scalar_lib(int force) {
        xzalgochain_force_scalar(force);
    }

    int xzalgochain_is_forced_scalar_lib(void) {
        return xzalgochain_is_forced_scalar();
    }

    #ifdef __cplusplus
}
#endif
