/*
 * permutation_compression_test.c
 *
 * Permutation/Compression Test for hash function output uniformity
 *
 * Purpose:
 *   Tests whether the distribution of output bytes is uniform using
 *   a chi-squared goodness-of-fit test. Detects potential compression
 *   or bias in the hash output.
 *
 * Method:
 *   1. Generate NUM_SAMPLES random inputs
 *   2. Compute hash output for each input
 *   3. Build histogram of all output bytes
 *   4. Compute chi-squared statistic and approximate p-value
 *   5. Determine PASS/FAIL based on significance level ALPHA
 *
 * Compile: clang -O3 -march=native -mtune=native -flto=full -lm -o permutation_compression_test permutation_compression_test.c
 *
 * Author: Xzrayツ
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../XzalgoChain/XzalgoChain.h"

/* ======================== CONFIGURATION ======================== */
#define INPUT_BITS 512
#define INPUT_BYTES (INPUT_BITS / 8)
#define OUTPUT_BITS 320
#define OUTPUT_BYTES (OUTPUT_BITS / 8)

#define NUM_SAMPLES 1000000   // Number of random samples
#define NUM_BINS 256          // Histogram bins for byte values (0–255)
#define ALPHA 0.01            // Significance level

/* ======================== UTILITY FUNCTIONS ======================== */

/* Approximate p-value from chi-squared using Wilson-Hilferty transformation */
static inline double chi2_pvalue(double chi2, int df) {
    double z = (pow(chi2 / df, 1.0/3.0) - (1 - 2.0/(9.0*df))) / sqrt(2.0/(9.0*df));
    return erfc(fabs(z) / sqrt(2.0));
}

/* ======================== MAIN TEST ROUTINE ======================== */
int main(void) {

    printf("===== Permutation/Compression Test (Chi-Squared) =====\n");
    printf("Samples: %d\n", NUM_SAMPLES);

    uint32_t histogram[NUM_BINS];
    memset(histogram, 0, sizeof(histogram));

    uint8_t input[INPUT_BYTES];
    uint8_t output[OUTPUT_BYTES];

    /* ================== SAMPLING LOOP ================== */
    for (int s = 0; s < NUM_SAMPLES; s++) {

        /* Generate random input */
        for (int i = 0; i < INPUT_BYTES; i++)
            input[i] = rand() & 0xFF;

        /* Compute hash output */
        xzalgochain(input, INPUT_BYTES, output);

        /* Build histogram over output bytes */
        for (int b = 0; b < OUTPUT_BYTES; b++)
            histogram[output[b]]++;
    }

    /* ================== CHI-SQUARED ANALYSIS ================== */
    double expected = (double)(NUM_SAMPLES * OUTPUT_BYTES) / NUM_BINS;
    double chi2 = 0.0;

    for (int i = 0; i < NUM_BINS; i++) {
        double diff = histogram[i] - expected;
        chi2 += diff * diff / expected;
    }

    int df = NUM_BINS - 1;
    double pval = chi2_pvalue(chi2, df);

    /* ================== REPORT ================== */
    printf("Chi-squared statistic: %.3f\n", chi2);
    printf("Degrees of freedom: %d\n", df);
    printf("Approx. p-value: %.6f\n", pval);

    if (pval > ALPHA) {
        printf("Permutation/Compression Test: PASS\n");
    } else {
        printf("Permutation/Compression Test: FAIL\n");
    }

    return 0;
}