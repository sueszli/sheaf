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
static u32 io_task_count = 8;
static u32 compute_task_count = 4;
static u64 prime_limit = 50000;

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

    // Simulate network delay with yielding
    for (u32 i = 0; i < 10; i++) {
        usleep(50000); // 50ms chunks
        async_yield(); // Let other tasks run
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

        // Yield occasionally to be cooperative
        if (n % 1000 == 0) {
            async_yield();
        }
    }

    printf("  [ASYNC] Worker %u: Found %u primes\n", my_id, count);
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
    printf("Finding primes up to %llu using %u workers...\n", prime_limit, compute_task_count);

    for (u32 i = 0; i < compute_task_count; i++) {
        spawn(count_primes_go);
    }
    wait();

    printf("Prime calculation completed!\n");
}

static void test_compute_heavy_async(void) {
    printf("\n=== Compute-Heavy Test: ASYNC (cooperative) ===\n");
    printf("Finding primes up to %llu using %u workers...\n", prime_limit, compute_task_count);

    for (u32 i = 0; i < compute_task_count; i++) {
        async_spawn(count_primes_async);
    }
    async_run_all();

    printf("Prime calculation completed!\n");
}

int main(void) {
    printf("=================================================================\n");
    printf("Concurrency Demo: GO vs ASYNC\n");
    printf("=================================================================\n");
    printf("This demo showcases when to use each concurrency model:\n");
    printf("\n");
    printf("GO (pthread-based):\n");
    printf("  + True parallelism across CPU cores\n");
    printf("  + Excellent for compute-heavy tasks\n");
    printf("  - Higher memory overhead per thread\n");
    printf("  - OS thread creation/switching cost\n");
    printf("\n");
    printf("ASYNC (cooperative):\n");
    printf("  + Low memory overhead (user-space threads)\n");
    printf("  + Excellent for IO-heavy tasks with yielding\n");
    printf("  + Fast context switching\n");
    printf("  - No true parallelism (single CPU core)\n");
    printf("  - Requires manual yielding for cooperation\n");
    printf("=================================================================\n");

    // Test IO-heavy workload (async should be faster)
    benchmark("IO-Heavy GO", {
        test_io_heavy_go();
    });

    benchmark("IO-Heavy ASYNC", {
        test_io_heavy_async();
    });

    // Test compute-heavy workload (go should be faster)
    benchmark("Compute-Heavy GO", {
        test_compute_heavy_go();
    });

    benchmark("Compute-Heavy ASYNC", {
        test_compute_heavy_async();
    });

    printf("\n=================================================================\n");
    printf("Summary:\n");
    printf("- For IO-heavy tasks: ASYNC wins due to efficient yielding\n");
    printf("- For compute-heavy tasks: GO wins due to true parallelism\n");
    printf("=================================================================\n");

    return 0;
}