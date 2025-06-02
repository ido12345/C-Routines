#ifndef _CROUTINES_H
#define _CROUTINES_H

// TODO: fix possible stack overflow corruption from a c routine

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef CROUTINES_DEFAULT_CAPACITY
#define CROUTINES_DEFAULT_CAPACITY 64
#endif // CROUTINES_DEFAULT_CAPACITY

#ifndef CROUTINE_STACK_SIZE
#define CROUTINE_STACK_SIZE 4096
#endif // CROUTINE_STACK_SIZE

typedef struct CTask CTask;
typedef struct CRoutine CRoutine;
typedef struct CRoutines CRoutines;

CTask *croutine_new(void *, void *arg);
void ctask_destroy(CTask *);

/////////////// ASSEMBLY COMPONENTS ///////////////

void croutine_yield(void);
void croutine_destroy(void);

/////////////// ASSEMBLY COMPONENTS ///////////////

#ifdef CROUTINES_IMPLEMENTATION

struct CRoutine {
    void **stack_top;
    void *stack_base;
    int id;
    CTask *task;
};

struct CTask {
    void *result;
    bool finished;
};

struct CRoutines {
    CRoutine *data;
    int count;
    int capacity;
    int current;
};

CRoutines cRoutines = {0};

#define ReserveMainRoutine                                                               \
    do {                                                                                 \
        if (cRoutines.data == NULL && cRoutines.count == 0 && cRoutines.capacity == 0) { \
            cRoutines.capacity = CROUTINES_DEFAULT_CAPACITY;                             \
            cRoutines.data = malloc(cRoutines.capacity * sizeof(*cRoutines.data));       \
            assert(cRoutines.data && "C routines malloc failure");                       \
            cRoutines.data[cRoutines.count++] = (CRoutine){0};                           \
        }                                                                                \
    } while (0)

#define cRoutineCount cRoutines.count

#define cRoutineId cRoutines.data[cRoutines.current].id

/////////////// ASSEMBLY COMPONENTS ///////////////

// not including rsp
#define REGISTERS_AMOUNT 15

#define PUSH_ALL_REGISTERS \
    __asm__ volatile(      \
        "push %rdi\n"      \
        "push %rbx\n"      \
        "push %rcx\n"      \
        "push %rdx\n"      \
        "push %rax\n"      \
        "push %rsi\n"      \
        "push %rbp\n"      \
        "push %r8\n"       \
        "push %r9\n"       \
        "push %r10\n"      \
        "push %r11\n"      \
        "push %r12\n"      \
        "push %r13\n"      \
        "push %r14\n"      \
        "push %r15\n")

#define POP_ALL_REGISTERS \
    __asm__ volatile(     \
        "pop %r15\n"      \
        "pop %r14\n"      \
        "pop %r13\n"      \
        "pop %r12\n"      \
        "pop %r11\n"      \
        "pop %r10\n"      \
        "pop %r9\n"       \
        "pop %r8\n"       \
        "pop %rbp\n"      \
        "pop %rsi\n"      \
        "pop %rax\n"      \
        "pop %rdx\n"      \
        "pop %rcx\n"      \
        "pop %rbx\n"      \
        "pop %rdi\n")

// capture the stack pointer into a variable
#define GET_STACK(dest) __asm__ volatile("mov %%rsp, %0\n" : "=r"(dest))
// switch the stack by assigning into the stack pointer
#define PUT_STACK(src) __asm__ volatile("mov %0, %%rsp\n" : : "r"(src))
// simply return
#define RET __asm__ volatile("ret")

void __attribute__((naked)) croutine_yield() {
    ReserveMainRoutine;

    // store registers on stack
    PUSH_ALL_REGISTERS;
    // store the stack pointer
    GET_STACK(cRoutines.data[cRoutines.current].stack_top);

    cRoutines.current++;
    if (cRoutines.current >= cRoutines.count) {
        cRoutines.current = 0;
    }

    // restore the stack
    PUT_STACK(cRoutines.data[cRoutines.current].stack_top);
    // restore the saved registers
    POP_ALL_REGISTERS;
    RET;
}

void __attribute__((naked)) croutine_destroy() {
    /*
        why not just mov rax into the result?
        this caused a very annoying bug, what was happening
        was that in order to get the address of the result
        to write into it, it modified rax and rdx to
        get the address then write there, the problem
        is that now rax has some garbage value,
        so i save rax into rbx, which is nonvolatile
        then mov that
    */
    __asm__ volatile("mov %rax, %rbx");
    __asm__ volatile("mov %%rbx, %0" : "=r"(cRoutines.data[cRoutines.current].task->result));

    cRoutines.data[cRoutines.current].task->finished = true;

    void *escaped_stack_base = cRoutines.data[cRoutines.current].stack_base;
    memmove(
        cRoutines.data + cRoutines.current,
        cRoutines.data + cRoutines.current + 1,
        (cRoutines.count - cRoutines.current - 1) * sizeof(*cRoutines.data));
    cRoutines.count--;
    if (cRoutines.current >= cRoutines.count) {
        cRoutines.current = 0;
    }

    // restore the stack
    PUT_STACK(cRoutines.data[cRoutines.current].stack_top);
    // free the stack of a finished c routine
    free(escaped_stack_base);
    // restore the saved registers
    POP_ALL_REGISTERS;
    RET;
}

/////////////// ASSEMBLY COMPONENTS ///////////////

CTask *croutine_new(void *func, void *arg) {
    ReserveMainRoutine;

    void *stack_base = NULL;
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
    stack_base = aligned_alloc(16, CROUTINE_STACK_SIZE);
#else
    stack_base = malloc(CROUTINE_STACK_SIZE);
    assert(stack_base && "Stack malloc failure");
#endif

    CTask *cTask = (CTask *) malloc(sizeof(*cTask));
    cTask->result = NULL;
    cTask->finished = false;
    assert(cTask && "Task malloc failure");

    void **stack_top = (void **) (stack_base + CROUTINE_STACK_SIZE);
    *(--stack_top) = croutine_destroy;
    *(--stack_top) = func;
    *(--stack_top) = arg;
    stack_top -= REGISTERS_AMOUNT - 1;

    if (cRoutines.count >= cRoutines.capacity) {
        cRoutines.capacity *= 2;
        cRoutines.data = realloc(cRoutines.data, cRoutines.capacity * sizeof(*cRoutines.data));
        assert(cRoutines.data && "C routines realloc failure");
    }
    cRoutines.data[cRoutines.count] = (CRoutine){
        .id = cRoutines.count,
        .stack_top = stack_top,
        .stack_base = stack_base,
        .task = cTask,
    };
    cRoutines.count++;
    return cTask;
}

void ctask_destroy(CTask *cTask) {
    if (cTask) {
        free(cTask);
    }
}

// undef library variables
#undef ReserveMainRoutine

#undef REGISTERS_AMOUNT
#undef PUSH_ALL_REGISTERS
#undef POP_ALL_REGISTERS
#undef GET_STACK
#undef PUT_STACK
#undef RET
// undef library variables

#endif // CROUTINES_IMPLEMENTATION

#endif // _CROUTINES_H