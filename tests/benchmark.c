/*
 * benchmark.c
 *
 * Hashing benchmark for XzalgoChain
 *
 * Measures:
 *   - real time  (wall clock)
 *   - user time
 *   - sys time
 *   - speed (hash/sec or MB/sec)
 *
 * Compile:
 * clang -O3 -march=native -mtune=native -flto=full -fopenmp -lm -o benchmark benchmark.c
 *
 * Author: Xzrayツ
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/resource.h>

#include "../XzalgoChain/XzalgoChain.h"

#define INPUT_BYTES 64
#define HASH_BYTES  40

typedef struct {
    const char *label;
    double real;
    double user;
    double sys;
    double speed;
} result_t;

/* ===================== Time Utils ===================== */

static double diff_timespec(const struct timespec *a,
                            const struct timespec *b) {
    double s  = (double)(b->tv_sec  - a->tv_sec);
    double ns = (double)(b->tv_nsec - a->tv_nsec);
    return s + ns * 1e-9;
}

static double diff_timeval(const struct timeval *a,
                           const struct timeval *b) {
    double s  = (double)(b->tv_sec  - a->tv_sec);
    double us = (double)(b->tv_usec - a->tv_usec);
    return s + us * 1e-6;
}

/* ===================== Hash Count Benchmark ===================== */

static void bench_hash_count(uint64_t iterations, result_t *r) {

    uint8_t input[INPUT_BYTES];
    uint8_t output[HASH_BYTES];

    memset(input, 0xA5, sizeof(input));

    struct timespec rs, re;
    struct rusage us, ue;

    getrusage(RUSAGE_SELF, &us);
    clock_gettime(CLOCK_MONOTONIC, &rs);

    for (uint64_t i = 0; i < iterations; ++i) {
        xzalgochain(input, INPUT_BYTES, output);
    }

    clock_gettime(CLOCK_MONOTONIC, &re);
    getrusage(RUSAGE_SELF, &ue);

    r->real = diff_timespec(&rs, &re);
    r->user = diff_timeval(&us.ru_utime, &ue.ru_utime);
    r->sys  = diff_timeval(&us.ru_stime, &ue.ru_stime);
    r->speed = iterations / r->real;
}

/* ===================== Streaming Benchmark ===================== */

static void bench_stream(size_t mb, result_t *r) {

    size_t bytes = mb * 1024ULL * 1024ULL;
    uint8_t *buffer = malloc(bytes);
    uint8_t output[HASH_BYTES];

    if (!buffer) {
        fprintf(stderr, "Allocation failed for %zu MB\n", mb);
        exit(EXIT_FAILURE);
    }

    memset(buffer, 0x5C, bytes);

    struct timespec rs, re;
    struct rusage us, ue;

    getrusage(RUSAGE_SELF, &us);
    clock_gettime(CLOCK_MONOTONIC, &rs);

    xzalgochain(buffer, bytes, output);

    clock_gettime(CLOCK_MONOTONIC, &re);
    getrusage(RUSAGE_SELF, &ue);

    r->real = diff_timespec(&rs, &re);
    r->user = diff_timeval(&us.ru_utime, &ue.ru_utime);
    r->sys  = diff_timeval(&us.ru_stime, &ue.ru_stime);
    r->speed = (double)mb / r->real;

    free(buffer);
}

/* ===================== Print ===================== */

static void print_result(const result_t *r, const char *unit) {

    printf("%-12s | real: %10.6f s | user: %10.6f s | sys: %10.6f s | speed: %12.2f %s\n",
           r->label,
           r->real,
           r->user,
           r->sys,
           r->speed,
           unit);
}

/* ===================== Main ===================== */

int main(void) {

    printf("===== XzalgoChain Benchmark =====\n");
    printf("Small input: %d bytes\n\n", INPUT_BYTES);

    uint64_t hash_tests[] = {
        1ULL,
        10ULL,
        100ULL,
        1000ULL,
        10000ULL,
        100000ULL,
        1000000ULL,
        10000000ULL
    };

    size_t mb_tests[] = {
        1ULL,
        10ULL,
        100ULL,
        1000ULL
    };

    result_t results[32];
    size_t idx = 0;

    /* ---------------- Hash Count ---------------- */

    printf("---- Hash Count Benchmark ----\n");

    for (size_t i = 0; i < sizeof(hash_tests)/sizeof(hash_tests[0]); ++i) {

        char *label = malloc(32);
        snprintf(label, 32, "%llu hash",
                 (unsigned long long)hash_tests[i]);

        results[idx].label = label;
        bench_hash_count(hash_tests[i], &results[idx]);

        print_result(&results[idx], "hash/sec");
        idx++;
    }

    /* ---------------- Streaming ---------------- */

    printf("\n---- Streaming Benchmark ----\n");

    for (size_t i = 0; i < sizeof(mb_tests)/sizeof(mb_tests[0]); ++i) {

        char *label = malloc(32);
        snprintf(label, 32, "%zu MB", mb_tests[i]);

        results[idx].label = label;
        bench_stream(mb_tests[i], &results[idx]);

        print_result(&results[idx], "MB/sec");
        idx++;
    }

    /* ---------------- Summary ---------------- */

    printf("\n===== SUMMARY =====\n");

    for (size_t i = 0; i < idx; ++i) {

        if (strstr(results[i].label, "MB"))
            print_result(&results[i], "MB/sec");
        else
            print_result(&results[i], "hash/sec");
    }

    return 0;
}
