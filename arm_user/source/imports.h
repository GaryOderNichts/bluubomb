#pragma once

#include <stdint.h>

typedef struct {
	void* ptr;
	uint32_t len;
	uint32_t unk;
} iovec_s;

#define IOS_CancelThread	((int(*)(int threadid, uint32_t* returnVal)) 0x11f82e78)
#define IOS_AllocAligned    ((void* (*)(int heap, uint32_t size, uint32_t alignment)) 0x11f82fa8)
#define IOS_Free            ((void (*)(int heap, void* ptr)) 0x11f82fb0)
#define IOS_Open            ((int (*)(const char* device, int mode)) 0x11f83000)
#define IOS_Close           ((int (*)(int fd)) 0x11f83008)
#define IOS_Ioctlv          ((int (*)(int fd, uint32_t request, uint32_t vector_count_in, uint32_t vector_count_out, iovec_s *vector)) 0x11f83030)
#define IOS_Shutdown        ((void (*)(int)) 0x11f831f8)

#define kernel_syscall_0x81 ((int (*)(void* ptr, uint32_t size)) 0x11f83270)

#define FSA_AllocIoBuf  ((int (*)(void **outBuf)) 0x11f7f554)
#define FSA_FreeIoBuf   ((void (*)(void* buf)) 0x11f7f59c)

#define _strncpy        ((char* (*)(char*, const char*, uint32_t)) 0x11f83e80)
