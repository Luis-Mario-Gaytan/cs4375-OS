#include "alloc.h"
#include <stdio.h>
#include <string.h>

// macro helper to print test result
#define TEST(cond, msg)                                                        \
  do {                                                                         \
    if (cond)                                                                  \
      printf("%s\n", msg);                                                     \
    else                                                                       \
      printf("%s\n", msg);                                                     \
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
  // Test 4: Maximum small allocations
  // Test 5: Complex fragmentation and allocation
  // Test 6: Dealloc with NULL or invalid pointer (should not crash)
  // Test 7: Cleanup and re-initialize
}
