#include <stdio.h>

#define CROUTINES_IMPLEMENTATION
#include "../croutines.h"

void counter(int count) {
    for (int i = 0; i <= count; i++) {
        printf("[%d]: %d\n", cRoutineId, i);
        croutine_yield();
    }
}

int main() {
    croutine_yield();

    croutine_new(counter, (void *) 4);
    croutine_new(counter, (void *) 5);
    croutine_new(counter, (void *) 3);

    printf("c routines started\n");
    while (cRoutineCount > 1)
        croutine_yield();
    printf("c routines finished\n");

    return 0;
}