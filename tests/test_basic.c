#include "unity.h"
#include <stdio.h>
#include <stdlib.h>

void setUp(void) {
    // Set up code before each test
}

void tearDown(void) {
    // Clean up code after each test
}

void test_basic_math(void) {
    TEST_ASSERT_EQUAL(4, 2 + 2);
    TEST_ASSERT_EQUAL(10, 5 * 2);
    TEST_ASSERT_EQUAL(3, 9 / 3);
}

void test_string_operations(void) {
    char buffer[100];
    sprintf(buffer, "Hello, %s!", "World");
    TEST_ASSERT_EQUAL_STRING("Hello, World!", buffer);
}

void test_memory_allocation(void) {
    int *ptr = malloc(sizeof(int) * 10);
    TEST_ASSERT_NOT_NULL(ptr);

    for (int i = 0; i < 10; i++) {
        ptr[i] = i * 2;
    }

    TEST_ASSERT_EQUAL(0, ptr[0]);
    TEST_ASSERT_EQUAL(18, ptr[9]);

    free(ptr);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_basic_math);
    RUN_TEST(test_string_operations);
    RUN_TEST(test_memory_allocation);
    return UNITY_END();
}