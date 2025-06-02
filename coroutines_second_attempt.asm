format ELF64

section '.text'

public coroutine_init
coroutine_init:
    mov rax, [COROUTINE_COUNTER]

    mov QWORD [COROUTINE_STACKS_END - 8], coroutine_destroy
    mov [COROUTINE_STACKS_END - 16], rdi
    lea rdx, [COROUTINE_STACKS_END - 16 - 15 * 8]
    mov [COROUTINE_RSPS + rax * 8], rdx

    sub QWORD [COROUTINE_STACKS_END], COROUTINE_STACK_SIZE

    inc QWORD [COROUTINE_COUNTER]

    ret

public coroutine_yield
coroutine_yield:
    push rax
    push rbx
    push rcx
    push rdx
    push rdi
    push rsi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rax, [COROUTINE_CURRENT]
    mov [COROUTINE_RSPS + rax * 8], rsp

    xor rbx, rbx
    inc rax
    cmp rax, [COROUTINE_COUNTER]
    cmovg rax, rbx

    mov [COROUTINE_CURRENT], rax

    mov rsp, [COROUTINE_RSPS + rax * 8]

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rsi
    pop rdi
    pop rdx
    pop rcx
    pop rbx
    pop rax
    ret

public coroutine_destroy
coroutine_destroy:
    mov rdx, unimplemented_msg_len
    lea rsi, [unimplemented_msg]
    mov rdi, 1
    mov rax, 1
    syscall

    mov rdi, 1
    mov eax, 60
    syscall

section '.data'
unimplemented_msg db "Destroy not yet implemented", 10
unimplemented_msg_len =$-unimplemented_msg

COROUTINE_MAX_AMOUNT = 5
COROUTINE_STACK_SIZE = 1024

COROUTINE_CURRENT dq 0
COROUTINE_COUNTER dq 0

COROUTINE_STACKS_END dq COROUTINE_STACKS + COROUTINE_STACK_SIZE * COROUTINE_MAX_AMOUNT

section '.bss'

COROUTINE_STACKS rb COROUTINE_STACK_SIZE * COROUTINE_MAX_AMOUNT

COROUTINE_RSPS rq COROUTINE_MAX_AMOUNT