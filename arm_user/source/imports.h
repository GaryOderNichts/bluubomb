#pragma once

#include <stdint.h>

typedef struct {
	void* ptr;
	uint32_t len;
	uint32_t unk;
} iovec_s;

int IOS_CancelThread(int threadid, uint32_t* returnVal);
void* IOS_AllocAligned(int heap, uint32_t size, uint32_t alignment);
void IOS_Free(int heap, void* ptr);
int IOS_Open(const char* device, int mode);
int IOS_Close(int fd);
int IOS_Ioctlv(int fd, uint32_t request, uint32_t vector_count_in, uint32_t vector_count_out, iovec_s *vector);
void IOS_Shutdown(int);

int kernel_syscall_0x81(void* ptr, uint32_t size);

char* strncpy(char*, const char*, uint32_t);
void* memset(void*, int, uint32_t);
