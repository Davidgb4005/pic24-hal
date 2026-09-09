
    .include "xc.inc"
    .text

    .global __T3Interrupt
    .extern _next_task

__T3Interrupt:

    push    W0
    push    W1
    push    W2
    push    W3
    push    W4
    push    W5
    push    W6
    push    W7
    push    W8
    push    W9
    push    W10
    push    W11
    push    W12
    push    W13
    push    W14

    bclr    IFS0, #T3IF

    mov     W15, W0
    call    _next_task
    mov     W0, W15

restore_context:


    pop     W14
    pop     W13
    pop     W12
    pop     W11
    pop     W10
    pop     W9
    pop     W8
    pop     W7
    pop     W6
    pop     W5
    pop     W4
    pop     W3
    pop     W2
    pop     W1
    pop     W0

    retfie


