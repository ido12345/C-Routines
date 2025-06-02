#ifndef _COROUTINES_H
#define _COROUTINES_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef COROUTINES_DEFAULT_CAPACITY
#define COROUTINES_DEFAULT_CAPACITY 64
#endif // COROUTINES_DEFAULT_CAPACITY

#ifndef COROUTINE_STACK_SIZE
#define COROUTINE_STACK_SIZE 1024
#endif // COROUTINE_STACK_SIZE

void coroutine_new(void *);
void _coroutine_switch(void *);
void _coroutine_destroy(void);
/////////////// ASSEMBLY COMPONENTS ///////////////
void coroutine_yield(void);
void _coroutine_restore(void *);
void _coroutine_escape(void *, void *);
/////////////// ASSEMBLY COMPONENTS ///////////////

#ifdef COROUTINES_IMPLEMENTATION

typedef struct {
    void *stack_top;
    void *stack_base;
} Coroutine;

typedef struct {
    Coroutine *data;
    int count;
    int capacity;
    int current;
} Coroutines;

Coroutines coroutines = {0};

#define ReserveMainCoroutine                                                                            \
    do {                                                                                                \
        if (coroutines.count == 0 && coroutines.capacity == 0) {                                        \
            coroutines.capacity = COROUTINES_DEFAULT_CAPACITY;                                          \
            coroutines.data = realloc(coroutines.data, coroutines.capacity * sizeof(*coroutines.data)); \
            assert(coroutines.data && "Realloc failed");                                                \
            coroutines.data[coroutines.count++] = (Coroutine){0};                                       \
        }                                                                                               \
    } while (0)

#define CoroutineCount coroutines.count

#define CoroutineId coroutines.current

/////////////// ASSEMBLY COMPONENTS ///////////////

// not including rsp
#define REGISTERS_AMOUNT 15

#define PUSH_ALL_REGISTERS \
    __asm__ volatile(      \
        "    push %rax\n"  \
        "    push %rbx\n"  \
        "    push %rcx\n"  \
        "    push %rdx\n"  \
        "    push %rdi\n"  \
        "    push %rsi\n"  \
        "    push %rbp\n"  \
        "    push %r8\n"   \
        "    push %r9\n"   \
        "    push %r10\n"  \
        "    push %r11\n"  \
        "    push %r12\n"  \
        "    push %r13\n"  \
        "    push %r14\n"  \
        "    push %r15\n")

#define POP_ALL_REGISTERS \
    __asm__ volatile(     \
        "    pop %r15\n"  \
        "    pop %r14\n"  \
        "    pop %r13\n"  \
        "    pop %r12\n"  \
        "    pop %r11\n"  \
        "    pop %r10\n"  \
        "    pop %r9\n"   \
        "    pop %r8\n"   \
        "    pop %rbp\n"  \
        "    pop %rsi\n"  \
        "    pop %rdi\n"  \
        "    pop %rdx\n"  \
        "    pop %rcx\n"  \
        "    pop %rbx\n"  \
        "    pop %rax\n")

// capture the stack pointer into a variable
#define GET_STACK(dest) __asm__ volatile("    mov %%rsp, %0\n" : "=r"(dest))
// switch the stack by assigning into the stack pointer
#define PUT_STACK(src) __asm__ volatile("    mov %0, %%rsp\n" : : "r"(src))
// simply return
#define RET __asm__ volatile("    ret")

void __attribute__((naked)) coroutine_yield() {
    ReserveMainCoroutine;

    PUSH_ALL_REGISTERS;
    register void *stack_top;
    GET_STACK(stack_top);
    _coroutine_switch(stack_top);
}

// restore the stack,
// restore the saved registers
void __attribute__((naked)) _coroutine_restore(register void *stack_top) {
    PUT_STACK(stack_top);
    POP_ALL_REGISTERS;
    RET;
}

// restore the stack,
// free the stack of a finished coroutine,
// and restore the saved registers
void __attribute__((naked)) _coroutine_escape(register void *stack_top, register void *escaped_stack_base) {
    PUT_STACK(stack_top);
    free(escaped_stack_base);
    POP_ALL_REGISTERS;
    RET;
}
/////////////// ASSEMBLY COMPONENTS ///////////////

void coroutine_new(void *func) {
    ReserveMainCoroutine;

    char *stack_base = (char *) malloc(COROUTINE_STACK_SIZE);
    assert(stack_base && "Malloc failure");

    void **stack_top = (void **) (stack_base + COROUTINE_STACK_SIZE);
    *(--stack_top) = _coroutine_destroy;
    *(--stack_top) = func;
    stack_top -= REGISTERS_AMOUNT;

    if (coroutines.count >= coroutines.capacity) {
        coroutines.capacity *= 2;
        coroutines.data = realloc(coroutines.data, coroutines.capacity * sizeof(*coroutines.data));
        assert(coroutines.data && "Realloc failure");
    }
    coroutines.data[coroutines.count++] = (Coroutine){
        .stack_top = stack_top,
        .stack_base = stack_base,
    };
}

void _coroutine_switch(register void *stack_top) {
    coroutines.data[coroutines.current].stack_top = stack_top;

    coroutines.current++;
    if (coroutines.current >= coroutines.count) {
        coroutines.current = 0;
    }

    _coroutine_restore(coroutines.data[coroutines.current].stack_top);
}

void _coroutine_destroy() {
    register void *escaped_stack_base = coroutines.data[coroutines.current].stack_base;
    memmove(
        coroutines.data + coroutines.current,
        coroutines.data + coroutines.current + 1,
        (coroutines.count - coroutines.current - 1) * sizeof(*coroutines.data));
    coroutines.count--;
    if (coroutines.current >= coroutines.count) {
        coroutines.current = 0;
    }

    _coroutine_escape(coroutines.data[coroutines.current].stack_top, escaped_stack_base);
}

// undef library variables
#undef ReserveMainCoroutine

#undef REGISTERS_AMOUNT
#undef PUSH_ALL_REGISTERS
#undef POP_ALL_REGISTERS
#undef GET_STACK
#undef PUT_STACK
#undef RET

#undef _coroutine_escape
// undef library variables

#endif // COROUTINES_IMPLEMENTATION

#endif // _COROUTINES_H