#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "imports.h"
#include "net_ifmgr_ncl.h"
#include "socket.h"
#include "wupserver.h"

static int serverKilled;
static int serverSocket;
static uint8_t threadStack[0x1000] __attribute__((aligned(0x20)));

// overwrites command_buffer with response
// returns length of response (or 0 for no response, negative for error)
static int serverCommandHandler(uint32_t* command_buffer, uint32_t length)
{
	if(!command_buffer || !length) return -1;

	int out_length = 4;

	switch(command_buffer[0])
	{
		case 0:
			// write
			// [cmd_id][addr]
			{
				void* dst = (void*)command_buffer[1];

				_memcpy(dst, &command_buffer[2], length - 8);
			}
			break;
		case 1:
			// read
			// [cmd_id][addr][length]
			{
				void* src = (void*)command_buffer[1];
				length = command_buffer[2];

				_memcpy(&command_buffer[1], src, length);
				out_length = length + 4;
			}
			break;
		case 2:
			// svc
			// [cmd_id][svc_id]
			{
				int svc_id = command_buffer[1];
				int size_arguments = length - 8;

				uint32_t arguments[8];
				_memset(arguments, 0x00, sizeof(arguments));
				_memcpy(arguments, &command_buffer[2], (size_arguments < 8 * 4) ? size_arguments : (8 * 4));

				// return error code as data
				out_length = 8;
				command_buffer[1] = ((int (*const)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t))(MCP_SVC_BASE + svc_id * 8))
                    (arguments[0], arguments[1], arguments[2], arguments[3], arguments[4], arguments[5], arguments[6], arguments[7]);
			}
			break;
		case 3:
			// kill
			// [cmd_id]
			{
				serverKilled = 1;
				wupserver_deinit();
			}
			break;
		case 4:
			// memcpy
			// [dst][src][size]
			{
				void* dst = (void*)command_buffer[1];
				void* src = (void*)command_buffer[2];
				int size = command_buffer[3];

				_memcpy(dst, src, size);
			}
			break;
		case 5:
			// repeated-write
			// [address][value][n]
			{
				uint32_t* dst = (uint32_t*)command_buffer[1];
				uint32_t* cache_range = (uint32_t*)(command_buffer[1] & ~0xFF);
				uint32_t value = command_buffer[2];
				uint32_t n = command_buffer[3];

				uint32_t old = *dst;
				int i;
				for(i = 0; i < n; i++)
				{
					if(*dst != old)
					{
						if(*dst == 0x0) old = *dst;
						else
						{
							*dst = value;
							IOS_FlushDCache(cache_range, 0x100);
							break;
						}
					}else
					{
						IOS_InvalidateDCache(cache_range, 0x100);
						usleep(50);
					}
				}
			}
			break;
		default:
			// unknown command
			return -2;
			break;
	}

	// no error !
	command_buffer[0] = 0x00000000;
	return out_length;
}

static void serverClientHandler(int sock)
{
	uint32_t command_buffer[0x180];

	while(!serverKilled)
	{
		int ret = recv(sock, command_buffer, sizeof(command_buffer), 0);

		if(ret <= 0) break;

		ret = serverCommandHandler(command_buffer, ret);

		if(ret > 0)
		{
			send(sock, command_buffer, ret, 0);
		}else if(ret < 0)
		{
			send(sock, &ret, sizeof(int), 0);
		}
	}

	closesocket(sock);
}

static void serverListenClients()
{
	serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	struct sockaddr_in server;

	_memset(&server, 0x00, sizeof(server));

	server.sin_family = AF_INET;
	server.sin_port = 1337;
	server.sin_addr.s_addr = 0;

	if(bind(serverSocket, (struct sockaddr *)&server, sizeof(server)) < 0)
    {
        closesocket(serverSocket);
        return;
    }

    if(listen(serverSocket, 1) < 0)
    {
        closesocket(serverSocket);
        return;
    }

	while(!serverKilled)
	{
        int csock = accept(serverSocket, NULL, NULL);
        if(csock < 0)
            break;

        serverClientHandler(csock);
	}

	closesocket(serverSocket);
	serverSocket = -1;
}

static int wupserver_thread(void *arg)
{
	while(ifmgrnclInit() <= 0)
	{
		usleep(1000);
	}

	while(1)
	{
		uint16_t out0, out1;

		int ret0 = IFMGRNCL_GetInterfaceStatus(0, &out0);
		if(!ret0 && out0 == 1) break;

		int ret1 = IFMGRNCL_GetInterfaceStatus(1, &out1);
		if(!ret1 && out1 == 1) break;

		usleep(1000);
	}

	while(socketInit() <= 0)
	{
		usleep(1000);
	}

	usleep(5*1000*1000);

	while(1)
	{
	    if(!serverKilled)
        {
	        serverListenClients();
        }
        else
        {
            break;
        }
		usleep(1000*1000);
	}

	return 0;
}

static int wupserver_started = 0;

void wupserver_init(void)
{
	if (wupserver_started) {
		return;
	}
	
	wupserver_started = 1;

	serverSocket = -1;
	serverKilled = 0;

	int threadId = IOS_CreateThread(wupserver_thread, 0, (uint32_t*)(threadStack + sizeof(threadStack)), sizeof(threadStack), 0x78, 1);
	if(threadId >= 0)
		IOS_StartThread(threadId);
}

void wupserver_deinit(void)
{
    serverKilled = 1;
    shutdown(serverSocket, SHUT_RDWR);
}
