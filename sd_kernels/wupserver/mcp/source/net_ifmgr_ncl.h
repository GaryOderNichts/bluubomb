#pragma once

#include <stdint.h>

int ifmgrnclInit();
int ifmgrnclExit();

int	IFMGRNCL_GetInterfaceStatus(uint16_t interface_id, uint16_t* out_status);
