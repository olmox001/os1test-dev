.global main
main:
    /* sys_write(1, "TEST\n", 5) */
    mov x0, #1          /* fd */
    adr x1, msg         /* buf */
    mov x2, #5          /* len */
    mov x8, #1          /* syscall: SYS_WRITE */
    svc #0              /* Trigger syscall */

    /* sys_exit(0) */
    mov x0, #0
    mov x8, #93         /* syscall: SYS_EXIT */
    svc #0

loop:
    b loop

msg:
    .ascii "TEST\n"
    .balign 4
