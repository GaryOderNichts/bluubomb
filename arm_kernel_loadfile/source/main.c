#include <stdlib.h>
#include "imports.h"

#include "mcp.bin.h"
#include "mcp_syms.h"

#define THUMB_BL(addr, func) ((0xF000F800 | ((((uint32_t)(func) - (uint32_t)(addr) - 4) >> 1) & 0x0FFF)) | ((((uint32_t)(func) - (uint32_t)(addr) - 4) << 4) & 0x7FFF000)) // +-4MB

int _main()
{
    kernel_flush_dcache(0x081200F0, 0x4001); // giving a size >= 0x4000 flushes all cache

    int level = kernel_disable_interrupts();

    unsigned int control_register = disable_mmu();

    /* Patch kernel_error_handler to BX LR immediately */
    *(int*) 0x08129A24 = 0xE12FFF1E;

    // load the custom ios_mcp
    kernel_memcpy((void *) (_text_start - 0x05000000 + 0x081C0000), mcp, mcp_size);

    // patch out mcp FSA_Mount file descriptor check
    *(volatile uint32_t *) (0x050397d8 - 0x05000000 + 0x081C0000) = 0xe3a00001; // mov r0, #1

    // replace the next loaded rpx
    *(volatile uint32_t *) (0x050254D6 - 0x05000000 + 0x081C0000) = THUMB_BL(0x050254D6, MCP_LoadFile_patch);

    // sd access
    *(volatile uint32_t *) (0x0501dd78 - 0x05000000 + 0x081C0000) = THUMB_BL(0x0501dd78, MCP_ReadCOSXml_patch);
    *(volatile uint32_t *) (0x051105ce - 0x05000000 + 0x081C0000) = THUMB_BL(0x051105ce, MCP_ReadCOSXml_patch);

    *(int*) 0x1555500 = 0;

    /* REENABLE MMU */
    restore_mmu(control_register);

    kernel_invalidate_dcache(0x081298BC, 0x4001); // giving a size >= 0x4000 invalidates all cache
    kernel_invalidate_icache();

    kernel_enable_interrupts(level);

    return 0;
}