/*
 * XzalgoChain - 320-bit Cryptographic Hash Function
 * Copyright 2026 Xzrayツ
 *
 * xzalgochain_wasm.c - Implementation wrapper for XzalgoChain library targeted for WASM
 * This file ensures symbols are exported from the compiled WebAssembly module.
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
    void xzalgochain_wasm(const uint8_t *data, size_t len, uint8_t output[XZALGOCHAIN_HASH_SIZE]) {
        XzalgoChain_CTX ctx;
        xzalgochain_init(&ctx);
        xzalgochain_update(&ctx, data, len);
        xzalgochain_final(&ctx, output);
        xzalgochain_ctx_wipe(&ctx);
    }

    /* ==================== CONTEXT MANAGEMENT ==================== */
    void xzalgochain_init_wasm(XzalgoChain_CTX *ctx) {
        xzalgochain_init(ctx);
    }

    void xzalgochain_update_wasm(XzalgoChain_CTX *ctx, const uint8_t *data, size_t len) {
        xzalgochain_update(ctx, data, len);
    }

    void xzalgochain_final_wasm(XzalgoChain_CTX *ctx, uint8_t output[XZALGOCHAIN_HASH_SIZE]) {
        xzalgochain_final(ctx, output);
    }

    void xzalgochain_ctx_reset_wasm(XzalgoChain_CTX *ctx) {
        xzalgochain_ctx_reset(ctx);
    }

    void xzalgochain_ctx_wipe_wasm(XzalgoChain_CTX *ctx) {
        xzalgochain_ctx_wipe(ctx);
    }

    /* ==================== UTILITY FUNCTIONS ==================== */
    void xzalgochain_copy_wasm(uint8_t *dst, const uint8_t *src) {
        xzalgochain_copy(dst, src);
    }

    int xzalgochain_equals_wasm(const uint8_t *h1, const uint8_t *h2) {
        return xzalgochain_equals(h1, h2);
    }

    /* ==================== INFO FUNCTIONS ==================== */
    const char* xzalgochain_version_wasm(void) {
        return xzalgochain_version();
    }

    #ifdef __cplusplus
}
#endif
