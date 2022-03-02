.section ".init"
.arm
.align 4

.extern _main
.type _main, %function

_start:
    bl _main

    @ restore the original stack pointer
    ldr sp, =0x12156424

    @ jump back into the btu_task loop
    ldr pc, =0x11f1bebc
