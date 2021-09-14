#include <stdlib.h>
#include "fsa.h"

void _main()
{
    int fsaFd = IOS_Open("/dev/fsa", 0);
    
    FSA_Mount(fsaFd, "/dev/sdcard01", "/vol/storage_hb", 2, NULL, 0);

    int fileHandle;
    FSA_OpenFile(fsaFd, "/vol/storage_hb/bluu_kern.bin", "r", &fileHandle);

    fileStat_s stat;
    FSA_StatFile(fsaFd, fileHandle, &stat);

    void* fileBuf = IOS_AllocAligned(0xcaff, stat.size, 0x40);

    FSA_ReadWriteFile(fsaFd, fileBuf, 1, stat.size, 0, fileHandle, 0);

    // closing and freeing takes up too much space :P
    // FSA_CloseFile(fsaFd, fileHandle);
    // IOS_Close(fsaFd);

    // run the loaded code via the custom kernel syscall
    kernel_syscall_0x81(fileBuf, stat.size);

    //IOS_Free(0xcaff, fileBuf);

    // TODO maybe we can recover IOS-PAD execution somehow
    while (1);
}
