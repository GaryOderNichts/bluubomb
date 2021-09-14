#include "fsa.h"

int FSA_Mount(int fd, char* device_path, char* volume_path, uint32_t flags, char* arg_string, int arg_string_len)
{
	uint8_t* iobuf;
    FSA_AllocIoBuf((void**) &iobuf);

	uint8_t* inbuf8 = iobuf;
	uint8_t* outbuf8 = &iobuf[0x520];
	iovec_s* iovec = (iovec_s*)&iobuf[0x7C0];
	uint32_t* inbuf = (uint32_t*)inbuf8;
	uint32_t* outbuf = (uint32_t*)outbuf8;

	_strncpy((char*) &inbuf8[0x04], device_path, 0x27F);
	_strncpy((char*) &inbuf8[0x284], volume_path, 0x27F);
	inbuf[0x504 / 4] = (uint32_t) flags;
	inbuf[0x508 / 4] = (uint32_t) arg_string_len;

	iovec[0].ptr = inbuf;
	iovec[0].len = 0x520;
	iovec[1].ptr = arg_string;
	iovec[1].len = arg_string_len;
	iovec[2].ptr = outbuf;
	iovec[2].len = 0x293;

	int ret = IOS_Ioctlv(fd, 0x01, 2, 1, iovec);

	FSA_FreeIoBuf(iobuf);
	return ret;
}
