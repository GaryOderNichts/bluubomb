#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "socket.h"
#include "imports.h"

static int socket_handle = 0;

int socketInit()
{
	if(socket_handle) return socket_handle;

	int ret = IOS_Open("/dev/socket", 0);

	if(ret > 0)
	{
		socket_handle = ret;
		return socket_handle;
	}

	return ret;
}

int socketExit()
{
	int ret = IOS_Close(socket_handle);

	socket_handle = 0;

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

int	socket(int domain, int type, int protocol)
{
	uint8_t* iobuf = allocIobuf(0xC);
	uint32_t* inbuf = (uint32_t*)iobuf;

	inbuf[0] = domain;
	inbuf[1] = type;
	inbuf[2] = protocol;

	int ret = IOS_Ioctl(socket_handle, 0x11, inbuf, 0xC, NULL, 0);

	freeIobuf(iobuf);
	return ret;
}

int	closesocket(int sockfd)
{
	uint8_t* iobuf = allocIobuf(0x4);
	uint32_t* inbuf = (uint32_t*)iobuf;

	inbuf[0] = sockfd;

	int ret = IOS_Ioctl(socket_handle, 0x3, inbuf, 0x4, NULL, 0);

	freeIobuf(iobuf);
	return ret;
}

int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
	uint8_t* iobuf = allocIobuf(0x18);
	uint32_t* inbuf = (uint32_t*)iobuf;
	uint32_t* outbuf = (uint32_t*)inbuf;

	inbuf[0] = sockfd;

	int ret = -1;

	if(addr && addrlen && *addrlen == 0x10)
	{
		inbuf[5] = *addrlen;

		ret = IOS_Ioctl(socket_handle, 0x1, inbuf, 0x18, outbuf, 0x18);

		if(ret >= 0)
		{
			_memcpy(addr, &outbuf[1], outbuf[5]);
			*addrlen = outbuf[5];
		}
	}else{
		inbuf[5] = 0x10;

		ret = IOS_Ioctl(socket_handle, 0x1, inbuf, 0x18, outbuf, 0x18);
	}

	freeIobuf(iobuf);
	return ret;
}

int	bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	if(addrlen != 0x10) return -1;

	uint8_t* iobuf = allocIobuf(0x18);
	uint32_t* inbuf = (uint32_t*)iobuf;

	inbuf[0] = sockfd;
	_memcpy(&inbuf[1], addr, addrlen);
	inbuf[5] = addrlen;

	int ret = IOS_Ioctl(socket_handle, 0x2, inbuf, 0x18, NULL, 0);

	freeIobuf(iobuf);
	return ret;
}

int	connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
	if(addrlen != 0x10) return -1;

	uint8_t* iobuf = allocIobuf(0x18);
	uint32_t* inbuf = (uint32_t*)iobuf;

	inbuf[0] = sockfd;
	_memcpy(&inbuf[1], addr, addrlen);
	inbuf[5] = addrlen;

	int ret = IOS_Ioctl(socket_handle, 0x4, inbuf, 0x18, NULL, 0);

	freeIobuf(iobuf);
	return ret;
}

int listen(int sockfd, int backlog)
{
	uint8_t* iobuf = allocIobuf(0x8);
	uint32_t* inbuf = (uint32_t*)iobuf;

	inbuf[0] = sockfd;
	inbuf[1] = backlog;

	int ret = IOS_Ioctl(socket_handle, 0xA, inbuf, 0x8, NULL, 0);

	freeIobuf(iobuf);
	return ret;
}

int shutdown(int sockfd, int how)
{
	uint8_t* iobuf = allocIobuf(0x8);
	uint32_t* inbuf = (uint32_t*)iobuf;

	inbuf[0] = sockfd;
	inbuf[1] = how;

	int ret = IOS_Ioctl(socket_handle, 0x10, inbuf, 0x8, NULL, 0);

	freeIobuf(iobuf);
	return ret;
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
	if(!len) return -101;

	// TODO : size checks, split up data into multiple vectors if necessary
	void* data_buf = IOS_AllocAligned(0xCAFF, len, 0x40);
	if(!data_buf) return -100;

	uint8_t* iobuf = allocIobuf(0x38);
	iovec_s* iovec = (iovec_s*)iobuf;
	uint32_t* inbuf = (uint32_t*)&iobuf[0x30];

	inbuf[0] = sockfd;
	inbuf[1] = flags;

	iovec[0].ptr = inbuf;
	iovec[0].len = 0x8;
	iovec[1].ptr = (void*)data_buf;
	iovec[1].len = len;

	int ret = IOS_Ioctlv(socket_handle, 0xC, 1, 3, iovec);

	if(ret > 0 && buf)
	{
		_memcpy(buf, data_buf, ret);
	}

	freeIobuf(data_buf);
	freeIobuf(iobuf);
	return ret;
}

ssize_t send(int sockfd, const void *buf, size_t len, int flags)
{
	if(!buf || !len) return -101;

	// TODO : size checks, split up data into multiple vectors if necessary
	void* data_buf = IOS_AllocAligned(0xCAFF, len, 0x40);
	if(!data_buf) return -100;

	uint8_t* iobuf = allocIobuf(0x38);
	iovec_s* iovec = (iovec_s*)iobuf;
	uint32_t* inbuf = (uint32_t*)&iobuf[0x30];

	_memcpy(data_buf, buf, len);

	inbuf[0] = sockfd;
	inbuf[1] = flags;

	iovec[0].ptr = inbuf;
	iovec[0].len = 0x8;
	iovec[1].ptr = (void*)data_buf;
	iovec[1].len = len;

	int ret = IOS_Ioctlv(socket_handle, 0xE, 4, 0, iovec);

	freeIobuf(data_buf);
	freeIobuf(iobuf);
	return ret;
}
