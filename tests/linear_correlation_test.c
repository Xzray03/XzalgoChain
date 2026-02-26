/*
 * linear_correlation_test.c
 *
 * Linear Correlation Test for cryptographic hash functions
 *
 * Purpose:
 *   Detects non-random linear dependencies between small combinations
 *   of input bits and individual output bits. Evaluates whether XOR
 *   of selected input bits predicts output bits significantly.
 *
 * Method:
 *   1. Generate NUM_SAMPLES random input vectors
 *   2. For each sample, compute hash output
 *   3. For each combination of up to MAX_INPUT_COMBO input bits:
 *       a) XOR the selected input bits
 *       b) Compare with each output bit
 *       c) Count matches where XOR(input) == output bit
 *   4. Analyze deviation from ideal 0.5 probability:
 *       - Global mean correlation
 *       - RMS deviation
 *       - Maximum deviation
 *       - Bonferroni-corrected significance test
 *
 * Compile: clang -O3 -march=native -mtune=native -flto=full -fopenmp -lm -o linear_correlation_test linear_correlation_test.c
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

/* Max number of input bits combined for linear approximation (1–4 recommended) */
#define MAX_INPUT_COMBO 3  

/* Store correlation counts: lc[combo][output_bit] */
static uint32_t lc[1 << MAX_INPUT_COMBO][OUTPUT_BITS];

/* ======================== UTILITY FUNCTIONS ======================== */

/* Compute two-tailed p-value from z-score */
static inline double p_value(double z) {
    return erfc(fabs(z) / sqrt(2.0));
}

/* ======================== MAIN TEST ROUTINE ======================== */
int main(void) {

    printf("===== Linear Correlation Test =====\n");
    printf("Samples: %d\n", NUM_SAMPLES);
    printf("Input size: %d bits, Output size: %d bits\n", INPUT_BITS, OUTPUT_BITS);
    printf("Max input bits combined: %d\n\n", MAX_INPUT_COMBO);

    memset(lc, 0, sizeof(lc));

    uint8_t input[INPUT_BYTES];
    uint8_t h[OUTPUT_BYTES];

    const int total_combos = 1 << MAX_INPUT_COMBO;

    /* ================== SAMPLING LOOP ================== */
    for (int s = 0; s < NUM_SAMPLES; s++) {

        /* Generate random input */
        for (int i = 0; i < INPUT_BYTES; i++)
            input[i] = rand() & 0xFF;

        /* Compute hash output */
        xzalgochain(input, INPUT_BYTES, h);

        /* Iterate over all non-empty input combinations */
        for (int combo = 1; combo < total_combos; combo++) {

            int xor_input = 0;
            for (int bit = 0; bit < MAX_INPUT_COMBO; bit++) {
                if (combo & (1 << bit)) {
                    int byte = bit / 8;
                    int bitpos = bit % 8;
                    xor_input ^= (input[byte] >> bitpos) & 1;
                }
            }

            /* Record correlation with each output bit */
            for (int out_bit = 0; out_bit < OUTPUT_BITS; out_bit++) {
                int b = out_bit / 8;
                int bit = out_bit % 8;
                int out_val = (h[b] >> bit) & 1;

                if ((xor_input ^ out_val) == 0)
                    lc[combo][out_bit]++;
            }
        }
    }

    /* ================== STATISTICAL ANALYSIS ================== */
    const int total_cells = (total_combos - 1) * OUTPUT_BITS;
    const int total_trials = NUM_SAMPLES;

    double se = sqrt(0.25 / total_trials); // standard error for unbiased probability 0.5
    double global_mean = 0.0;
    double rms_dev = 0.0;
    double max_dev = 0.0;
    int significant_cells = 0;

    double bonf_alpha = ALPHA / total_cells;

    for (int combo = 1; combo < total_combos; combo++) {
        for (int out_bit = 0; out_bit < OUTPUT_BITS; out_bit++) {
            double p = (double)lc[combo][out_bit] / total_trials;
            double dev = fabs(p - 0.5);

            global_mean += p;
            rms_dev += dev * dev;
            if (dev > max_dev)
                max_dev = dev;

            double z = dev / se;
            double pval = p_value(z);
            if (pval < bonf_alpha)
                significant_cells++;
        }
    }

    global_mean /= total_cells;
    rms_dev = sqrt(rms_dev / total_cells);
    double expected_max = se * sqrt(2.0 * log(total_cells));

    /* ================== REPORT RESULTS ================== */
    printf("Global mean correlation: %.6f\n", global_mean);
    printf("Ideal: 0.500000\n\n");

    printf("Standard error per cell: %.6f\n", se);
    printf("RMS deviation: %.6f\n", rms_dev);
    printf("Maximum deviation: %.6f\n", max_dev);
    printf("Expected max deviation (theoretical): %.6f\n\n", expected_max);

    printf("Bonferroni corrected alpha: %.12f\n", bonf_alpha);
    printf("Significant cells after correction: %d / %d\n\n",
           significant_cells, total_cells);

    if (fabs(global_mean - 0.5) < 3*se &&
        max_dev < expected_max * 1.5 &&
        significant_cells == 0)
    {
        printf("Linear Correlation Test Result: PASS\n");
    } else {
        printf("Linear Correlation Test Result: FAIL\n");
    }

    return 0;
}
