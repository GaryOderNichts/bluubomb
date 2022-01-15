#pragma once

#include <imports.h>

int MCP_InstallGetInfo(int handle, const char* path, void* out_info);

int MCP_SetTargetUsb(int handle, int target);

int MCP_InstallSetTargetDevice(int handle, int device);

int MCP_InstallTitle(int handle, const char* path);
