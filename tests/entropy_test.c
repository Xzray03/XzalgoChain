/*
 * entropy_test.c
 *
 * Shannon Entropy and Bit Bias Analysis for Hash Function Output
 *
 * Purpose:
 *   Evaluates the per-bit randomness of a hash function by:
 *     1. Computing the probability of each output bit being '1'
 *     2. Calculating Shannon entropy per bit
 *     3. Detecting biased bits (P(1) < 0.45 or P(1) > 0.55)
 *
 * Method:
 *   - Generate NUM_SAMPLES random inputs
 *   - Compute hash output for each input
 *   - Count the number of '1's per output bit
 *   - Compute per-bit entropy and average entropy
 *
 * Compile: clang -O3 -march=native -mtune=native -flto=full -fopenmp -lm -o entropy_test entropy_test.c
 *
 * Author: Xzrayツ
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include "../XzalgoChain/XzalgoChain.h"

/* ======================== CONFIGURATION ======================== */
#define INPUT_BITS 512
#define INPUT_BYTES (INPUT_BITS / 8)
#define OUTPUT_BITS 320
#define OUTPUT_BYTES (OUTPUT_BITS / 8)

#define NUM_SAMPLES 1000000

/* ======================== UTILITY FUNCTIONS ======================== */

/* Compute Shannon entropy for probability p of bit being '1' */
static inline double shannon_entropy(double p1) {
    double p0 = 1.0 - p1;
    double h = 0.0;
    if (p1 > 0.0) h -= p1 * log2(p1);
    if (p0 > 0.0) h -= p0 * log2(p0);
    return h;
}

/* ======================== MAIN TEST ROUTINE ======================== */
int main(void) {

    printf("===== Entropy Test =====\n");
    printf("Number of samples: %d\n\n", NUM_SAMPLES);

    uint8_t input[INPUT_BYTES];
    uint8_t output[OUTPUT_BYTES];

    uint32_t bit_count[OUTPUT_BITS] = {0};

    /* ================== SAMPLING LOOP ================== */
    for (int s = 0; s < NUM_SAMPLES; s++) {

        /* Generate random input */
        for (int i = 0; i < INPUT_BYTES; i++)
            input[i] = rand() & 0xFF;

        /* Compute hash output */
        xzalgochain(input, INPUT_BYTES, output);

        /* Count '1' bits per output bit */
        for (int b = 0; b < OUTPUT_BITS; b++) {
            int byte = b / 8;
            int bit = b % 8;
            if (output[byte] & (1 << bit))
                bit_count[b]++;
        }
    }

    /* ================== STATISTICAL ANALYSIS ================== */
    double entropy_total = 0.0;
    int zero_bias_bits = 0, one_bias_bits = 0;

    printf("Bit\tCount_1\tProb_1\tEntropy\n");
    for (int b = 0; b < OUTPUT_BITS; b++) {
        double p1 = (double)bit_count[b] / NUM_SAMPLES;

        double h = shannon_entropy(p1);
        entropy_total += h;

        if (p1 < 0.45)
            zero_bias_bits++;
        else if (p1 > 0.55)
            one_bias_bits++;

        printf("%3d\t%6u\t%.6f\t%.6f\n", b, bit_count[b], p1, h);
    }

    double avg_entropy = entropy_total / OUTPUT_BITS;

    /* ================== REPORT ================== */
    printf("\nAverage bit entropy: %.6f bits (max 1.0)\n", avg_entropy);
    printf("Total biased bits (P(1)<0.45 or P(1)>0.55): %d\n",
           zero_bias_bits + one_bias_bits);

    if (avg_entropy > 0.99 && zero_bias_bits + one_bias_bits == 0) {
        printf("Entropy Test: PASS\n");
    } else {
        printf("Entropy Test: FAIL\n");
    }

    return 0;
}
