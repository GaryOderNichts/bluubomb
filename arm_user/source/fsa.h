#pragma once

#include "imports.h"

typedef struct {
    uint32_t flag;
    uint32_t permission;
    uint32_t owner_id;
    uint32_t group_id;
	uint32_t size; // size in bytes
	uint32_t physsize; // physical size on disk in bytes
	uint32_t unk[3];
	uint32_t id;
	uint32_t ctime;
	uint32_t mtime;
	uint32_t unk2[0x0D];
} fileStat_s;

int FSAShimOpen(int *attach);
int FSAShimClose(int fd);

int FSA_AllocIoBuf(void **outBuf);
void FSA_FreeIoBuf(void* buf);

int FSA_Mount(int fd, char* device_path, char* volume_path, uint32_t flags, char* arg_string, int arg_string_len);

int FSA_CloseFile(int fd, int fileHandle);

int FSA_StatFile(int fd, int handle, fileStat_s* out_data);

int FSA_ReadWriteFile(int fd, void* data, uint32_t size, uint32_t cnt, int flags, int fileHandle, int read_write_flag);

int FSA_OpenFile(int fd, char* path, char* mode, int* outHandle);
