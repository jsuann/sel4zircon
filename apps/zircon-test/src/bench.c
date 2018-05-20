#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <zircon/types.h>
#include <zircon/syscalls.h>
#include <zircon/process.h>

#include <mini-process/mini-process.h>

#define NUM_WARMUP  10u
#define NUM_RUNS    10u
#define NUM_ITER    100u

#define MAX_BUF_SIZE    65536u

#define VMO_SIZE        16777216u

/* Array for storing bench results */
double result1[NUM_RUNS];
double result2[NUM_RUNS];

double mean, stddev, overhead;

void calc_results(const char *test, double *result)
{
    mean = stddev = 0;
    for (size_t i = 0; i < NUM_RUNS; ++i) {
        mean += result[i];
    }
    mean /= NUM_RUNS;
    for (size_t i = 0; i < NUM_RUNS; ++i) {
        double x = (result[i] - mean);
        stddev += x * x;
    }
    stddev /= NUM_RUNS;
    stddev = sqrt(stddev);
    printf("=== %s results ===\n", test);
    printf("mean: %f\n", mean);
    printf("stddev: %f\n", stddev);
    printf("stddev/mean: %f \n", (stddev / mean));
}

void rand_write_buf(char *buf, size_t numbytes)
{
    for (size_t j = 0; j < numbytes; j += sizeof(int)) {
        int *val = (int *)&buf[j];
        *val = rand();
    }
}

void run_benchmarks(void)
{
    uint64_t start, end;
    srand(6123129);

    /* Calc loop overhead */
    for (size_t i = 0; i < NUM_RUNS; ++i) {
        for (size_t j = 0; j < NUM_WARMUP; ++j) {
            __asm__ volatile("");
        }
        start = zx_ticks_get();
        for (size_t j = 0; j < NUM_ITER; ++j) {
            __asm__ volatile("");
        }
        end = zx_ticks_get();
        result1[i] = (end - start);
    }
    calc_results("empty loop overhead", result1);

    /* Save the mean overhead */
    overhead = mean;

    /*
     *  Test syscall bench
     */
    for (size_t i = 0; i < NUM_RUNS; ++i) {
        for (size_t j = 0; j < NUM_WARMUP; ++j) {
            zx_syscall_test_0();
        }
        start = zx_ticks_get();
        for (size_t j = 0; j < NUM_ITER; ++j) {
            zx_syscall_test_0();
        }
        end = zx_ticks_get();
        result1[i] = (end - start - overhead) / NUM_ITER;
    }
    calc_results("zx_syscall_test_0", result1);

    for (size_t i = 0; i < NUM_RUNS; ++i) {
        for (size_t j = 0; j < NUM_WARMUP; ++j) {
            zx_syscall_test_8(1,2,3,4,5,6,7,8);
        }
        start = zx_ticks_get();
        for (size_t j = 0; j < NUM_ITER; ++j) {
            zx_syscall_test_8(1,2,3,4,5,6,7,8);
        }
        end = zx_ticks_get();
        result1[i] = (end - start - overhead) / NUM_ITER;
    }
    calc_results("zx_syscall_test_8", result1);

    /*
     *  Handle dup, replace, close bench
     */
    zx_handle_t event;
    assert(!zx_event_create(0, &event));
    zx_handle_t *dup = malloc(NUM_ITER * sizeof(zx_handle_t));
    assert(dup != NULL);

    for (size_t i = 0; i < NUM_RUNS; ++i) {
        for (size_t j = 0; j < NUM_WARMUP; ++j) {
            assert(!zx_handle_replace(event, ZX_RIGHT_SAME_RIGHTS, &event));
        }
        start = zx_ticks_get();
        for (size_t j = 0; j < NUM_ITER; ++j) {
            zx_handle_replace(event, ZX_RIGHT_SAME_RIGHTS, &event);
        }
        end = zx_ticks_get();
        result1[i] = (end - start - overhead) / NUM_ITER;
    }
    calc_results("zx_handle_replace", result1);

    for (size_t i = 0; i < NUM_RUNS; ++i) {
        for (size_t j = 0; j < NUM_ITER; ++j) {
            assert(!zx_handle_duplicate(event, ZX_RIGHT_SAME_RIGHTS, &dup[i]));
            assert(!zx_handle_close(dup[i]));
        }

        start = zx_ticks_get();
        for (size_t j = 0; j < NUM_ITER; ++j) {
            zx_handle_duplicate(event, ZX_RIGHT_SAME_RIGHTS, &dup[i]);
        }
        end = zx_ticks_get();
        result1[i] = (end - start - overhead) / NUM_ITER;

        start = zx_ticks_get();
        for (size_t j = 0; j < NUM_ITER; ++j) {
           zx_handle_close(dup[i]);
        }
        end = zx_ticks_get();
        result2[i] = (end - start - overhead) / NUM_ITER;
    }
    calc_results("zx_handle_duplicate", result1);
    calc_results("zx_handle_close", result2);

    free(dup);

    /*
     *  Event signal bench
     */
    for (size_t i = 0; i < NUM_RUNS; ++i) {
        for (size_t j = 0; j < NUM_WARMUP; ++j) {
            assert(!zx_object_signal(event, 0, ZX_EVENT_SIGNALED));
        }
        start = zx_ticks_get();
        for (size_t j = 0; j < NUM_ITER; ++j) {
            zx_object_signal(event, 0, ZX_EVENT_SIGNALED);
        }
        end = zx_ticks_get();
        result1[i] = (end - start - overhead) / NUM_ITER;
    }
    calc_results("zx_object_signal", result1);

    assert(!zx_handle_close(event));


    /*
     *  Channel/socket read/write bench
     */
    char *writebuf = malloc(MAX_BUF_SIZE);
    char *readbuf = malloc(MAX_BUF_SIZE);
    assert(writebuf != NULL && readbuf != NULL);

    zx_handle_t ch0, ch1;
    assert(!zx_channel_create(0, &ch0, &ch1));

    zx_handle_t sock0, sock1;
    assert(!zx_socket_create(ZX_SOCKET_STREAM, &sock0, &sock1));

    size_t numbytes = 64;
    char str_buf[100];

    while (numbytes <= MAX_BUF_SIZE) {
        for (size_t i = 0; i < NUM_RUNS; ++i) {
            rand_write_buf(writebuf, numbytes);
            /* Do untimed test run with sanity checks */
            for (size_t j = 0; j < NUM_ITER; ++j) {
                assert(!zx_channel_write(ch0, 0, writebuf, numbytes, NULL, 0));
            }
            for (size_t j = 0; j < NUM_ITER; ++j) {
                assert(!zx_channel_read(ch1, 0, readbuf, NULL, numbytes, 0, NULL, NULL));
                assert(!memcmp(writebuf, readbuf, numbytes));
            }
            /* Now time it */
            start = zx_ticks_get();
            for (size_t j = 0; j < NUM_ITER; ++j) {
                zx_channel_write(ch0, 0, writebuf, numbytes, NULL, 0);
            }
            end = zx_ticks_get();
            result1[i] = (end - start - overhead) / NUM_ITER;
            start = zx_ticks_get();
            for (size_t j = 0; j < NUM_ITER; ++j) {
                zx_channel_read(ch1, 0, readbuf, NULL, numbytes, 0, NULL, NULL);
            }
            end = zx_ticks_get();
            assert(!memcmp(writebuf, readbuf, numbytes));
            result2[i] = (end - start - overhead) / NUM_ITER;
        }
        sprintf(str_buf, "channel write %lu", numbytes);
        calc_results(str_buf, result1);
        sprintf(str_buf, "channel read %lu", numbytes);
        calc_results(str_buf, result2);

        for (size_t i = 0; i < NUM_RUNS; ++i) {
            rand_write_buf(writebuf, numbytes);
            /* Do untimed test run with sanity checks */
            for (size_t j = 0; j < NUM_ITER; ++j) {
                assert(!zx_socket_write(sock0, 0, writebuf, numbytes, NULL));
            }
            for (size_t j = 0; j < NUM_ITER; ++j) {
                assert(!zx_socket_read(sock1, 0, readbuf, numbytes, NULL));
                assert(!memcmp(writebuf, readbuf, numbytes));
            }
            /* Now time it */
            start = zx_ticks_get();
            for (size_t j = 0; j < NUM_ITER; ++j) {
                zx_socket_write(sock0, 0, writebuf, numbytes, NULL);
            }
            end = zx_ticks_get();
            result1[i] = (end - start - overhead) / NUM_ITER;
            start = zx_ticks_get();
            for (size_t j = 0; j < NUM_ITER; ++j) {
                zx_socket_read(sock1, 0, readbuf, numbytes, NULL);
            }
            end = zx_ticks_get();
            assert(!memcmp(writebuf, readbuf, numbytes));
            result2[i] = (end - start - overhead) / NUM_ITER;
        }
        sprintf(str_buf, "socket write %lu", numbytes);
        calc_results(str_buf, result1);
        sprintf(str_buf, "socket read %lu", numbytes);
        calc_results(str_buf, result2);

        numbytes *= 4;
    }

    /* Close channels & sockets */
    assert(!zx_handle_close(ch0));
    assert(!zx_handle_close(ch1));
    assert(!zx_handle_close(sock0));
    assert(!zx_handle_close(sock1));
    free(writebuf);
    free(readbuf);

    /*
     *  Thread create
     */
    zx_handle_t *thrd = malloc(NUM_ITER * sizeof(zx_handle_t));
    assert(thrd != NULL);
    for (size_t i = 0; i < NUM_RUNS; ++i) {
        for (size_t j = 0; j < NUM_ITER; ++j) {
            assert(!zx_thread_create(zx_process_self(), "thrd", 4, 0, &thrd[i]));
            assert(!zx_handle_close(thrd[i]));
        }

        start = zx_ticks_get();
        for (size_t j = 0; j < NUM_ITER; ++j) {
            zx_thread_create(zx_process_self(), "thrd", 4, 0, &thrd[i]);
        }
        end = zx_ticks_get();
        result1[i] = (end - start - overhead) / NUM_ITER;

        for (size_t j = 0; j < NUM_ITER; ++j) {
           zx_handle_close(thrd[i]);
        }
    }
    calc_results("zx_thread_create", result1);


    while (1);
}
