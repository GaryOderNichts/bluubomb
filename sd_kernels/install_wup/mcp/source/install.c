#include "install.h"
#include "imports.h"
#include "mcp.h"
#include "fsa.h"
#include <stdlib.h>

#define STACK_SIZE 0x400
uint8_t stack[STACK_SIZE];

int thread_fn()
{
    int fsaFd = IOS_Open("/dev/fsa", 0);
    if (fsaFd < 0) {
        IOS_Shutdown(0);
        return 0;
    }

    int res = FSA_Mount(fsaFd, "/dev/sdcard01", "/vol/storage_sdcard", 2, NULL, 0);
    if (res < 0) {
        IOS_Shutdown(0);
        return 0;
    }

    int handle = IOS_Open("/dev/mcp", 0);
    if (handle < 0) {
        IOS_Shutdown(0);
        return 0;
    }

    uint8_t info[0x16];
    res = MCP_InstallGetInfo(handle, "/vol/storage_sdcard/install", info);
    if (res != 0) {
        IOS_Shutdown(0);
        return 0;
    }

    res = MCP_InstallSetTargetDevice(handle, 0);
    if (res != 0) {
        IOS_Shutdown(0);
        return 0;
    }

    res = MCP_SetTargetUsb(handle, 0);
    if (res != 0) {
        IOS_Shutdown(0);
        return 0;
    }

    res = MCP_InstallTitle(handle, "/vol/storage_sdcard/install");
    if (res != 0) {
        IOS_Shutdown(0);
        return 0;
    }

    // flush never hurts
    res = FSA_FlushVolume(fsaFd, "/vol/storage_mlc01");
    if (res < 0) {
        IOS_Shutdown(0);
        return 0;
    }

    IOS_Shutdown(1);

    return 0;
}

static int code_run = 0;

void do_install(void)
{
    if (code_run) {
        return;
    }

    code_run = 1;

    // run code in a new thread so the other thread doesn't get stuck
    int thread_id = IOS_CreateThread(thread_fn, NULL, stack + STACK_SIZE, STACK_SIZE, IOS_GetThreadPriority(0), 1);
    IOS_StartThread(thread_id);
}
