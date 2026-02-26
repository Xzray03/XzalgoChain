/*
 * dot_test.c
 *
 * Linear Combination / Dot Product Test for hash function outputs.
 * Measures linear dependency between input bit combinations and output bits.
 *
 * Compile: clang -O3 -march=native -mtune=native -flto=full -fopenmp -lm -o dot_test dot_test.c
 *
 * Author: Xzrayツ
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../XzalgoChain/XzalgoChain.h"

#define INPUT_BITS 512
#define INPUT_BYTES 64
#define OUTPUT_BITS 320
#define OUTPUT_BYTES 40

#define NUM_SAMPLES 1000000
#define MAX_INPUT_COMBO 6
#define ALPHA 0.01

static uint32_t dp[1 << MAX_INPUT_COMBO][OUTPUT_BITS];

static inline void random_input(uint8_t *buf) {
    for (int i = 0; i < INPUT_BYTES; i++)
        buf[i] = rand() & 0xFF;
}

int main(void) {
    printf("Dot Product / Linear Combination Test\n");
    printf("Samples: %d\n\n", NUM_SAMPLES);

    memset(dp, 0, sizeof(dp));

    uint8_t input[INPUT_BYTES];
    uint8_t h[OUTPUT_BYTES];

    const int total_combos = 1 << MAX_INPUT_COMBO;

    /* ================== Sampling ================== */
    for (int s = 0; s < NUM_SAMPLES; s++) {
        random_input(input);
        xzalgochain(input, INPUT_BYTES, h);

        for (int combo = 1; combo < total_combos; combo++) {
            int xor_input = 0;

            for (int bit = 0; bit < MAX_INPUT_COMBO; bit++) {
                if (combo & (1 << bit)) {
                    int byte = bit / 8;
                    int bitpos = bit % 8;
                    xor_input ^= (input[byte] >> bitpos) & 1;
                }
            }

            for (int out_bit = 0; out_bit < OUTPUT_BITS; out_bit++) {
                int byte = out_bit / 8;
                int bitpos = out_bit % 8;
                int out_val = (h[byte] >> bitpos) & 1;

                if ((xor_input ^ out_val) == 0)
                    dp[combo][out_bit]++;
            }
        }
    }

    /* ================== Analysis ================== */
    const int total_cells = (total_combos - 1) * OUTPUT_BITS;
    double se = sqrt(0.25 / NUM_SAMPLES);
    double global_mean = 0.0;
    double rms_dev = 0.0;
    double max_dev = 0.0;
    int significant_cells = 0;
    double bonf_alpha = ALPHA / total_cells;

    for (int combo = 1; combo < total_combos; combo++) {
        for (int out_bit = 0; out_bit < OUTPUT_BITS; out_bit++) {
            double p = (double)dp[combo][out_bit] / NUM_SAMPLES;
            double dev = fabs(p - 0.5);
            global_mean += p;
            rms_dev += dev * dev;
            if (dev > max_dev) max_dev = dev;

            double z = dev / se;
            double pval = erfc(z / sqrt(2.0));
            if (pval < bonf_alpha)
                significant_cells++;
        }
    }

    global_mean /= total_cells;
    rms_dev = sqrt(rms_dev / total_cells);
    double expected_max = se * sqrt(2.0 * log(total_cells));

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
        printf("Dot Product Test Result: PASS\n");
    } else {
        printf("Dot Product Test Result: FAIL\n");
    }

    return 0;
}
