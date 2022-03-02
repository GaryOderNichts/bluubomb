#pragma once

#include <stdint.h>
#include <stdarg.h>

#define ALIGNAS(x, align)  (((x) + ((align) - 1)) & ~((align) - 1))

// Kernel functions
void* kernel_memcpy(void*, const void*, int);
void* kernel_memset(void*, int, unsigned int);
char* kernel_strncpy(char*, const char*, unsigned int);
int kernel_disable_interrupts(void);
int kernel_enable_interrupts(int);
int kernel_bsp_command_5(const char*, int offset, const char*, int size, void *buffer);
int kernel_vsnprintf(char *, size_t, const char *, va_list);
int kernel_snprintf(char *, size_t, const char *, ...);
int kernel_strncmp(const char *, const char *, size_t);
void kernel_invalidate_icache(void);
void kernel_invalidate_dcache(unsigned int, unsigned int);
void kernel_ios_shutdown(int);
void kernel_ios_reset(void);
void kernel_flush_dcache(unsigned int, unsigned int);
int setClientCapability(int pid, int fid, uint64_t mask);

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
