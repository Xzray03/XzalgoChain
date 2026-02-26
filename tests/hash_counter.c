/*
 * hash_counter.c - Infinite loop hashing with 320-bit counter (optimized)
 *
 * Features:
 *   - Buffered output for fewer syscalls
 *   - Multi-threaded counter hashing using OpenMP
 *   - Maintains 320-bit counter increment safely per thread
 *
 * Compile: clang -O3 -march=native -mtune=native -flto=full -fopenmp -o hash_counter hash_counter.c
 *
 * Run:
 *   ./hash_counter | RNG_test stdin -multithreaded
 *
 * Author: Xzrayツ
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <omp.h>
#include "../XzalgoChain/XzalgoChain.h"

#define NUM_THREADS (omp_get_num_procs())
#define BUF_SIZE 65536              // Output buffer per thread

int main(void) {

    uint64_t counter_base[5] = {0};
    uint8_t hash_output[XZALGOCHAIN_HASH_SIZE];

    // Each thread maintains its own counter offset
    #pragma omp parallel num_threads(NUM_THREADS) private(hash_output)
    {
        int tid = omp_get_thread_num();
        uint64_t counter[5];
        memcpy(counter, counter_base, sizeof(counter_base));

        // Offset counter by thread ID
        counter[0] += tid;

        uint8_t out_buf[BUF_SIZE];
        size_t buf_pos = 0;

        while (1) {
            xzalgochain((const uint8_t*)counter, sizeof(counter), hash_output);

            // Copy hash to buffer
            memcpy(out_buf + buf_pos, hash_output, XZALGOCHAIN_HASH_SIZE);
            buf_pos += XZALGOCHAIN_HASH_SIZE;

            // Flush buffer when full
            if (buf_pos + XZALGOCHAIN_HASH_SIZE > BUF_SIZE) {
                #pragma omp critical
                {
                    fwrite(out_buf, 1, buf_pos, stdout);
                    fflush(stdout);
                }
                buf_pos = 0;
            }

            // Increment 320-bit counter
            for (int i = 0; i < 5; i++) {
                counter[i]++;
                if (counter[i] != 0) break;
            }
        }
    }

    return 0;
}