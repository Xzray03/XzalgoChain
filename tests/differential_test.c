/*
 * differential_test.c
 *
 * Differential Probability Test for multiple input bit flips
 *
 * Purpose:
 *   Measures the avalanche property of a hash function when flipping
 *   1 up to MAX_FLIP_BITS input bits. Evaluates whether output bits
 *   flip independently with ~50% probability.
 *
 * Method:
 *   1. For each number of flipped input bits (1..MAX_FLIP_BITS):
 *       a) Generate NUM_SAMPLES random inputs
 *       b) Flip random unique positions in input
 *       c) Compute hash before and after flip
 *       d) Count output bit flips per bit
 *   2. Analyze statistics:
 *       - Global mean flip probability
 *       - RMS deviation
 *       - Maximum deviation
 *       - Bonferroni-corrected significance test
 *
 * Compile: clang -O3 -march=native -mtune=native -flto=full -fopenmp -lm -o differential_test differential_test.c
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

#define NUM_SAMPLES 50000
#define MAX_FLIP_BITS 50
#define ALPHA 0.01
#define TOLERANCE_FACTOR 2.0  // Flexible tolerance multiplier for max deviation

/* ======================== UTILITY FUNCTIONS ======================== */

/* Generate 'n' unique random bit positions in the range [0, max_bits-1] */
static void generate_flip_positions(int *pos, int n, int max_bits) {
    for (int i = 0; i < n; i++) {
        pos[i] = rand() % max_bits;
        for (int j = 0; j < i; j++) {
            if (pos[i] == pos[j]) {
                i--;  // Retry if duplicate
                break;
            }
        }
    }
}

/* Compute two-tailed p-value from z-score */
static inline double p_value(double z) {
    return erfc(fabs(z) / sqrt(2.0));
}

/* ======================== MAIN TEST ROUTINE ======================== */
int main(void) {

    printf("===== Differential Probability Test =====\n");
    printf("Samples per flip count: %d\n", NUM_SAMPLES);
    printf("Testing 1 up to %d input bits flipped\n\n", MAX_FLIP_BITS);

    uint8_t input[INPUT_BYTES], modified[INPUT_BYTES];
    uint8_t h1[OUTPUT_BYTES], h2[OUTPUT_BYTES];
    int flip_pos[MAX_FLIP_BITS];

    for (int flip_bits = 1; flip_bits <= MAX_FLIP_BITS; flip_bits++) {

        printf("==== Flipping %d input bit(s) ====\n", flip_bits);

        uint32_t diff[OUTPUT_BITS] = {0};

        /* ================== SAMPLING LOOP ================== */
        for (int s = 0; s < NUM_SAMPLES; s++) {

            /* Generate random input */
            for (int i = 0; i < INPUT_BYTES; i++)
                input[i] = rand() & 0xFF;

            xzalgochain(input, INPUT_BYTES, h1);

            /* Generate unique random bit positions to flip */
            generate_flip_positions(flip_pos, flip_bits, INPUT_BITS);

            memcpy(modified, input, INPUT_BYTES);
            for (int i = 0; i < flip_bits; i++)
                modified[flip_pos[i] / 8] ^= (1 << (flip_pos[i] % 8));

            xzalgochain(modified, INPUT_BYTES, h2);

            /* Count output bit flips */
            for (int out_bit = 0; out_bit < OUTPUT_BITS; out_bit++) {
                int b = out_bit / 8;
                int bit = out_bit % 8;
                if ((h1[b] ^ h2[b]) & (1 << bit))
                    diff[out_bit]++;
            }
        }

        /* ================== STATISTICAL ANALYSIS ================== */
        const int total_trials = NUM_SAMPLES;
        double se = sqrt(0.25 / total_trials); // Standard error for ideal p=0.5
        double global_mean = 0.0, rms_dev = 0.0, max_dev = 0.0;
        int significant_bits = 0;
        double bonf_alpha = ALPHA / OUTPUT_BITS;

        for (int i = 0; i < OUTPUT_BITS; i++) {
            double p = (double)diff[i] / total_trials;
            double dev = fabs(p - 0.5);
            global_mean += p;
            rms_dev += dev * dev;
            if (dev > max_dev) max_dev = dev;

            double z = dev / se;
            double pval = p_value(z);
            if (pval < bonf_alpha)
                significant_bits++;
        }

        global_mean /= OUTPUT_BITS;
        rms_dev = sqrt(rms_dev / OUTPUT_BITS);
        double expected_max = se * sqrt(2.0 * log(OUTPUT_BITS));

        /* ================== REPORT ================== */
        printf("Global mean flip probability: %.6f (Ideal: 0.5)\n", global_mean);
        printf("Standard error per bit: %.6f\n", se);
        printf("RMS deviation: %.6f\n", rms_dev);
        printf("Maximum deviation: %.6f\n", max_dev);
        printf("Expected max deviation: %.6f\n", expected_max);
        printf("Significant bits after Bonferroni correction: %d / %d\n", significant_bits, OUTPUT_BITS);

        /* Flexible PASS/FAIL criteria */
        if (fabs(global_mean - 0.5) < 3*se &&
            max_dev < expected_max * TOLERANCE_FACTOR &&
            significant_bits <= 1) {
            printf("Result: PASS\n\n");
        } else {
            printf("Result: FAIL\n\n");
        }
    }

    return 0;
}
