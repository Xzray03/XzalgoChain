/*
 * bic_test.c
 *
 * Bit Independence Criterion (BIC) test
 *
 * Purpose:
 *   Evaluates bit independence of a hash function by testing whether
 *   output bits flip independently when individual input bits are flipped.
 *
 * Method:
 *   1. For each sample, generate a random input and compute hash h1
 *   2. Flip each input bit in turn, compute hash h2
 *   3. For each pair of output bits (i != j), count how often both bits flip
 *   4. Analyze results with:
 *        - Global mean flip correlation
 *        - RMS deviation from ideal 0.25
 *        - Maximum deviation vs theoretical expectation
 *        - Bonferroni-corrected significance test
 *
 * Compile: clang -O3 -march=native -mtune=native -flto=full -fopenmp -lm -o bic_test bic_test.c
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

#define NUM_SAMPLES 10000         /* Number of random input samples */
#define ALPHA 0.01                /* Significance level for hypothesis tests */

/* BIC co-occurrence matrix: bic[i][j] counts how often output bits i and j flip together */
static uint32_t bic[OUTPUT_BITS][OUTPUT_BITS];

/* ======================== UTILITY FUNCTIONS ======================== */

/* Compute p-value from two-tailed z-score */
static inline double p_value(double z) {
    return erfc(fabs(z) / sqrt(2.0));
}

/* ======================== MAIN TEST ROUTINE ======================== */
int main(void) {

    printf("===== Bit Independence Criterion (BIC) Test =====\n");
    printf("Samples: %d\n", NUM_SAMPLES);
    printf("Input size: %d bits, Output size: %d bits\n\n", INPUT_BITS, OUTPUT_BITS);

    memset(bic, 0, sizeof(bic));

    uint8_t input[INPUT_BYTES];
    uint8_t modified[INPUT_BYTES];
    uint8_t h1[OUTPUT_BYTES];
    uint8_t h2[OUTPUT_BYTES];

    /* ================== SAMPLING LOOP ================== */
    for (int s = 0; s < NUM_SAMPLES; s++) {

        /* Generate random input */
        for (int i = 0; i < INPUT_BYTES; i++)
            input[i] = rand() & 0xFF;

        xzalgochain(input, INPUT_BYTES, h1);

        /* Flip each input bit individually */
        for (int in_bit = 0; in_bit < INPUT_BITS; in_bit++) {

            memcpy(modified, input, INPUT_BYTES);
            modified[in_bit / 8] ^= (1 << (in_bit % 8));

            xzalgochain(modified, INPUT_BYTES, h2);

            /* Compare output bits */
            for (int out_i = 0; out_i < OUTPUT_BITS; out_i++) {
                int byte_i = out_i / 8;
                int bit_i  = out_i % 8;
                int flip_i = ((h1[byte_i] ^ h2[byte_i]) & (1 << bit_i)) ? 1 : 0;

                for (int out_j = 0; out_j < OUTPUT_BITS; out_j++) {
                    if (out_i == out_j) continue;

                    int byte_j = out_j / 8;
                    int bit_j  = out_j % 8;
                    int flip_j = ((h1[byte_j] ^ h2[byte_j]) & (1 << bit_j)) ? 1 : 0;

                    if (flip_i && flip_j)
                        bic[out_i][out_j]++;
                }
            }
        }
    }

    /* ================== STATISTICAL ANALYSIS ================== */
    const int total_cells = OUTPUT_BITS * (OUTPUT_BITS - 1);
    const int total_trials = NUM_SAMPLES * INPUT_BITS;

    /* Standard error for two-bit flip probability (ideal p = 0.25) */
    double se = sqrt(0.25 * 0.75 / total_trials);

    double global_mean = 0.0;
    double rms_dev = 0.0;
    double max_dev = 0.0;
    int significant_cells = 0;

    /* Bonferroni correction for multiple testing */
    double bonf_alpha = ALPHA / total_cells;

    for (int i = 0; i < OUTPUT_BITS; i++) {
        for (int j = 0; j < OUTPUT_BITS; j++) {
            if (i == j) continue;

            double p = (double)bic[i][j] / total_trials;
            double dev = fabs(p - 0.25);

            global_mean += p;
            rms_dev += dev * dev;

            if (dev > max_dev)
                max_dev = dev;

            /* Two-sided z-test */
            double z = dev / se;
            double pval = p_value(z);

            if (pval < bonf_alpha)
                significant_cells++;
        }
    }

    global_mean /= total_cells;
    rms_dev = sqrt(rms_dev / total_cells);

    double expected_max = se * sqrt(2.0 * log(total_cells));

    /* ================== REPORT ================== */
    printf("Global mean flip correlation: %.6f (ideal 0.25)\n", global_mean);
    printf("Standard error per cell: %.6f\n", se);
    printf("RMS deviation: %.6f\n", rms_dev);
    printf("Maximum deviation: %.6f\n", max_dev);
    printf("Expected max deviation (theoretical): %.6f\n\n", expected_max);

    printf("Bonferroni-corrected alpha: %.12f\n", bonf_alpha);
    printf("Significant cells after correction: %d / %d\n\n", significant_cells, total_cells);

    if (fabs(global_mean - 0.25) < 3*se &&
        max_dev < expected_max * 1.5 &&
        significant_cells == 0)
    {
        printf("BIC Result: PASS\n");
    } else {
        printf("BIC Result: FAIL\n");
    }

    return 0;
}