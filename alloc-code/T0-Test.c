#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

int main() {
  printf("PID: %d\n", getpid());
  printf("Press Enter to continue...\n");
  getchar(); // Pause 1 – before mmap

  // Map one anonymous page (4KB)
  void *ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

  if (ptr == MAP_FILE) {
    perror("mmap failed");
    return 1;
  }

  printf("mmap succeeded at %p\n", ptr);
  printf("Press Enter to continue (after mmpa, before write)...\n");
  getchar(); // Pause 2

  // Write one byte into the page
  ((char *)ptr)[0] = 'A';
  printf("Wrote to page\n");
  printf("Press Enter to continue (after write)...\n");
  getchar(); // Pause 3 – after writing

  munmap(ptr, 4096);
  printf("Unmaped, exiting.\n");
  return 0;
}
