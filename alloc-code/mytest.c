#include "alloc.h"
#include <stdio.h>
#include <string.h>

int main() {
  printf("--- Custom Memory Manager Test ---\n");

  if (init_alloc()) {
    printf("init_alloc failed, aborting.\n");
    return 1;
  }
}
