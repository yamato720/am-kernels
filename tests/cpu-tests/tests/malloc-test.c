#include "trap.h"

int main() {
    char *p = malloc(128);
    check(p != NULL);
    strcpy(p, "Hello world!\n");
    printf("%s", p);

    printf("All tests passed!\n");

    return 0;
}