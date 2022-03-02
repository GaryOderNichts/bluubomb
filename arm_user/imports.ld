PROVIDE(IOS_CancelThread = 0x11f82e78);
PROVIDE(IOS_AllocAligned = 0x11f82fa8);
PROVIDE(IOS_Free = 0x11f82fb0);
PROVIDE(IOS_Open = 0x11f83000);
PROVIDE(IOS_Close = 0x11f83008);
PROVIDE(IOS_Ioctlv = 0x11f83030);
PROVIDE(IOS_Shutdown = 0x11f831f8);

PROVIDE(kernel_syscall_0x81 = 0x11f83270);

PROVIDE(strncpy = 0x11f83e80);
PROVIDE(memset = 0x11f83488);

PROVIDE(FSAShimOpen = 0x11f7f7fc);
PROVIDE(FSAShimClose = 0x11f7f728);

PROVIDE(FSA_AllocIoBuf = 0x11f7f554);
PROVIDE(FSA_FreeIoBuf = 0x11f7f59c);

PROVIDE(FSA_CloseFile = 0x11f7fae8);
PROVIDE(FSA_StatFile = 0x11f7fbc4);
PROVIDE(FSA_ReadWriteFile = 0x11f7fccc);
PROVIDE(FSA_OpenFile = 0x11f80024);
