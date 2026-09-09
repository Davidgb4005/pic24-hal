    .text
    .global _asm_add

_asm_add:
    add     w0, w1, w0
    return

_pull_stack