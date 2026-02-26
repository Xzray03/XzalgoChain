/*
 * avalanche_test.c
 *
 * Avalanche Statistical Test
 *
 * Purpose:
 *   Evaluates the avalanche effect of the XzalgoChain hash function by:
 *     1. Mean Hamming distance test
 *     2. Variance of Hamming distance test
 *     3. Global flip probability test
 *
 * Model:
 *   Assumes each hash bit is independent and flips with probability 0.5,
 *   i.e., Binomial(HASH_BITS, 0.5)
 *
 * Usage:
 *   Compile: clang -O3 -march=native -mtune=native -flto=full -fopenmp -lm -o avalanche_test avalanche_test.c
 *
 *   Run:
 *     ./avalanche_test
 *
 * Author: Xzrayツ
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "../XzalgoChain/XzalgoChain.h"

/* ======================== CONFIGURATION ======================== */
#define NUM_TESTS 1000000ULL         /* Total number of test vectors */
#define INPUT_SIZE 64                /* Size of input message in bytes */
#define HASH_BITS 320                /* Output hash size in bits */
#define HASH_BYTES (HASH_BITS / 8)
#define ALPHA 0.01                   /* Significance level for hypothesis tests */

/* ======================== UTILITY FUNCTIONS ======================== */

/* Count set bits in a byte (Hamming weight) */
static inline uint64_t popcnt8(uint8_t x) {
    return __builtin_popcount((unsigned)x);
}

/* Compute Hamming distance between two hash outputs */
static inline uint64_t hamming(const uint8_t *a, const uint8_t *b) {
    uint64_t dist = 0;
    for (int i = 0; i < HASH_BYTES; i++) {
        dist += popcnt8(a[i] ^ b[i]);
    }
    return dist;
}

/* ======================== STATISTICAL TESTING ======================== */

/*
 * Welford online algorithm for computing mean and variance
 * Updates mean and sum-of-squares-of-differences (M2) incrementally
 */
static void welford_update(double *mean, double *M2, double x, uint64_t n) {
    double delta = x - *mean;
    *mean += delta / (double)n;
    *M2 += delta * (x - *mean);
}

/*
 * Compute z-test p-value for two-tailed hypothesis
 * p = erfc(|z| / sqrt(2))
 */
static inline double p_value(double z) {
    return erfc(fabs(z) / sqrt(2.0));
}

/* ======================== MAIN TEST ROUTINE ======================== */

int main(void) {

    printf("===== Avalanche Statistical Test =====\n");
    printf("Number of samples: %llu\n", NUM_TESTS);
    printf("Input size: %d bytes, Hash output: %d bits\n\n", INPUT_SIZE, HASH_BITS);

    uint8_t input[INPUT_SIZE];
    uint8_t modified[INPUT_SIZE];
    uint8_t h1[HASH_BYTES];
    uint8_t h2[HASH_BYTES];

    double mean_hd = 0.0;      /* Mean Hamming distance */
    double M2 = 0.0;           /* For variance calculation */
    uint64_t total_flipped_bits = 0;

    /* ======================== TEST LOOP ======================== */
    for (uint64_t i = 0; i < NUM_TESTS; i++) {

        /* Generate random input */
        for (int j = 0; j < INPUT_SIZE; j++)
            input[j] = rand() & 0xFF;

        /* Copy input and flip a single random bit */
        memcpy(modified, input, INPUT_SIZE);
        int bit_to_flip = rand() % (INPUT_SIZE * 8);
        modified[bit_to_flip / 8] ^= (1 << (bit_to_flip % 8));

        /* Compute hash values */
        xzalgochain(input, INPUT_SIZE, h1);
        xzalgochain(modified, INPUT_SIZE, h2);

        /* Compute Hamming distance */
        uint64_t hd = hamming(h1, h2);
        total_flipped_bits += hd;

        /* Update online mean and variance */
        welford_update(&mean_hd, &M2, (double)hd, i + 1);
    }

    double var_hd = M2 / (NUM_TESTS - 1);

    /* ======================== EXPECTED VALUES ======================== */
    double ideal_mean = HASH_BITS / 2.0;
    double ideal_var  = HASH_BITS * 0.25;

    /* ======================== MEAN TEST ======================== */
    double se_mean = sqrt(ideal_var / NUM_TESTS);
    double z_mean = fabs(mean_hd - ideal_mean) / se_mean;
    double p_mean = p_value(z_mean);

    /* ======================== VARIANCE TEST ======================== */
    double se_var = sqrt((2.0 * ideal_var * ideal_var) / (NUM_TESTS - 1));
    double z_var = fabs(var_hd - ideal_var) / se_var;
    double p_var = p_value(z_var);

    /* ======================== GLOBAL FLIP PROBABILITY ======================== */
    double flip_prob = (double)total_flipped_bits / (NUM_TESTS * HASH_BITS);
    double se_flip = sqrt(0.25 / (NUM_TESTS * HASH_BITS));
    double z_flip = fabs(flip_prob - 0.5) / se_flip;
    double p_flip = p_value(z_flip);

    /* ======================== REPORT RESULTS ======================== */
    printf("Mean Hamming Distance: %.6f (ideal %.2f)\n", mean_hd, ideal_mean);
    printf("Mean test p-value: %.10f => %s\n\n", p_mean, p_mean >= ALPHA ? "PASS" : "FAIL");

    printf("Variance of Hamming Distance: %.6f (ideal %.2f)\n", var_hd, ideal_var);
    printf("Variance test p-value: %.10f => %s\n\n", p_var, p_var >= ALPHA ? "PASS" : "FAIL");

    printf("Global flip probability: %.8f (ideal 0.5)\n", flip_prob);
    printf("Flip probability p-value: %.10f => %s\n\n", p_flip, p_flip >= ALPHA ? "PASS" : "FAIL");

    if (p_mean >= ALPHA && p_var >= ALPHA && p_flip >= ALPHA) {
        printf("Overall Avalanche Result: PASS\n");
    } else {
        printf("Overall Avalanche Result: FAIL\n");
    }

    return 0;
}