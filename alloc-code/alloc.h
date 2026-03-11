#ifndef ALLOC_H
#define ALLOC_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define PAGESIZE 4096 // size of memory to allocate from OS
#define MINALLOC 8    // allocations will be 8 bytes or multiples of it
#define GRANULE 8     // each granule is 8 bytes
#define NUM_GRANULES (PAGESIZE / GRANULE) // 512

// Global metadata
static void *page_base = NULL;                       // address of the 4KB page
static unsigned char bitmap[NUM_GRANULES / 8] = {0}; // 64 bytes
static int alloc_size[NUM_GRANULES] = {
    0}; // size in bytes for each starting granule

// Chech if a specific granule is free (bit = 0)
static int is_free(int idx) {
  int byte = idx / 8;
  int bit = idx % 8;
  return !(bitmap[byte] & (1 << bit));
}

// Set or clear a range of granules
static void set_range(int start, int count, int allocated) {
  for (int i = start; i < start + count; i++) {
    int byte = i / 8;
    int bit = i % 8;
    if (allocated)
      bitmap[byte] |= (1 << bit);
    else
      bitmap[byte] &= ~(1 << bit);
  }
}

// Find a run of at least 'neaded' consecutive free granules (first fit)
static int find_free_run(int needed) {
  int run_start = -1;
  int run_len = 0;
  for (int i = 0; i < NUM_GRANULES; i++) {
    if (is_free(i)) {
      if (run_len == 0)
        run_start = i;
      run_len++;
      if (run_start >= needed)
        return run_start;
    } else {
      run_len = 0;
    }
  }
  return -1;
}

int init_alloc() {
  // Request one 4KB anonymous page
  page_base = mmap(NULL, PAGESIZE, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (page_base == MAP_FAILED)
    return 1; // failure

  // Clear metadata
  memset(bitmap, 0, sizeof(bitmap));
  memset(alloc_size, 0, sizeof(alloc_size));
  return 0;
}

int cleanup() {
  if (page_base) {
    if (munmap(page_base, PAGESIZE) == -1)
      return 1;
    page_base = NULL;
  }
  // metadata is static will be overwritten on the next init_alloc
  return 0;
}

char *alloc(int size) {
  // Validate request
  if (size <= 0 || size % MINALLOC != 0 || size > PAGESIZE)
    return NULL;
  int needed = size / GRANULE; // number of granules required
  int start = find_free_run(needed);
  if (start == -1)
    return NULL; // not enough contiguos free space

  // Mark granules as allocated
  set_range(start, needed, 1);
  // Record the size at the starting granule
  alloc_size[start] = size;

  return (char *)page_base + start * GRANULE;
}

void dealloc(char *ptr) {
  if (!ptr || !page_base)
    return;

  // Compute granule index
  int idx = (ptr - (char *)page_base) / GRANULE;
  if (idx < 0 || idx >= NUM_GRANULES)
    return; // pointer out of range

  int size = alloc_size[idx];
  if (size == 0) {
    return; // not a valid allocated block
  }

  int count = size / GRANULE; // number of granules to free

  // Clear the bits in the bitmap
  set_range(idx, count, 0);
  // Reset the size entry
  alloc_size[idx] = 0;
}

#endif // ALLOC_H
