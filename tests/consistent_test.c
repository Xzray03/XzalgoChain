/*
 * consistent_test.c
 *
 * Test hash determinism / consistency.
 * Generates random inputs and checks if repeated hashing produces same outputs.
 *
 * Compile: clang -O3 -march=native -mtune=native -flto=full -fopenmp -lm -o consistent_test consistent_test.c
 *
 * Author: Xzrayツ
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../XzalgoChain/XzalgoChain.h"

#define INPUT_BYTES 64
#define HASH_BYTES 40
#define NUM_TESTS 500000

int main(void) {

    printf("===== Consistency Test =====\n");
    printf("Samples: %d\n", NUM_TESTS);
    printf("Input size: %d bytes, Hash size: %d bytes\n\n", INPUT_BYTES, HASH_BYTES);

    uint8_t (*inputs)[INPUT_BYTES] = malloc(NUM_TESTS * INPUT_BYTES);
    uint8_t (*hashes)[HASH_BYTES] = malloc(NUM_TESTS * HASH_BYTES);

    if (!inputs || !hashes) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    srand((unsigned)time(NULL));

    /* ========== Step 1: Generate all inputs and hashes ========== */
    for (int i = 0; i < NUM_TESTS; i++) {
        for (int j = 0; j < INPUT_BYTES; j++)
            inputs[i][j] = rand() & 0xFF;

        xzalgochain(inputs[i], INPUT_BYTES, hashes[i]);
    }

    printf("Generated %d hashes.\n", NUM_TESTS);

    /* ========== Step 2: Re-hash each input one by one and check consistency ========== */
    int failures = 0;
    uint8_t tmp_hash[HASH_BYTES];

    for (int i = 0; i < NUM_TESTS; i++) {
        xzalgochain(inputs[i], INPUT_BYTES, tmp_hash);

        if (memcmp(tmp_hash, hashes[i], HASH_BYTES) != 0) {
            failures++;
            if (failures <= 10) {
                printf("Mismatch at index %d\n", i);
            }
        }
    }

    if (failures == 0) {
        printf("All %d hashes are consistent. PASS\n", NUM_TESTS);
    } else {
        printf("Consistency test failed for %d / %d inputs.\n", failures, NUM_TESTS);
    }

    free(inputs);
    free(hashes);

    return 0;
}