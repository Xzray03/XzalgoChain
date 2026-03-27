/*
 * CSPRNG Function (Part of XzalgoChain)
 * Copyright 2026 Xzrayツ
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef XZ_CSPRNG_H
#define XZ_CSPRNG_H

#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>

/* Maximum request size to prevent denial of service attacks */
#define XZ_CSPRNG_MAX_REQUEST (1024 * 1024) /* 1 MB limit */

/**
 * Cryptographically secure pseudorandom number generator
 * Fills buffer with cryptographically secure random bytes using system RNG
 *
 * @param buf Pointer to buffer to fill with random bytes (must not be NULL)
 * @param len Number of bytes to generate (must be > 0 and <= XZ_CSPRNG_MAX_REQUEST)
 * @return 0 on success, -1 on error (buffer zeroed on error if memset_s available)
 *
 * Security considerations:
 * - Uses system's cryptographically secure RNG (BCrypt on Windows,
 *   SecRandomCopyBytes on macOS, /dev/urandom on Unix-like systems)
 * - Prevents large allocations that could cause denial of service
 * - Zeroes buffer on failure to prevent information leakage
 * - Thread-safe on all supported platforms
 */
static int xz_csp_rng(void* buf, size_t len);

#if defined(_WIN32)
    #include <windows.h>
    #include <bcrypt.h>

    #pragma comment(lib, "bcrypt.lib")

/**
 * Windows implementation using BCryptGenRandom
 * Uses system-preferred RNG (CNG) with optimal performance
 */
static inline int xz_csp_rng(void* buf, size_t len) {
    /* Validate input parameters */
    if (!buf || len == 0) {
        return -1;
    }

    /* Prevent excessively large allocations (DoS protection) */
    if (len > XZ_CSPRNG_MAX_REQUEST) {
        return -1;
    }

    /* Generate cryptographically secure random bytes */
    NTSTATUS status = BCryptGenRandom(
        NULL,                           /* Use default algorithm provider */
        (PUCHAR) buf,                   /* Output buffer */
        (ULONG) len,                    /* Number of bytes */
        BCRYPT_USE_SYSTEM_PREFERRED_RNG /* Use system RNG */
    );

    /* Handle failure - zero buffer to prevent information leak */
    if (status != STATUS_SUCCESS) {
    #ifdef __STDC_LIB_EXT1__
        memset_s(buf, len, 0, len);
    #else
        volatile uint8_t* p = (volatile uint8_t*) buf;
        for (size_t i = 0; i < len; i++) {
            p[i] = 0;
        }
    #endif
        return -1;
    }

    return 0;
}

#elif defined(__APPLE__)
    #include <Security/Security.h>

/**
 * macOS/iOS implementation using SecRandomCopyBytes
 * Uses Apple's Security Framework with hardware entropy sources
 */
static inline int xz_csp_rng(void* buf, size_t len) {
    /* Validate input parameters */
    if (!buf || len == 0) {
        return -1;
    }

    /* Prevent excessively large allocations (DoS protection) */
    if (len > XZ_CSPRNG_MAX_REQUEST) {
        return -1;
    }

    /* Generate cryptographically secure random bytes */
    if (SecRandomCopyBytes(kSecRandomDefault, len, buf) != 0) {
    #ifdef __STDC_LIB_EXT1__
        memset_s(buf, len, 0, len);
    #else
        volatile uint8_t* p = (volatile uint8_t*) buf;
        for (size_t i = 0; i < len; i++) {
            p[i] = 0;
        }
    #endif
        return -1;
    }

    return 0;
}

#else
    #include <fcntl.h>
    #include <unistd.h>

/**
 * Unix/Linux implementation using /dev/urandom
 * Non-blocking, cryptographically secure random device
 */
static inline int xz_csp_rng(void* buf, size_t len) {
    ssize_t r;
    size_t remaining;
    uint8_t* ptr;
    int fd;
    int saved_errno;

    /* Validate input parameters */
    if (!buf || len == 0) {
        return -1;
    }

    /* Prevent excessively large allocations (DoS protection) */
    if (len > XZ_CSPRNG_MAX_REQUEST) {
        return -1;
    }

    /* Open /dev/urandom with O_CLOEXEC to prevent FD leakage in child processes */
    fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return -1;
    }

    /* Read exactly 'len' bytes, handling partial reads */
    ptr = (uint8_t*) buf;
    remaining = len;

    while (remaining > 0) {
        r = read(fd, ptr, remaining);

        if (r < 0) {
            /* Handle interrupt signals gracefully */
            if (errno == EINTR) {
                continue;
            }
            /* Read error occurred */
            saved_errno = errno;
            close(fd);

    /* Zero buffer to prevent information leak on failure */
    #ifdef __STDC_LIB_EXT1__
            memset_s(buf, len, 0, len);
    #else
            volatile uint8_t* p = (volatile uint8_t*) buf;
            for (size_t i = 0; i < len; i++) {
                p[i] = 0;
            }
    #endif
            errno = saved_errno;
            return -1;
        }

        if (r == 0) {
            /* EOF should not happen with /dev/urandom */
            close(fd);
    #ifdef __STDC_LIB_EXT1__
            memset_s(buf, len, 0, len);
    #else
            volatile uint8_t* p = (volatile uint8_t*) buf;
            for (size_t i = 0; i < len; i++) {
                p[i] = 0;
            }
    #endif
            errno = EIO;
            return -1;
        }

        ptr += r;
        remaining -= (size_t) r;
    }

    close(fd);
    return 0;
}
#endif

/**
 * Securely fill memory with random bytes with retry mechanism
 * Wrapper around xz_csp_rng that retries on transient failures
 *
 * @param buf Pointer to buffer to fill (must not be NULL)
 * @param len Number of bytes to generate (must be > 0)
 * @param max_retries Maximum number of retry attempts
 * @return 0 on success, -1 on error
 */
static inline int xz_csp_rng_retry(void* buf, size_t len, unsigned int max_retries) {
    int result;
    unsigned int attempts = 0;

    if (!buf || len == 0) {
        return -1;
    }

    do {
        result = xz_csp_rng(buf, len);
        if (result == 0) {
            return 0;
        }
        attempts++;
    } while (attempts < max_retries);

    return -1;
}

/**
 * Generate random 32-bit unsigned integer
 *
 * @param out Pointer to store random value
 * @return 0 on success, -1 on error
 */
static inline int xz_csp_rng_uint32(uint32_t* out) {
    if (!out) {
        return -1;
    }
    return xz_csp_rng(out, sizeof(uint32_t));
}

/**
 * Generate random 64-bit unsigned integer
 *
 * @param out Pointer to store random value
 * @return 0 on success, -1 on error
 */
static inline int xz_csp_rng_uint64(uint64_t* out) {
    if (!out) {
        return -1;
    }
    return xz_csp_rng(out, sizeof(uint64_t));
}

/**
 * Generate random number in range [min, max] (inclusive)
 *
 * @param min Minimum value (inclusive)
 * @param max Maximum value (inclusive)
 * @param out Pointer to store random value
 * @return 0 on success, -1 on error
 *
 * Note: Uses rejection sampling to avoid modulo bias
 */
static inline int xz_csp_rng_range(uint32_t min, uint32_t max, uint32_t* out) {
    uint32_t range;
    uint32_t limit;
    uint32_t rand_val;

    if (!out || min > max) {
        return -1;
    }

    range = max - min + 1;

    /* Calculate rejection limit to eliminate modulo bias */
    limit = UINT32_MAX - (UINT32_MAX % range);

    do {
        if (xz_csp_rng_uint32(&rand_val) != 0) {
            return -1;
        }
    } while (rand_val >= limit);

    *out = min + (rand_val % range);
    return 0;
}

#endif /* XZ_CSPRNG_H */
