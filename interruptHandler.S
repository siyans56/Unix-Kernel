
.data
    NUM_SYS_CALLS = 10 # supporting ten system calls
    SYS_START = 1 # start of range for system calls
    FOUR_OFF = 4 # used for 4 byte offset
    ST_POP = 12 # used for popping off stack

.global keyboard_INT
.global rtc_INT
.global sys_call_INT
.global pit_INT

# subroutine keyboard_INT
# inputs: none
# outputs: none
# side effects: saves/restores all registers, before/after calling keyboard interrupt
keyboard_INT:

    # save registers
    pushl %eax
    pushl %ebx
    pushl %ecx
    pushl %ebp
    pushl %esp
    pushl %esi
    pushl %edi

    # interrupt call to kb.c
    call read_kb

    # restore registers
    popl %edi
    popl %esi
    popl %esp
    popl %ebp
    popl %ecx
    popl %ebx
    popl %eax

    iret

    # subroutine rtc_INT
    # inputs: none
    # outputs: none
    # side effects: saves/restores all registers, before/after calling rtc interrupt

rtc_INT:

    # save registers
    pushl %eax
    pushl %ebx
    pushl %ecx
    pushl %ebp
    pushl %esp
    pushl %esi
    pushl %edi

    # interrupt call to rtc.c
    call rtc_interrupt

    # restore registers
    popl %edi
    popl %esi
    popl %esp
    popl %ebp
    popl %ecx
    popl %ebx
    popl %eax

    iret

    # subroutine PIT_INT
    # inputs: none
    # outputs: none
    # side effects: saves/restores all registers, before/after calling rtc interrupt

pit_INT:

    # save registers
    pushl %eax
    pushl %ebx
    pushl %ecx
    pushl %ebp
    pushl %esp
    pushl %esi
    pushl %edi

    # interrupt call to scheduling.c
    call pit_interrupt

    # restore registers
    popl %edi
    popl %esi
    popl %esp
    popl %ebp
    popl %ecx
    popl %ebx
    popl %eax

    iret


    # subroutine sys_call
    # inputs: none
    # outputs: none
    # side effects: saves/restores all registers, before/after calling sys_call interrupt

sys_call_INT:

    # save registers
    pushl %eax
    pushl %ebx
    pushl %ecx
    pushl %ebp
    pushl %esp
    pushl %esi
    pushl %edi
    pushfl

    pushl %edx
    pushl %ecx
    pushl %ebx

    # check for bounds in jumptable
    cmpl $SYS_START, %eax
    jl ERROR
    cmpl $NUM_SYS_CALLS, %eax
    ja ERROR
    decl %eax
    call *jumptable(, %eax, FOUR_OFF)
    jmp END

ERROR:
    movl $-1, %eax

END:
    # restore registers
    addl $ST_POP, %esp
    popfl
    popl %edi
    popl %esi
    popl %esp
    popl %ebp
    popl %ecx
    popl %ebx
    addl $FOUR_OFF, %esp

    iret



jumptable:
.long halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn
