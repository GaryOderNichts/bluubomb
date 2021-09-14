#include <stdlib.h>
#include "imports.h"

#include "../../arm_user/arm_user.bin.h"

#define ARM_BL(addr, func) (0xEB000000 | ((((uint32_t)(func) - (uint32_t)(addr) - 8) >> 2) & 0x00FFFFFF))

#define SD_KERNEL_CODE_LOCATION 0x08135400

int kernel_syscall_0x81(void* ptr, uint32_t size)
{
    // copy the custom kernel code
    kernel_memcpy((void*) SD_KERNEL_CODE_LOCATION, ptr, size);

    // jump to it
    ((void (*)(void)) 0x08135400)();

    return 0;
}

int _main()
{
    // disable interrupts and mmu
    int level = kernel_disable_interrupts();
    uint32_t control_register = disable_mmu();

    // patch kernel_error_handler
    *(volatile uint32_t*) 0x08129a24 = 0xe12fff1e; // bx lr

    // replace the custom kernel syscall
    *(volatile uint32_t*) 0x0812cd2c = ARM_BL(0x0812cd2c, kernel_syscall_0x81);

    // patch ios-pad fsa handle check to always succeed
    *(volatile uint32_t*) 0x11f7f418 = 0xe3a00001; // mov r0, #1
    *(volatile uint32_t*) 0x11f7f41c = 0xe12fff1e; // bx lr

    // give everything full access to fsa (we need this to access the sd from ios-pad)
    *(volatile uint32_t*) 0x107043e4 = 0xe3e02000; // mvn r2, #0
    *(volatile uint32_t*) 0x107043e8 = 0xe3e03000; // mvn r3, #0

    // load arm_user
    kernel_memcpy((void*) 0x11f85800, arm_user, arm_user_size);

    // reenable mmu
    restore_mmu(control_register);

    // invalidate all cache
    // kernel_invalidate_dcache(0x081298BC, 0x4001);
    kernel_invalidate_icache();

    // restore interrupts
    kernel_enable_interrupts(level);

    return 0;
}
