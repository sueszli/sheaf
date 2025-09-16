#include "unity.h"
#include <stdlib.h>

void setUp(void) {
}

void tearDown(void) {
}

void test_pointer_basics(void) {
    int value = 42;
    int *ptr = &value;

    TEST_ASSERT_EQUAL(42, *ptr);
    TEST_ASSERT_EQUAL(&value, ptr);

    *ptr = 100;
    TEST_ASSERT_EQUAL(100, value);
}

void test_null_pointer(void) {
    int *ptr = NULL;

    TEST_ASSERT_NULL(ptr);
    TEST_ASSERT_FALSE(ptr != NULL);
}

void test_pointer_arithmetic(void) {
    int arr[5] = {10, 20, 30, 40, 50};
    int *ptr = arr;

    TEST_ASSERT_EQUAL(10, *ptr);
    TEST_ASSERT_EQUAL(20, *(ptr + 1));
    TEST_ASSERT_EQUAL(30, *(ptr + 2));

    ptr++;
    TEST_ASSERT_EQUAL(20, *ptr);

    ptr += 2;
    TEST_ASSERT_EQUAL(40, *ptr);
}

void test_dynamic_allocation(void) {
    int *ptr = malloc(sizeof(int) * 5);
    TEST_ASSERT_NOT_NULL(ptr);

    for (int i = 0; i < 5; i++) {
        ptr[i] = i * 10;
    }

    TEST_ASSERT_EQUAL(0, ptr[0]);
    TEST_ASSERT_EQUAL(40, ptr[4]);

    free(ptr);
    // Note: After free, the pointer becomes invalid
    // In a real scenario, you should set ptr = NULL
}

void test_pointer_to_pointer(void) {
    int value = 123;
    int *ptr = &value;
    int **ptr_to_ptr = &ptr;

    TEST_ASSERT_EQUAL(123, **ptr_to_ptr);
    TEST_ASSERT_EQUAL(ptr, *ptr_to_ptr);
    TEST_ASSERT_EQUAL(&ptr, ptr_to_ptr);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_pointer_basics);
    RUN_TEST(test_null_pointer);
    RUN_TEST(test_pointer_arithmetic);
    RUN_TEST(test_dynamic_allocation);
    RUN_TEST(test_pointer_to_pointer);
    return UNITY_END();
}