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

int FSA_Mount(int fd, char* device_path, char* volume_path, uint32_t flags, char* arg_string, int arg_string_len);

#define FSA_CloseFile 		((int (*)(int fd, int fileHandle)) 0x11f7fae8)

#define FSA_StatFile  		((int (*)(int fd, int handle, fileStat_s* out_data)) 0x11f7fbc4)

#define FSA_ReadWriteFile  	((int (*)(int fd, void* data, uint32_t size, uint32_t cnt, int flags, int fileHandle, int read_write_flag)) 0x11f7fccc)

#define FSA_OpenFile        ((int (*)(int fd, char* path, char* mode, int* outHandle)) 0x11f80024)
