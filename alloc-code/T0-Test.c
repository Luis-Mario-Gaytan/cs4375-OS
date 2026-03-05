#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

int main() {
  printf("PID: %d\n", getpid());
  printf("Press Enter to continue...\n");
  getchar(); // pause 1 – before mmap

  // TODO: mmap one anonymous page (size 4096)
  // Use MAP_ANONYMOUS | MAP_PRIVATE,
  // PROT_READ | PROT_WRITE Check return value

  printf("After mmap, press Enter...\n");
  getchar(); // pause 2 – after mmap, before writing

  // TODO: write at least one byte into the mapped page

  printf("After writing, press Enter to finish...\n");
  getchar(); // pause 3 – after writing

  // TODO: munmap the page
  return 0;
}
