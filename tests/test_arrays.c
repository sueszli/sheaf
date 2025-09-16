#include "unity.h"
#include <string.h>

void setUp(void) {}

void tearDown(void) {}

void test_array_initialization(void) {
    int arr[5] = {1, 2, 3, 4, 5};

    TEST_ASSERT_EQUAL(1, arr[0]);
    TEST_ASSERT_EQUAL(5, arr[4]);
    TEST_ASSERT_EQUAL(sizeof(int) * 5, sizeof(arr));
}

void test_array_manipulation(void) {
    int arr[3] = {10, 20, 30};
    int expected_initial[3] = {10, 20, 30};
    int expected_modified[3] = {10, 25, 30};

    // Test initial values
    TEST_ASSERT_EQUAL_INT_ARRAY(expected_initial, arr, 3);

    // Modify array
    arr[1] = 25;
    TEST_ASSERT_EQUAL(25, arr[1]);

    // Test modified array
    TEST_ASSERT_EQUAL_INT_ARRAY(expected_modified, arr, 3);
}

void test_string_array(void) {
    char str[] = "Hello";

    TEST_ASSERT_EQUAL(5, strlen(str));
    TEST_ASSERT_EQUAL('H', str[0]);
    TEST_ASSERT_EQUAL('o', str[4]);
    TEST_ASSERT_EQUAL('\0', str[5]);
}

void test_multidimensional_array(void) {
    int matrix[2][3] = {{1, 2, 3}, {4, 5, 6}};

    TEST_ASSERT_EQUAL(1, matrix[0][0]);
    TEST_ASSERT_EQUAL(6, matrix[1][2]);
    TEST_ASSERT_EQUAL(5, matrix[1][1]);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_array_initialization);
    RUN_TEST(test_array_manipulation);
    RUN_TEST(test_string_array);
    RUN_TEST(test_multidimensional_array);
    return UNITY_END();
}