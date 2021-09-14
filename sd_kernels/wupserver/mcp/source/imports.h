#pragma once

#include <stdint.h>

#define MCP_SVC_BASE ((void*) 0x050567ec)

typedef struct {
	void* ptr;
	uint32_t len;
	uint32_t unk;
} iovec_s;

#define usleep                  ((void(*)(uint32_t)) 0x050564e4)

#define IOS_CreateThread        ((int(*)(int (*proc)(void* arg), void* arg, uint32_t* stack_top, uint32_t stacksize, int priority, uint32_t flags)) 0x050567ec)
#define IOS_StartThread         ((int(*)(int threadid)) 0x05056824)
#define IOS_ReceiveMessage		((int(*)(int queueid, uint32_t *message, uint32_t flags)) 0x0505686c)
#define IOS_Alloc               ((void*(*)(int heapid, uint32_t size)) 0x05056924)
#define IOS_AllocAligned        ((void*(*)(int heapid, uint32_t size, uint32_t alignment)) 0x0505692c)
#define IOS_Free                ((void(*)(int heapid, void *ptr)) 0x05056934)
#define IOS_Open                ((int(*)(const char* device, int mode)) 0x05056984)
#define IOS_Close               ((int(*)(int fd)) 0x0505698c)
#define IOS_Ioctl               ((int(*)(int fd, uint32_t request, void *input_buffer, uint32_t input_buffer_len, void *output_buffer, uint32_t output_buffer_len)) 0x050569ac)
#define IOS_Ioctlv              ((int(*)(int fd, uint32_t request, uint32_t vector_count_in, uint32_t vector_count_out, iovec_s *vector)) 0x050569b4)
#define IOS_InvalidateDCache    ((void(*)(void *ptr, unsigned int len)) 0x05056a74)
#define IOS_FlushDCache         ((void(*)(void *ptr, unsigned int len)) 0x05056a7c)
#define IOS_Shutdown			((void(*)(int)) 0x05056b7c)

#define _memcpy                 ((void*(*)(void*, const void*, uint32_t)) 0x05054e54)
#define _memset                 ((void*(*)(void*, int, uint32_t)) 0x05054ef0)
