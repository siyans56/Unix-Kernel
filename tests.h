#ifndef TESTS_H
#define TESTS_H

#define LARGE_PTR 0xFFFFFFFF // large pointer
#define MULT_INT 25 // counter for rtc itnerrupt
#define OVERFLOW 99999999 // number for overflow
#define LARGE_OVERFLOW 999999999 // large number for overflow
#define FLOAT_TEST 5 // float value
#define DEREF_TEST 10 // dereference value
#define BAD_PTR -100 // bad pointer value
#define ONEKB 1024 // one kylobyte
#define PAGE_IN_BOUND 0xB8000 // ptr location for paging_in_bounds_test

// the following macros are arbitrarily chosen - and are just used to read a certain number of bits at a given offset in a given file
#define VERY_LARGE_SIZE 5277
#define VERY_LARGE_OFFSET 5229
#define VERY_LARGE_SMALL_OFFSET 21
#define VERY_LARGE_NUM_COPIED 26
#define FRAME0_SIZE 187
#define FRAME0_INDEX 10
#define SHELL_OFFSET 5572
#define KB_WRITE_SIZE 32
#define KB_LONG_WRITE_SIZE 165
// test launcher
void launch_tests();

#endif /* TESTS_H */
