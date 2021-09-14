#pragma once

#include <stdint.h>

#define ALIGNAS(x, align)  (((x) + ((align) - 1)) & ~((align) - 1))

// Kernel functions

#define kernel_memcpy               ((void* (*)(void*, const void*, int)) 0x08131D04)
#define kernel_memset               ((void* (*)(void*, int, unsigned int)) 0x08131DA0)
#define kernel_strncpy              ((char* (*)(char*, const char*, unsigned int)) 0x081329B8)
#define kernel_disable_interrupts   ((int (*)(void)) 0x0812E778)
#define kernel_enable_interrupts    ((int (*)(int)) 0x0812E78C)
#define kernel_bsp_command_5        ((int (*)(const char*, int offset, const char*, int size, void *buffer)) 0x0812EC40)
#define kernel_vsnprintf            ((int (*)(char *, size_t, const char *, va_list)) 0x0813293c)
#define kernel_snprintf             ((int (*)(char *, size_t, const char *, ...)) 0x08132988)
#define kernel_strncmp              ((int (*)(const char *, const char *, size_t)) 0x08132a14)
#define kernel_invalidate_icache    ((void (*)(void)) 0x0812DCF0)
#define kernel_invalidate_dcache    ((void (*)(unsigned int, unsigned int)) 0x08120164)
#define kernel_ios_shutdown         ((void (*)(int)) 0xffffdc48)
#define kernel_ios_reset            ((void (*)(void)) 0x08129760)
#define kernel_flush_dcache         ((void (*)(unsigned int, unsigned int)) 0x08120160)

static inline unsigned int disable_mmu(void)
{
    unsigned int control_register = 0;
    asm volatile("MRC p15, 0, %0, c1, c0, 0" : "=r" (control_register));
    asm volatile("MCR p15, 0, %0, c1, c0, 0" : : "r" (control_register & 0xFFFFEFFA));
    return control_register;
}

static inline void restore_mmu(unsigned int control_register)
{
    asm volatile("MCR p15, 0, %0, c1, c0, 0" : : "r" (control_register));
}
