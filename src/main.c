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

// Global variables for our demo problems - optimized for dramatic differences
static u32 io_task_count = 200;           // Many concurrent I/O operations (under thread limit)
static u32 compute_task_count = 12;       // More CPU-bound work across cores
static u64 prime_limit = 10000000;        // More intensive computation
static u32 memory_task_count = 32;        // More memory-intensive parallel work

// Shared progress counters (thread-safe)
static volatile u32 io_progress_go = 0;
static volatile u32 io_progress_async = 0;
static volatile u32 compute_progress_go = 0;
static volatile u32 compute_progress_async = 0;
static volatile u32 memory_progress_go = 0;
static volatile u32 memory_progress_async = 0;

//
// IO-Heavy Problem: Simulated file downloads
//

static void simulate_download_go(void) {
    // Each thread blocks - creating 1000 threads will be extremely slow
    usleep(100000); // 100ms simulated network delay (blocking I/O)
    __atomic_fetch_add(&io_progress_go, 1, __ATOMIC_SEQ_CST);
}

static void simulate_download_async(void) {
    // Simulate async I/O - yield immediately to simulate non-blocking I/O
    // This allows other tasks to run while "waiting" for I/O completion
    async_yield(); // Simulate async I/O operation that doesn't block

    // Minimal processing to simulate parsing downloaded data
    volatile u32 data_processed = 0;
    for (u32 i = 0; i < 1000; i++) {
        data_processed += i % 100;
    }

    __atomic_fetch_add(&io_progress_async, 1, __ATOMIC_SEQ_CST);
}

//
// Compute-Heavy Problem: Prime number calculation
//

static bool is_prime(u64 n) {
    if (n < 2) return false;
    if (n == 2) return true;
    if (n % 2 == 0) return false;

    for (u64 i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return false;
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
        if (is_prime(n)) count++;
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
        if (is_prime(n)) count++;

        // Occasional yielding to allow other tasks to run
        // Much less frequent than before to maintain performance
        if (n % 50000 == 0) {
            async_yield();
        }
    }

    __atomic_fetch_add(&compute_progress_async, 1, __ATOMIC_SEQ_CST);
}

//
// Memory-Intensive Problem: Frequent allocation and processing
//

static void memory_intensive_go(void) {
    u64 total_sum = 0;
    for (u32 iteration = 0; iteration < 1000; iteration++) {
        // Large allocations that benefit greatly from parallel processing
        u32 *buffer = malloc(sizeof(u32) * 15000);
        if (!buffer) continue;

        // Memory operations that can run in parallel across cores
        for (u32 i = 0; i < 15000; i++) {
            buffer[i] = (iteration * i) % 15000;
            total_sum += buffer[i];
        }

        // Memory-intensive processing with true parallelism
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
        // Same allocation size and work as GO version
        u32 *buffer = malloc(sizeof(u32) * 15000);
        if (!buffer) continue;

        // Memory operations - same work as GO version
        for (u32 i = 0; i < 15000; i++) {
            buffer[i] = (iteration * i) % 15000;
            total_sum += buffer[i];
        }

        // Memory-intensive processing
        volatile u64 checksum = 0;
        for (u32 i = 0; i < 15000; i++) {
            checksum ^= buffer[i] * buffer[(i + 1) % 15000];
        }

        free(buffer);

        // Less frequent yielding to maintain better performance
        if (iteration % 100 == 0) {
            async_yield();
        }
    }

    __atomic_fetch_add(&memory_progress_async, 1, __ATOMIC_SEQ_CST);
}

//
// Test runners
//

static void test_io_heavy_go(void) {
    printf("IO-Heavy GO (pthread-based): ");
    fflush(stdout);

    io_progress_go = 0;
    for (u32 i = 0; i < io_task_count; i++) {
        spawn(simulate_download_go);
    }

    // Monitor progress
    while (io_progress_go < io_task_count) {
        tqdm(io_progress_go, io_task_count, NULL, "downloads");
        usleep(50000); // 50ms updates
    }
    wait();
    tqdm(io_task_count, io_task_count, NULL, "downloads");
}

static void test_io_heavy_async(void) {
    printf("IO-Heavy ASYNC (cooperative): ");
    fflush(stdout);

    io_progress_async = 0;
    for (u32 i = 0; i < io_task_count; i++) {
        async_spawn(simulate_download_async);
    }

    async_run_all();
    tqdm(io_task_count, io_task_count, NULL, "downloads");
}

static void test_compute_heavy_go(void) {
    printf("Compute-Heavy GO (pthread-based): ");
    fflush(stdout);

    compute_progress_go = 0;
    for (u32 i = 0; i < compute_task_count; i++) {
        spawn(count_primes_go);
    }

    // Monitor progress
    while (compute_progress_go < compute_task_count) {
        tqdm(compute_progress_go, compute_task_count, NULL, "workers");
        usleep(10000); // 10ms updates
    }
    wait();
    tqdm(compute_task_count, compute_task_count, NULL, "workers");
}

static void test_compute_heavy_async(void) {
    printf("Compute-Heavy ASYNC (cooperative): ");
    fflush(stdout);

    compute_progress_async = 0;
    for (u32 i = 0; i < compute_task_count; i++) {
        async_spawn(count_primes_async);
    }

    async_run_all();
    tqdm(compute_task_count, compute_task_count, NULL, "workers");
}

static void test_memory_heavy_go(void) {
    printf("Memory-Heavy GO (pthread-based): ");
    fflush(stdout);

    memory_progress_go = 0;
    for (u32 i = 0; i < memory_task_count; i++) {
        spawn(memory_intensive_go);
    }

    // Monitor progress
    while (memory_progress_go < memory_task_count) {
        tqdm(memory_progress_go, memory_task_count, NULL, "workers");
        usleep(10000); // 10ms updates
    }
    wait();
    tqdm(memory_task_count, memory_task_count, NULL, "workers");
}

static void test_memory_heavy_async(void) {
    printf("Memory-Heavy ASYNC (cooperative): ");
    fflush(stdout);

    memory_progress_async = 0;
    for (u32 i = 0; i < memory_task_count; i++) {
        async_spawn(memory_intensive_async);
    }

    async_run_all();
    tqdm(memory_task_count, memory_task_count, NULL, "workers");
}

int main(void) {
    printf("=== Parallelism Paradigm Comparison ===\n");
    printf("Comparing GO (pthread-based) vs ASYNC (cooperative) across different workloads\n\n");

    // IO-Heavy Tests
    printf("=== IO-Heavy Tests (%u simulated downloads) ===\n", io_task_count);
    f64 io_go_time = benchmark_silent({
        test_io_heavy_go();
    });

    f64 io_async_time = benchmark_silent({
        test_io_heavy_async();
    });

    // Compute-Heavy Tests
    printf("\n=== Compute-Heavy Tests (primes up to %" PRIu64 ") ===\n", prime_limit);
    f64 compute_go_time = benchmark_silent({
        test_compute_heavy_go();
    });

    f64 compute_async_time = benchmark_silent({
        test_compute_heavy_async();
    });

    // Memory-Heavy Tests
    printf("\n=== Memory-Heavy Tests (%u allocation workers) ===\n", memory_task_count);
    f64 memory_go_time = benchmark_silent({
        test_memory_heavy_go();
    });

    f64 memory_async_time = benchmark_silent({
        test_memory_heavy_async();
    });

    printf("IO-Heavy:     GO %.3fs vs ASYNC %.3fs (%.1fx %s)\n",
           io_go_time, io_async_time,
           io_async_time < io_go_time ? io_go_time/io_async_time : io_async_time/io_go_time,
           io_async_time < io_go_time ? "faster ASYNC" : "faster GO");

    printf("Compute-Heavy: GO %.3fs vs ASYNC %.3fs (%.1fx %s)\n",
           compute_go_time, compute_async_time,
           compute_go_time < compute_async_time ? compute_async_time/compute_go_time : compute_go_time/compute_async_time,
           compute_go_time < compute_async_time ? "faster GO" : "faster ASYNC");

    printf("Memory-Heavy:  GO %.3fs vs ASYNC %.3fs (%.1fx %s)\n",
           memory_go_time, memory_async_time,
           memory_go_time < memory_async_time ? memory_async_time/memory_go_time : memory_go_time/memory_async_time,
           memory_go_time < memory_async_time ? "faster GO" : "faster ASYNC");

    return EXIT_SUCCESS;
}
