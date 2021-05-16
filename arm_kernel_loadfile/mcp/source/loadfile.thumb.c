#include "loadfile.h"

#define real_MCP_LoadFile ((int (*)(ipcmessage *msg)) 0x0501CAA8 + 1) // + 1 for thumb
#define MCP_DoLoadFile    ((int (*)(const char *path, const char *path2, void *outputBuffer, uint32_t outLength, uint32_t pos, int *bytesRead, uint32_t unk)) 0x05017248 + 1)
#define real_MCP_ReadCOSXml_patch ((int (*)(uint32_t u1, uint32_t u2, MCPPPrepareTitleInfo *xmlData)) 0x050024ec + 1)
#define IOS_Open          ((int (*)(char* dev, int mode)) 0x05056984)
#define IOS_Close         ((int (*)(int fd)) 0x0505698c)
#define FSA_Mount         ((int (*)(int fd, char *device_path, char *volume_path, uint32_t flags, char *arg_string, int arg_string_len)) 0x050397b0)
#define mcp_strncmp       ((int (*)(const char*, const char*, int)) 0x05055e10)
#define ios_shutdown      ((void (*)(int reset)) 0x05056b7c)
#define MCP_SyslogPrint   ((void (*)(const char*, ...)) 0x0503dcf8)

static int MCP_LoadCustomFile(void *buffer_out, int buffer_len, int pos)
{
    int fsaFd = IOS_Open("/dev/fsa", 0);
    FSA_Mount(fsaFd, "/dev/sdcard01", "/vol/storage_homebrew", 2, NULL, 0);
    IOS_Close(fsaFd);

    int bytesRead = 0;
    int result = MCP_DoLoadFile("/vol/storage_homebrew/launch.rpx", NULL, buffer_out, buffer_len, pos, &bytesRead, 0);
    if (result >= 0) {
        if (!bytesRead) {
            return 0;
        }
        if (result >= 0) {
            return bytesRead;
        }
    }
    return result;
}

int _strlen(const char *str)
{
    const char *s;

    for (s = str; *s; ++s);

    return (s - str);
}

int MCP_LoadFile_patch(ipcmessage *msg)
{
    MCPLoadFileRequest *request = (MCPLoadFileRequest *) msg->ioctl.buffer_in;

    if(request->name[_strlen(request->name) - 1] == 'x') {
        // don't replace the menu
        if (mcp_strncmp(request->name + (_strlen(request->name) - 7), "men.rpx", sizeof("men.rpx")) != 0) {
            return MCP_LoadCustomFile(msg->ioctl.buffer_io, msg->ioctl.length_io, request->pos);
        }
    }

    return real_MCP_LoadFile(msg);
}

int MCP_ReadCOSXml_patch(uint32_t u1, uint32_t u2, MCPPPrepareTitleInfo *xmlData)
{
    int res = real_MCP_ReadCOSXml_patch(u1, u2, xmlData);
        
    // Give us sd access!
    xmlData->permissions[4].mask = 0xFFFFFFFFFFFFFFFF;

    return res;
}

