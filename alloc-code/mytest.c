#include "alloc.h"
#include <stdio.h>
#include <string.h>

// macro helper to print test result
#define TEST(cond, msg)                                                        \
  do {                                                                         \
    if (cond)                                                                  \
      printf("[PASS] %s\n", msg);                                              \
    else                                                                       \
      printf("[FAIL] %s\n", msg);                                              \
  } while (0)

int main() {
  printf("--- Custom Memory Manager Test ---\n\n");

  if (init_alloc()) {
    printf("init_alloc failed, aborting.\n");
    return 1;
  }

  // Test 1. Basic allocation and deallocation
  printf("--- Test 1: Basic alloc/dealloc --- \n");

  char *p1 = alloc(8);
  char *p2 = alloc(16);
  char *p3 = alloc(24);

  TEST(p1 != NULL && p2 != NULL && p3 != NULL, "Three allocations successful");
  TEST(p1 + 8 == p2 || p2 + 16 == p3, "Pointer are contiguous (likely)");

  dealloc(p2);

  char *p4 = alloc(16);

  TEST(p4 == p2, "Reuse freed block (first fit)");

  dealloc(p1);
  dealloc(p3);
  dealloc(p4);

  printf("\n");

  // Test 2: Full page allocations
  printf("--- Test 2: Full page allocation --- \n");

  char *big = alloc(4096);

  TEST(big != NULL, "Allocated full 4096 bytes");

  char *should_fail = alloc(8);

  TEST(should_fail == NULL, "No space left (returns NULL)");
  dealloc(big);
  printf("\n");

  // Test 3: Invalid sizes
  printf("--- Test 3: Invalid sizes ---\n");
  TEST(alloc(0) == NULL, "Size 0 returns NULL");
  TEST(alloc(-8) == NULL, "Negative size returns NULL");
  TEST(alloc(7) == NULL, "Size not multiple of 8 returns NULL");
  TEST(alloc(4100) == NULL, "Size > page returns NULL");
  printf("\n");

  // Test 4: Maximum small allocations
  printf("--- Test 4: Max small allocation (512 * 8) ---\n");
  char *ptrs[512];
  int i;
  for (i = 0; i < 512; i++) {
    ptrs[i] = alloc(8);
    if (!ptrs[i])
      break;
  }
  TEST(i == 512, "Allocated 512 blocks of 8 bytes");
  if (i == 512) {
    // Try one more
    char *extra = alloc(8);
    TEST(extra == NULL, "No extra space left");
  }
  // Free all
  for (i = 0; i < 512; i++) {
    dealloc(ptrs[i]);
  }
  printf("\n");

  // Test 5: Dealloc with NULL or invalid pointer (should not crash)
  printf("--- Tests 5: Edge deallocation ---\n");
  dealloc(NULL);
  dealloc((char *)0x12345678); // likely out of range, should be ignored.
  TEST(1, "dealloc with invalid pointers didn't crash");
  printf("\n");

  // Test 6: Cleanup and re-initialize
  printf("--- Test 6: Cleanup and re-init ---\n");
  TEST(cleanup() == 0, "cleanup succeeded");
  TEST(init_alloc() == 0, "re-init succeeded");

  char *test = alloc(8);
  TEST(test != NULL, "alloc works after init");
  dealloc(test);
  cleanup();
  printf("\n");

  printf("------ All test completed ------\n");
  return 0;
}
