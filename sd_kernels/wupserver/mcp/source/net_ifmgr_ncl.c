#include <stdlib.h>
#include <string.h>
#include "net_ifmgr_ncl.h"
#include "imports.h"

static int ifmgrncl_handle = 0;

int ifmgrnclInit()
{
	if(ifmgrncl_handle) return ifmgrncl_handle;

	int ret = IOS_Open("/dev/net/ifmgr/ncl", 0);

	if(ret > 0)
	{
		ifmgrncl_handle = ret;
		return ifmgrncl_handle;
	}

	return ret;
}

int ifmgrnclExit()
{
	int ret = IOS_Close(ifmgrncl_handle);

	ifmgrncl_handle = 0;

	return ret;
}

static void* allocIobuf(uint32_t size)
{
	void* ptr = IOS_Alloc(0xCAFF, size);

	if(ptr) _memset(ptr, 0x00, size);

	return ptr;
}

static void freeIobuf(void* ptr)
{
	IOS_Free(0xCAFF, ptr);
}

int	IFMGRNCL_GetInterfaceStatus(uint16_t interface_id, uint16_t* out_status)
{
	uint8_t* iobuf1 = allocIobuf(0x2);
	uint16_t* inbuf = (uint16_t*)iobuf1;
	uint8_t* iobuf2 = allocIobuf(0x8);
	uint16_t* outbuf = (uint16_t*)iobuf2;

	inbuf[0] = interface_id;

	int ret = IOS_Ioctl(ifmgrncl_handle, 0x14, inbuf, 0x2, outbuf, 0x8);

	if(!ret && out_status) *out_status = outbuf[2];

	freeIobuf(iobuf1);
	freeIobuf(iobuf2);
	return ret;
}
