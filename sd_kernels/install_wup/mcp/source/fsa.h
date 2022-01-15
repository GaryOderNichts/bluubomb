#ifndef FSA_H
#define FSA_H

typedef struct
{
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
}fileStat_s;

typedef struct
{
    fileStat_s stat;
	char name[0x100];
}directoryEntry_s;

#define DIR_ENTRY_IS_DIRECTORY      0x80000000

#define FSA_MOUNTFLAGS_BINDMOUNT (1 << 0)
#define FSA_MOUNTFLAGS_GLOBAL (1 << 1)

int FSA_Mount(int fd, char* device_path, char* volume_path, uint32_t flags, char* arg_string, int arg_string_len);
int FSA_Unmount(int fd, char* path, uint32_t flags);
int FSA_FlushVolume(int fd, char* volume_path);

int FSA_GetDeviceInfo(int fd, char* device_path, int type, uint32_t* out_data);

int FSA_MakeDir(int fd, char* path, uint32_t flags);
int FSA_OpenDir(int fd, char* path, int* outHandle);
int FSA_ReadDir(int fd, int handle, directoryEntry_s* out_data);
int FSA_RewindDir(int fd, int handle);
int FSA_CloseDir(int fd, int handle);
int FSA_ChangeDir(int fd, char* path);

int FSA_OpenFile(int fd, char* path, char* mode, int* outHandle);
int FSA_ReadFile(int fd, void* data, uint32_t size, uint32_t cnt, int fileHandle, uint32_t flags);
int FSA_WriteFile(int fd, void* data, uint32_t size, uint32_t cnt, int fileHandle, uint32_t flags);
int FSA_StatFile(int fd, int handle, fileStat_s* out_data);
int FSA_CloseFile(int fd, int fileHandle);
int FSA_SetPosFile(int fd, int fileHandle, uint32_t position);
int FSA_GetStat(int fd, char *path, fileStat_s* out_data);
int FSA_Remove(int fd, char *path);
int FSA_ChangeMode(int fd, char *path, int mode);

int FSA_RawOpen(int fd, char* device_path, int* outHandle);
int FSA_RawRead(int fd, void* data, uint32_t size_bytes, uint32_t cnt, uint64_t sector_offset, int device_handle);
int FSA_RawWrite(int fd, void* data, uint32_t size_bytes, uint32_t cnt, uint64_t sector_offset, int device_handle);
int FSA_RawClose(int fd, int device_handle);

#endif
