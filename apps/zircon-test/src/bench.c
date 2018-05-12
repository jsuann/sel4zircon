#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <zircon/types.h>
#include <zircon/syscalls.h>
//#include <sel4zircon/cspace.h>
//#include <sel4zircon/endpoint.h>

#define NUM_WARMUP          10u
#define NUM_RUNS            100u
#define NUM_ITERATIONS      1000u

#define RUN_BENCHMARK(...) \
    uint64_t mean = 0; \
    double stddev = 0; \
    uint64_t result[NUM_RUNS]; \
    for (size_t i = 0; i < NUM_RUNS; ++i) { \
        uint64_t time1, time2; \
        for (size_t j = 0; j < NUM_WARMUP; ++j) { \
            __VA_ARGS__; \
        } \
        time1 = zx_clock_get(ZX_CLOCK_MONOTONIC); \
        for (size_t j = 0; j < NUM_ITERATIONS; ++j) { \
            __VA_ARGS__; \
        } \
        time2 = zx_clock_get(ZX_CLOCK_MONOTONIC); \
        result[i] = (time2 - time1 - timer_overhead) / NUM_ITERATIONS; \
        mean += result[i]; \
    } \
    do { \
        mean /= NUM_RUNS; \
        for (size_t i = 0; i < NUM_RUNS; ++i) { \
            int64_t x = (result[i] - mean); \
            stddev += x * x; \
        } \
        stddev /= NUM_RUNS; \
        stddev = sqrt(stddev); \
    } while (0)


uint64_t timer_overhead;

void calc_timer_overhead(void)
{
    timer_overhead = 0;
    for (size_t i = 0; i < NUM_RUNS; ++i) {
        uint64_t time1, time2;
        for (uint64_t j = 0; j < NUM_WARMUP; ++j) {
            __asm__ volatile("");
        }
        time1 = zx_clock_get(ZX_CLOCK_MONOTONIC);
        for (uint64_t j = 0; j < NUM_ITERATIONS; ++j) {
            __asm__ volatile("");
        }
        time2 = zx_clock_get(ZX_CLOCK_MONOTONIC);
        timer_overhead += (time2 - time1);
    }

    timer_overhead /= NUM_RUNS;
    printf("Timer overhead: %lu\n", timer_overhead);
}

void bench_test0_syscall(void)
{
    RUN_BENCHMARK(zx_syscall_test_0());
    printf("test 0 syscall: mean %lu stddev %f\n", mean, stddev);
}

void bench_test8_syscall(void)
{
    RUN_BENCHMARK(zx_syscall_test_8(1,2,3,4,5,6,7,8));
    printf("test 8 syscall: mean %lu stddev %f\n", mean, stddev);
}

void bench_event_create(void)
{
    zx_handle_t handle;
    RUN_BENCHMARK(zx_event_create(0, &handle));
    printf("event create: mean %lu stddev %f\n", mean, stddev);
    /* Check oom */
    assert(!zx_event_create(0, &handle));
}
