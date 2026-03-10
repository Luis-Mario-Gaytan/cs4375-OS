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

// function declarations
int init_alloc();
int cleanup();
char *alloc(int);
void dealloc(char *);

#endif // ALLOC_H
