#include "async.h"
#include "benchmark.h"
#include "go.h"
#include "types.h"

#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// Global variables for our demo problems
static u32 io_task_count = 16;
static u32 compute_task_count = 4;
static u64 prime_limit = 1000000;
static u32 memory_task_count = 8;

//
// IO-Heavy Problem: Simulated file downloads
//

static void simulate_download_go(void) {
    static u32 download_id = 1;
    u32 my_id = __atomic_fetch_add(&download_id, 1, __ATOMIC_SEQ_CST);

    printf("  [GO] Download %u: Starting...\n", my_id);
    usleep(500000); // 500ms simulated network delay
    printf("  [GO] Download %u: Complete!\n", my_id);
}

static void simulate_download_async(void) {
    static u32 download_id = 1;
    u32 my_id = download_id++;

    printf("  [ASYNC] Download %u: Starting...\n", my_id);

    // Simulate network delay with cooperative yielding (no blocking syscalls)
    // This demonstrates async's strength: handling many concurrent I/O operations
    for (u32 i = 0; i < 50; i++) {
        // Simulate processing chunks of data as they arrive
        volatile u32 busy_work = 0;
        for (u32 j = 0; j < 100000; j++) {
            busy_work += j % 1000;
        }
        async_yield(); // Cooperatively yield between chunks
    }

    printf("  [ASYNC] Download %u: Complete!\n", my_id);
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

    printf("  [GO] Worker %u: Checking primes from %" PRIu64 " to %" PRIu64 "...\n", my_id, start, end);

    for (u64 n = start; n < end; n++) {
        if (is_prime(n)) count++;
    }

    printf("  [GO] Worker %u: Found %u primes\n", my_id, count);
}

static void count_primes_async(void) {
    static u32 worker_id = 1;
    u32 my_id = worker_id++;

    u64 start = (my_id - 1) * (prime_limit / compute_task_count);
    u64 end = my_id * (prime_limit / compute_task_count);
    u32 count = 0;

    printf("  [ASYNC] Worker %u: Checking primes from %" PRIu64 " to %" PRIu64 "...\n", my_id, start, end);

    for (u64 n = start; n < end; n++) {
        if (is_prime(n)) count++;

        // Async is poor for CPU-bound work - frequent yielding hurts performance
        // This demonstrates that cooperative multitasking isn't ideal for compute-heavy tasks
        if (n % 100 == 0) {
            async_yield();
        }
    }

    printf("  [ASYNC] Worker %u: Found %u primes\n", my_id, count);
}

//
// Memory-Intensive Problem: Frequent allocation and processing
//

static void memory_intensive_go(void) {
    static u32 worker_id = 1;
    u32 my_id = __atomic_fetch_add(&worker_id, 1, __ATOMIC_SEQ_CST);

    printf("  [GO] Memory worker %u: Starting allocation/processing...\n", my_id);

    u64 total_sum = 0;
    for (u32 iteration = 0; iteration < 1000; iteration++) {
        // Allocate a moderately large chunk
        u32 *buffer = malloc(sizeof(u32) * 10000);
        if (!buffer) continue;

        // Fill with data and process
        for (u32 i = 0; i < 10000; i++) {
            buffer[i] = (my_id * 1000 + iteration) % 10000;
            total_sum += buffer[i];
        }

        // Process the data (simulate work)
        volatile u64 checksum = 0;
        for (u32 i = 0; i < 10000; i++) {
            checksum ^= buffer[i];
        }

        free(buffer);
    }

    printf("  [GO] Memory worker %u: Processed %" PRIu64 " total\n", my_id, total_sum);
}

static void memory_intensive_async(void) {
    static u32 worker_id = 1;
    u32 my_id = worker_id++;

    printf("  [ASYNC] Memory worker %u: Starting allocation/processing...\n", my_id);

    u64 total_sum = 0;
    for (u32 iteration = 0; iteration < 1000; iteration++) {
        // Allocate a moderately large chunk
        u32 *buffer = malloc(sizeof(u32) * 10000);
        if (!buffer) continue;

        // Fill with data and process
        for (u32 i = 0; i < 10000; i++) {
            buffer[i] = (my_id * 1000 + iteration) % 10000;
            total_sum += buffer[i];
        }

        // Process the data (simulate work)
        volatile u64 checksum = 0;
        for (u32 i = 0; i < 10000; i++) {
            checksum ^= buffer[i];
        }

        free(buffer);

        // Yield after each allocation/processing cycle to be cooperative
        if (iteration % 10 == 0) {
            async_yield();
        }
    }

    printf("  [ASYNC] Memory worker %u: Processed %" PRIu64 " total\n", my_id, total_sum);
}

//
// Test runners
//

static void test_io_heavy_go(void) {
    printf("\n=== IO-Heavy Test: GO (pthread-based) ===\n");
    printf("Running %u simulated downloads...\n", io_task_count);

    for (u32 i = 0; i < io_task_count; i++) {
        spawn(simulate_download_go);
    }
    wait();

    printf("All downloads completed!\n");
}

static void test_io_heavy_async(void) {
    printf("\n=== IO-Heavy Test: ASYNC (cooperative) ===\n");
    printf("Running %u simulated downloads...\n", io_task_count);

    for (u32 i = 0; i < io_task_count; i++) {
        async_spawn(simulate_download_async);
    }
    async_run_all();

    printf("All downloads completed!\n");
}

static void test_compute_heavy_go(void) {
    printf("\n=== Compute-Heavy Test: GO (pthread-based) ===\n");
    printf("Finding primes up to %" PRIu64 " using %u workers...\n", prime_limit, compute_task_count);

    for (u32 i = 0; i < compute_task_count; i++) {
        spawn(count_primes_go);
    }
    wait();

    printf("Prime calculation completed!\n");
}

static void test_compute_heavy_async(void) {
    printf("\n=== Compute-Heavy Test: ASYNC (cooperative) ===\n");
    printf("Finding primes up to %" PRIu64 " using %u workers...\n", prime_limit, compute_task_count);

    for (u32 i = 0; i < compute_task_count; i++) {
        async_spawn(count_primes_async);
    }
    async_run_all();

    printf("Prime calculation completed!\n");
}

static void test_memory_heavy_go(void) {
    printf("\n=== Memory-Intensive Test: GO (pthread-based) ===\n");
    printf("Running %u memory allocation workers...\n", memory_task_count);

    for (u32 i = 0; i < memory_task_count; i++) {
        spawn(memory_intensive_go);
    }
    wait();

    printf("Memory processing completed!\n");
}

static void test_memory_heavy_async(void) {
    printf("\n=== Memory-Intensive Test: ASYNC (cooperative) ===\n");
    printf("Running %u memory allocation workers...\n", memory_task_count);

    for (u32 i = 0; i < memory_task_count; i++) {
        async_spawn(memory_intensive_async);
    }
    async_run_all();

    printf("Memory processing completed!\n");
}

int main(void) {
    printf("=== Parallelism Paradigm Comparison ===\n");
    printf("This demo shows the strengths and weaknesses of different approaches:\n");
    printf("• GO (pthread-based): True parallelism, good for CPU-bound and I/O-bound work\n");
    printf("• ASYNC (cooperative): Single-threaded, excellent for I/O-bound, poor for CPU-bound\n\n");

    benchmark("IO-Heavy GO", {
        test_io_heavy_go();
    });

    benchmark("IO-Heavy ASYNC", {
        test_io_heavy_async();
    });

    benchmark("Compute-Heavy GO", {
        test_compute_heavy_go();
    });

    benchmark("Compute-Heavy ASYNC", {
        test_compute_heavy_async();
    });

    benchmark("Memory-Heavy GO", {
        test_memory_heavy_go();
    });

    benchmark("Memory-Heavy ASYNC", {
        test_memory_heavy_async();
    });

    printf("\n=== Summary ===\n");
    printf("Expected results:\n");
    printf("• IO-Heavy: ASYNC should perform much better due to cooperative yielding\n");
    printf("• Compute-Heavy: GO should perform better due to true parallelism\n");
    printf("• Memory-Heavy: GO should perform better due to parallel memory access\n");

    return EXIT_SUCCESS;
}
