#include <stdio.h>
#include <stdlib.h>

int main() {
  printf("Hello, world!\n");

  // Intentional memory leak for testing
  char *leaked_memory = malloc(100);
  if (leaked_memory) {
    sprintf(leaked_memory, "This memory will leak!");
    printf("Allocated memory: %s\n", leaked_memory);
    // Note: deliberately not calling free(leaked_memory)
  }

  return 0;
}
