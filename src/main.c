#include "async.h"
#include "benchmark.h"
#include "go.h"
#include "tqdm.h"
#include "types.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static u32 io_task_count = 200;
static u32 compute_task_count = 12;
static u64 prime_limit = 10000000;
static u32 memory_task_count = 32;

static volatile u32 io_progress_go = 0;
static volatile u32 io_progress_async = 0;
static volatile u32 compute_progress_go = 0;
static volatile u32 compute_progress_async = 0;
static volatile u32 memory_progress_go = 0;
static volatile u32 memory_progress_async = 0;

// 
// io-bound task: simulate downloading a file
// 

static void simulate_download_go(void) {
    usleep(100000);
    __atomic_fetch_add(&io_progress_go, 1, __ATOMIC_SEQ_CST);
}

static void simulate_download_async(void) {
    async_yield();

    volatile u32 data_processed = 0;
    for (u32 i = 0; i < 1000; i++) {
        data_processed += i % 100;
    }

    __atomic_fetch_add(&io_progress_async, 1, __ATOMIC_SEQ_CST);
}

// 
// compute-bound task: count primes up to a limit
// 

static bool is_prime(u64 n) {
    if (n < 2)
        return false;
    if (n == 2)
        return true;
    if (n % 2 == 0)
        return false;

    for (u64 i = 3; i * i <= n; i += 2) {
        if (n % i == 0)
            return false;
    }
    return true;
}

static void count_primes_go(void) {
    static u32 worker_id = 1;
    u32 my_id = __atomic_fetch_add(&worker_id, 1, __ATOMIC_SEQ_CST);

    u64 start = (my_id - 1) * (prime_limit / compute_task_count);
    u64 end = my_id * (prime_limit / compute_task_count);
    u32 count = 0;

    for (u64 n = start; n < end; n++) {
        if (is_prime(n))
            count++;
    }

    __atomic_fetch_add(&compute_progress_go, 1, __ATOMIC_SEQ_CST);
}

static void count_primes_async(void) {
    static u32 worker_id = 1;
    u32 my_id = __atomic_fetch_add(&worker_id, 1, __ATOMIC_SEQ_CST);

    u64 start = (my_id - 1) * (prime_limit / compute_task_count);
    u64 end = my_id * (prime_limit / compute_task_count);
    u32 count = 0;

    for (u64 n = start; n < end; n++) {
        if (is_prime(n))
            count++;

        if (n % 50000 == 0) {
            async_yield();
        }
    }

    __atomic_fetch_add(&compute_progress_async, 1, __ATOMIC_SEQ_CST);
}

// 
// memory-bound task: allocate and manipulate large arrays
// 

static void memory_intensive_go(void) {
    u64 total_sum = 0;
    for (u32 iteration = 0; iteration < 1000; iteration++) {
        u32 *buffer = malloc(sizeof(u32) * 15000);
        if (!buffer)
            continue;

        for (u32 i = 0; i < 15000; i++) {
            buffer[i] = (iteration * i) % 15000;
            total_sum += buffer[i];
        }

        volatile u64 checksum = 0;
        for (u32 i = 0; i < 15000; i++) {
            checksum ^= buffer[i] * buffer[(i + 1) % 15000];
        }

        free(buffer);
    }

    __atomic_fetch_add(&memory_progress_go, 1, __ATOMIC_SEQ_CST);
}

static void memory_intensive_async(void) {
    u64 total_sum = 0;
    for (u32 iteration = 0; iteration < 1000; iteration++) {
        u32 *buffer = malloc(sizeof(u32) * 15000);
        if (!buffer)
            continue;

        for (u32 i = 0; i < 15000; i++) {
            buffer[i] = (iteration * i) % 15000;
            total_sum += buffer[i];
        }

        volatile u64 checksum = 0;
        for (u32 i = 0; i < 15000; i++) {
            checksum ^= buffer[i] * buffer[(i + 1) % 15000];
        }

        free(buffer);

        if (iteration % 100 == 0) {
            async_yield();
        }
    }

    __atomic_fetch_add(&memory_progress_async, 1, __ATOMIC_SEQ_CST);
}

// 
// runners
// 

static void test_io_heavy_go(void) {
    io_progress_go = 0;
    for (u32 i = 0; i < io_task_count; i++) {
        spawn(simulate_download_go);
    }

    while (io_progress_go < io_task_count) {
        tqdm(io_progress_go, io_task_count, NULL, "downloads");
        usleep(50000);
    }
    wait();
    tqdm(io_task_count, io_task_count, NULL, "downloads");
}

static void test_io_heavy_async(void) {
    io_progress_async = 0;
    for (u32 i = 0; i < io_task_count; i++) {
        async_spawn(simulate_download_async);
    }

    async_run_all();
    tqdm(io_task_count, io_task_count, NULL, "downloads");
}

static void test_compute_heavy_go(void) {
    compute_progress_go = 0;
    for (u32 i = 0; i < compute_task_count; i++) {
        spawn(count_primes_go);
    }

    while (compute_progress_go < compute_task_count) {
        tqdm(compute_progress_go, compute_task_count, NULL, "workers");
        usleep(10000);
    }
    wait();
    tqdm(compute_task_count, compute_task_count, NULL, "workers");
}

static void test_compute_heavy_async(void) {
    compute_progress_async = 0;
    for (u32 i = 0; i < compute_task_count; i++) {
        async_spawn(count_primes_async);
    }

    async_run_all();
    tqdm(compute_task_count, compute_task_count, NULL, "workers");
}

static void test_memory_heavy_go(void) {
    memory_progress_go = 0;
    for (u32 i = 0; i < memory_task_count; i++) {
        spawn(memory_intensive_go);
    }

    while (memory_progress_go < memory_task_count) {
        tqdm(memory_progress_go, memory_task_count, NULL, "workers");
        usleep(10000);
    }
    wait();
    tqdm(memory_task_count, memory_task_count, NULL, "workers");
}

static void test_memory_heavy_async(void) {
    memory_progress_async = 0;
    for (u32 i = 0; i < memory_task_count; i++) {
        async_spawn(memory_intensive_async);
    }

    async_run_all();
    tqdm(memory_task_count, memory_task_count, NULL, "workers");
}

int main(void) {
    f64 io_go_time = benchmark_silent({ test_io_heavy_go(); });
    f64 io_async_time = benchmark_silent({ test_io_heavy_async(); });
    f64 compute_go_time = benchmark_silent({ test_compute_heavy_go(); });
    f64 compute_async_time = benchmark_silent({ test_compute_heavy_async(); });
    f64 memory_go_time = benchmark_silent({ test_memory_heavy_go(); });
    f64 memory_async_time = benchmark_silent({ test_memory_heavy_async(); });

    printf("\n\nIO-Bound:\t GO in %.3fs vs ASYNC in %.3fs (%.1fx %s)\n", io_go_time, io_async_time, io_async_time < io_go_time ? io_go_time / io_async_time : io_async_time / io_go_time, io_async_time < io_go_time ? "faster ASYNC" : "faster GO");

    printf("Compute-Bound:\t GO in %.3fs vs ASYNC in %.3fs (%.1fx %s)\n", compute_go_time, compute_async_time, compute_go_time < compute_async_time ? compute_async_time / compute_go_time : compute_go_time / compute_async_time, compute_go_time < compute_async_time ? "faster GO" : "faster ASYNC");

    printf("Memory-Bound:\t GO in %.3fs vs ASYNC in %.3fs (%.1fx %s)\n", memory_go_time, memory_async_time, memory_go_time < memory_async_time ? memory_async_time / memory_go_time : memory_go_time / memory_async_time, memory_go_time < memory_async_time ? "faster GO" : "faster ASYNC");

    return EXIT_SUCCESS;
}
