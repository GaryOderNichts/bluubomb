#include "mcp_install.h"

static void* allocIoBuf(uint32_t size)
{
    void* ptr = IOS_Alloc(0xcaff, size);
    _memset(ptr, 0, size);
    return ptr;
}

static void freeIoBuf(void* ptr)
{
    IOS_Free(0xcaff, ptr);
}

int MCP_InstallGetInfo(int handle, const char* path, void* out_info)
{
    uint8_t* buf = allocIoBuf(0x27f + 0x16 + (sizeof(iovec_s) * 2));

    char* path_buf = (char*) buf + (sizeof(iovec_s) * 2);
    _strncpy(path_buf, path, 0x27f);

    void* out_buf = buf + (sizeof(iovec_s) * 2) + 0x27f;

    iovec_s* vecs = (iovec_s*) buf;
    vecs[0].ptr = path_buf;
    vecs[0].len = 0x27f;
    vecs[1].ptr = out_buf;
    vecs[1].len = 0x16;

    int res = IOS_Ioctlv(handle, 0x80, 1, 1, vecs);
    if (res >= 0) {
        _memcpy(out_info, out_buf, 0x16);
    }

    freeIoBuf(buf);

    return res;
}

int MCP_SetTargetUsb(int handle, int target)
{
    uint32_t* buf = allocIoBuf(4);
    _memcpy(buf, &target, 4);

    int res = IOS_Ioctl(handle, 0xf1, buf, 4, NULL, 0);

    freeIoBuf(buf);

    return res;
}

int MCP_InstallSetTargetDevice(int handle, int device)
{
    uint32_t* buf = allocIoBuf(4);
    _memcpy(buf, &device, 4);

    int res = IOS_Ioctl(handle, 0x8d, buf, 4, NULL, 0);

    freeIoBuf(buf);

    return res;
}

int MCP_InstallTitle(int handle, const char* path)
{
    uint8_t* buf = allocIoBuf(0x27f + sizeof(iovec_s));

    char* path_buf = (char*) buf + sizeof(iovec_s);
    _strncpy(path_buf, path, 0x27f);

    iovec_s* vecs = (iovec_s*) buf;
    vecs[0].ptr = path_buf;
    vecs[0].len = 0x27f;

    int res = IOS_Ioctlv(handle, 0x81, 1, 0, vecs);

    freeIoBuf(buf);

    return res;
}
