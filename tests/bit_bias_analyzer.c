/*
 * bit_bias_analyzer.c
 *
 * Statistical bias analysis
 *
 * Tests:
 *   - Frequency (Monobit) Test
 *   - Per-bit Chi-Square Test
 *
 * Compile: clang -O3 -march=native -mtune=native -flto=full -fopenmp -lm -o bit_bias_analyzer bit_bias_analyzer.c
 *
 * Author: Xzrayツ
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../XzalgoChain/XzalgoChain.h"

#define TOTAL_HASHES 10000000ULL
#define BITS_PER_HASH 320
#define BYTES_PER_HASH (BITS_PER_HASH / 8)
#define ALPHA 0.01

typedef struct {
    uint64_t count_one[BITS_PER_HASH];
    uint64_t total_bits;
    uint64_t total_hashes;
} BiasCounters;

/* ================= NUMERICAL CORE ================= */

static const double lanczos_coef[] = {
    676.5203681218851,
    -1259.1392167224028,
    771.32342877765313,
    -176.61502916214059,
    12.507343278686905,
    -0.13857109526572012,
    9.9843695780195716e-6,
    1.5056327351493116e-7
};

double gamma_lanczos(double z) {
    if (z < 0.5)
        return M_PI / (sin(M_PI * z) * gamma_lanczos(1 - z));

    z -= 1;
    double x = 0.99999999999980993;
    for (int i = 0; i < 8; i++)
        x += lanczos_coef[i] / (z + i + 1);

    double t = z + 7.5;
    return sqrt(2 * M_PI) * pow(t, z + 0.5) * exp(-t) * x;
}

double igamc_cf(double a, double x) {
    const int MAX_ITER = 1000;
    const double EPS = 1e-14;
    const double FPMIN = 1e-300;

    double b = x + 1 - a;
    double c = 1.0 / FPMIN;
    double d = 1.0 / b;
    double h = d;

    for (int i = 1; i <= MAX_ITER; i++) {
        double an = -i * (i - a);
        b += 2.0;
        d = an * d + b;
        if (fabs(d) < FPMIN) d = FPMIN;
        c = b + an / c;
        if (fabs(c) < FPMIN) c = FPMIN;
        d = 1.0 / d;
        double delta = d * c;
        h *= delta;
        if (fabs(delta - 1.0) < EPS) break;
    }

    return exp(-x + a * log(x) - lgamma(a)) * h;
}

double igamc(double a, double x) {
    if (x <= 0) return 1.0;
    if (x < a + 1.0)
        return 1.0 - igamc_cf(a, x);
    return igamc_cf(a, x);
}

/* ================= STATISTICAL TESTS ================= */

double monobit_test(BiasCounters *c) {
    double n = (double)c->total_bits;
    double sum = 0.0;

    for (int i = 0; i < BITS_PER_HASH; i++)
        sum += (2.0 * c->count_one[i] - c->total_hashes);

    double s_obs = fabs(sum) / sqrt(n);
    return erfc(s_obs / sqrt(2.0));
}

double chi_square_test(BiasCounters *c) {
    double expected = c->total_hashes / 2.0;
    double chi_square = 0.0;

    for (int i = 0; i < BITS_PER_HASH; i++) {
        double obs = (double)c->count_one[i];
        chi_square += (obs - expected) * (obs - expected) / expected;
    }

    return igamc(BITS_PER_HASH / 2.0, chi_square / 2.0);
}

/* ================= COUNTERS ================= */

void init_counters(BiasCounters *c) {
    memset(c, 0, sizeof(BiasCounters));
}

void update_counters(BiasCounters *c, const uint8_t *hash) {
    for (int bit = 0; bit < BITS_PER_HASH; bit++) {
        int byte_pos = bit / 8;
        int bit_in_byte = 7 - (bit % 8);
        if (hash[byte_pos] & (1 << bit_in_byte))
            c->count_one[bit]++;
    }
    c->total_hashes++;
    c->total_bits += BITS_PER_HASH;
}

/* ======================== MAIN ======================== */
int main(void) {

    BiasCounters counters;
    init_counters(&counters);

    uint64_t counter[5] = {0};
    uint8_t hash[BYTES_PER_HASH];

    printf("===== Bias Analysis =====\n");
    printf("Total hashes: %llu\n\n", TOTAL_HASHES);

    for (uint64_t i = 0; i < TOTAL_HASHES; i++) {
        xzalgochain((const uint8_t*)counter, sizeof(counter), hash);
        update_counters(&counters, hash);

        for (int j = 0; j < 5; j++) {
            counter[j]++;
            if (counter[j] != 0) break;
        }
    }

    double p_monobit = monobit_test(&counters);
    double p_chi    = chi_square_test(&counters);

    printf("Frequency (Monobit) Test\n");
    printf("p-value: %.10f => %s\n\n", p_monobit, p_monobit >= ALPHA ? "PASS" : "FAIL");

    printf("Per-bit Chi-Square Test\n");
    printf("p-value: %.10f => %s\n\n", p_chi, p_chi >= ALPHA ? "PASS" : "FAIL");

    /* ================== Per-bit detailed report ================== */
    printf("Per-bit statistics:\n");
    printf("Bit\tCount1\tPercentage\tDeviation\n");

    for (int i = 0; i < BITS_PER_HASH; i++) {
        double pct = (counters.count_one[i] * 100.0) / counters.total_hashes;
        printf("%d\t%llu\t%.6f\t%.6f\n",
               i,
               (unsigned long long)counters.count_one[i],
               pct,
               pct - 50.0);
    }

    return 0;
}
