/*
 * cross_correlation_test.c
 *
 * Cross-Correlation Test for hash function outputs
 *
 * Purpose:
 *   Measures independence of individual output bits when small changes
 *   are applied to input (bit flips). Checks whether output bits respond
 *   independently and with the correct flip probability (~0.5 for each bit).
 *
 * Method:
 *   1. Generate random input sample
 *   2. Flip a small random subset of input bits (delta)
 *   3. Compute hash before and after flip
 *   4. Count how often each output bit flips
 *   5. Analyze statistical deviation, RMS, max, Bonferroni-corrected significance
 *
 * Compile: clang -O3 -march=native -mtune=native -flto=full -fopenmp -lm -o cross_correlation_test cross_correlation_test.c
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
#define INPUT_BITS 512
#define INPUT_BYTES (INPUT_BITS / 8)
#define OUTPUT_BITS 320
#define OUTPUT_BYTES (OUTPUT_BITS / 8)

#define NUM_SAMPLES 1000000
#define ALPHA 0.01

/* Output bit flip counts */
static uint32_t corr[OUTPUT_BITS];

/* ======================== UTILITY FUNCTIONS ======================== */

/* Compute p-value from two-tailed z-score */
static inline double p_value(double z) {
    return erfc(fabs(z) / sqrt(2.0));
}

/* ======================== MAIN TEST ROUTINE ======================== */
int main(void) {

    printf("===== Cross-Correlation Test =====\n");
    printf("Samples: %d\n", NUM_SAMPLES);
    printf("Input size: %d bits, Output size: %d bits\n\n", INPUT_BITS, OUTPUT_BITS);

    memset(corr, 0, sizeof(corr));

    uint8_t input[INPUT_BYTES];
    uint8_t modified[INPUT_BYTES];
    uint8_t h1[OUTPUT_BYTES];
    uint8_t h2[OUTPUT_BYTES];

    /* ================== SAMPLING LOOP ================== */
    for (int s = 0; s < NUM_SAMPLES; s++) {

        /* Generate random input */
        for (int i = 0; i < INPUT_BYTES; i++)
            input[i] = rand() & 0xFF;

        /* Create delta: flip 5 random bits */
        memcpy(modified, input, INPUT_BYTES);
        for (int k = 0; k < 5; k++) {
            int bit = rand() % INPUT_BITS;
            modified[bit / 8] ^= (1 << (bit % 8));
        }

        /* Compute hashes */
        xzalgochain(input, INPUT_BYTES, h1);
        xzalgochain(modified, INPUT_BYTES, h2);

        /* Count output bit flips */
        for (int out_bit = 0; out_bit < OUTPUT_BITS; out_bit++) {
            int b = out_bit / 8;
            int bit = out_bit % 8;
            if ((h1[b] ^ h2[b]) & (1 << bit))
                corr[out_bit]++;
        }
    }

    /* ================== STATISTICAL ANALYSIS ================== */
    const int total_trials = NUM_SAMPLES;
    double se = sqrt(0.5 * 0.5 / total_trials); // Standard error for flip probability p = 0.5

    double global_mean = 0.0;
    double rms_dev = 0.0;
    double max_dev = 0.0;
    int significant_cells = 0;

    double bonf_alpha = ALPHA / OUTPUT_BITS;

    for (int i = 0; i < OUTPUT_BITS; i++) {
        double p = (double)corr[i] / total_trials;
        double dev = fabs(p - 0.5);

        global_mean += p;
        rms_dev += dev * dev;
        if (dev > max_dev) max_dev = dev;

        double z = dev / se;
        double pval = p_value(z);
        if (pval < bonf_alpha)
            significant_cells++;
    }

    global_mean /= OUTPUT_BITS;
    rms_dev = sqrt(rms_dev / OUTPUT_BITS);
    double expected_max = se * sqrt(2.0 * log(OUTPUT_BITS));

    /* ================== REPORT RESULTS ================== */
    printf("Global mean flip probability: %.6f\n", global_mean);
    printf("Ideal: 0.500000\n\n");

    printf("Standard error per bit: %.6f\n", se);
    printf("RMS deviation: %.6f\n", rms_dev);
    printf("Maximum deviation: %.6f\n", max_dev);
    printf("Expected max deviation (theoretical): %.6f\n\n", expected_max);

    printf("Bonferroni-corrected alpha: %.12f\n", bonf_alpha);
    printf("Significant bits after correction: %d / %d\n\n", significant_cells, OUTPUT_BITS);

    if (fabs(global_mean - 0.5) < 3*se &&
        max_dev < expected_max * 1.5 &&
        significant_cells == 0)
    {
        printf("Cross-Correlation Result: PASS\n");
    } else {
        printf("Cross-Correlation Result: FAIL\n");
    }

    return 0;
}
