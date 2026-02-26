/*
 * sac_test.c
 *
 * Strict Avalanche Criterion (SAC) Test for cryptographic hash functions
 *
 * Purpose:
 *   Evaluates the strict avalanche property: flipping a single input bit
 *   should cause each output bit to flip with ~50% probability.
 *
 * Method:
 *   1. Generate NUM_SAMPLES random input vectors
 *   2. For each input bit:
 *       a) Flip the bit
 *       b) Compute hash output before and after the flip
 *       c) Count output bit flips per input bit
 *   3. Analyze statistics:
 *       - Global mean flip probability
 *       - RMS deviation
 *       - Maximum deviation
 *       - Bonferroni-corrected significance testing per cell
 *
 * Compile: clang -O3 -march=native -mtune=native -flto=full -fopenmp -lm -o sac_test sac_test.c
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

#define NUM_SAMPLES 10000
#define ALPHA 0.01

/* Flip count matrix: sac[input_bit][output_bit] */
static uint32_t sac[INPUT_BITS][OUTPUT_BITS];

/* ======================== UTILITY FUNCTIONS ======================== */

/* Compute two-tailed p-value from z-score */
static inline double p_value(double z) {
    return erfc(fabs(z) / sqrt(2.0));
}

/* ======================== MAIN TEST ROUTINE ======================== */
int main(void) {

    printf("===== Strict Avalanche Criterion (SAC) Test =====\n");
    printf("Samples: %d\n", NUM_SAMPLES);
    printf("Input size: %d bits, Output size: %d bits\n\n", INPUT_BITS, OUTPUT_BITS);

    memset(sac, 0, sizeof(sac));

    uint8_t input[INPUT_BYTES];
    uint8_t modified[INPUT_BYTES];
    uint8_t h1[OUTPUT_BYTES], h2[OUTPUT_BYTES];

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

            /* Count output bit flips for this input bit */
            for (int out_bit = 0; out_bit < OUTPUT_BITS; out_bit++) {
                int b = out_bit / 8;
                int bit = out_bit % 8;
                if ((h1[b] ^ h2[b]) & (1 << bit))
                    sac[in_bit][out_bit]++;
            }
        }
    }

    /* ================== STATISTICAL ANALYSIS ================== */
    const int total_cells = INPUT_BITS * OUTPUT_BITS;
    double se = sqrt(0.25 / NUM_SAMPLES);

    double global_mean = 0.0;
    double rms_dev = 0.0;
    double max_dev = 0.0;
    int significant_cells = 0;

    double bonf_alpha = ALPHA / total_cells;

    for (int i = 0; i < INPUT_BITS; i++) {
        for (int j = 0; j < OUTPUT_BITS; j++) {

            double p = (double)sac[i][j] / NUM_SAMPLES;
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
    printf("Global mean flip probability: %.6f\n", global_mean);
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
        printf("SAC Result: PASS\n");
    } else {
        printf("SAC Result: FAIL\n");
    }

    return 0;
}
