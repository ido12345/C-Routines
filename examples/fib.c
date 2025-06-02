#include <stdio.h>

#define CROUTINES_IMPLEMENTATION
#include "../croutines.h"

int fib(int x) {
    if (x <= 1)
        return x;
    return fib(x - 1) + fib(x - 2);
}

int main() {
    CTask *fibTask = croutine_new(fib, (void *) 40);

    while (!fibTask->finished) {
        croutine_yield();
    }

    printf("%ld\n", (long) fibTask->result);
    ctask_destroy(fibTask);
    return 0;
}