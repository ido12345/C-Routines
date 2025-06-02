format ELF64 executable

segment readable executable

entry start

STD_OUT = 1
SYS_WRITE = 1

macro SYSCALL SYSCALL_ID {
    mov rax, SYSCALL_ID
    syscall
}

macro WRITE_OUT buffer, bufferLen {
    push rdx
    push rsi
    push rdi

    mov rdx, bufferLen
    lea rsi, [buffer]
    mov rdi, STD_OUT
    SYSCALL SYS_WRITE

    pop rdi
    pop rsi
    pop rdx
}

COROUTINE_AMOUNT = 5
COROUTINE_STACK_SIZE = 1024

coroutine_init:
    mov rax, [COROUTINE_STACK_END]
    inc QWORD [COROUTINE_COUNTER]
    mov rsi, [COROUTINE_COUNTER]

    sub rax, 8
    mov QWORD [rax], coroutine_destroy

    ; sub rax, 8*6
    mov [COROUTINE_RSPS + rsi*8], rax
    sub QWORD [COROUTINE_STACK_END], COROUTINE_STACK_SIZE
    mov QWORD [COROUTINE_RBPS + rsi*8], 0

    mov [COROUTINE_RIPS + rsi*8], rdi
    ret

; ...........[ret]

coroutine_yield:
    pop rax
    mov rsi, [COROUTINE_CURRENT]

    ; push rax
    ; push rbx
    ; push rcx
    ; push rdx
    ; push rdi
    ; push rsi

    mov [COROUTINE_RSPS + rsi*8], rsp
    mov [COROUTINE_RBPS + rsi*8], rbp
    mov [COROUTINE_RIPS + rsi*8], rax

    xor rdi, rdi
    inc rsi
    cmp rsi, [COROUTINE_COUNTER]
    cmovg rsi, rdi
    mov [COROUTINE_CURRENT], rsi

    mov rsp, [COROUTINE_RSPS + rsi*8]
    mov rbp, [COROUTINE_RBPS + rsi*8]

    ; pop rsi
    ; pop rdi
    ; pop rdx
    ; pop rcx
    ; pop rbx
    ; pop rax

    jmp [COROUTINE_RIPS + rsi*8]

coroutine_destroy:
    WRITE_OUT unimplemented_msg, unimplemented_msg_len

    mov rdi, 1
    mov eax, 60
    syscall

start:
    ;mov QWORD [COROUTINE_RSPS], rsp
    ;mov QWORD [COROUTINE_RBPS], rbp

    mov rdi, count
    call coroutine_init
    mov rdi, count
    call coroutine_init

    call coroutine_yield
    call coroutine_yield
    call coroutine_yield
    call coroutine_yield
    call coroutine_yield
    call coroutine_yield
    call coroutine_yield
    call coroutine_yield
    call coroutine_yield
    call coroutine_yield
    call coroutine_yield

    xor rdi, rdi
    mov eax, 60
    syscall

count:
    push rbp
    push rbx
    mov rbp, rsp

    sub rsp, 2

    mov BYTE [rbp-2], '0'
    mov BYTE [rbp-1], 0x0A
    mov rbx, 10 ; count 0 -> 9 

.printLoop:
    WRITE_OUT rbp-2, 2

    push rbx
    call coroutine_yield
    pop rbx
    
    inc BYTE [rbp-2]
    dec rbx
    jnz .printLoop

    mov rsp, rbp ; cleans up my 2 bytes
    pop rbx
    pop rbp
    ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; ARG1: rdi: 64bit number to print unsigned
printSignedNum:
    push rbp
    push rax
    push rbx
    push rdx
    push rdi
    push rsi
    mov rbp, rsp

    mov rax, rdi ; ARG1
    mov rdi, 0

    sub rsp, 32
    lea rsi, [rbp-1]
    mov byte [rsi], 0x0A

    cmp rax, 0
    jge .startDivLoop
    mov rdi, 1
    neg rax
    jmp .startDivLoop

.divLoop:
    cmp rax, 0
    je .doneDiv
.startDivLoop:
    mov rbx, 10
    mov rdx, 0
    div rbx
    add dl, '0'
    dec rsi
    mov [rsi], dl 
    jmp .divLoop

.doneDiv:

    cmp rdi, 0
    je .print
    dec rsi
    mov byte [rsi], '-'

.print:
    mov rdi, STD_OUT
    mov rdx, rbp
    sub rdx, rsi
    SYSCALL SYS_WRITE

    mov rsp, rbp
    pop rsi
    pop rdi
    pop rdx
    pop rbx
    pop rax
    pop rbp
    ret
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

segment readable writeable

unimplemented_msg db "Destroy not yet implemented", 10
unimplemented_msg_len =$-unimplemented_msg

COROUTINE_CURRENT dq 0
COROUTINE_COUNTER dq 0

COROUTINE_STACK_END dq COROUTINE_STACKS + COROUTINE_STACK_SIZE * COROUTINE_AMOUNT
COROUTINE_STACKS rb COROUTINE_STACK_SIZE * COROUTINE_AMOUNT
COROUTINE_RSPS rq COROUTINE_AMOUNT
COROUTINE_RBPS rq COROUTINE_AMOUNT

; COROUTINE_RAXS rq COROUTINE_AMOUNT
; COROUTINE_RBXS rq COROUTINE_AMOUNT
; COROUTINE_RCXS rq COROUTINE_AMOUNT
; COROUTINE_RDXS rq COROUTINE_AMOUNT
; COROUTINE_RDIS rq COROUTINE_AMOUNT
; COROUTINE_RSIS rq COROUTINE_AMOUNT

COROUTINE_RIPS rq COROUTINE_AMOUNT