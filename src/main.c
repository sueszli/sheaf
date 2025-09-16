#include<stdlib.h>
#include<stdio.h>

int main() {
    printf("Hello, world!\n");

    char *leak = malloc(100);
    if (leak) {
        sprintf(leak, "this memory will be leaked!");
    }

    return 0;
}
