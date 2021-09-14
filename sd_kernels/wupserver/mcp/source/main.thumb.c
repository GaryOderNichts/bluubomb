#include "wupserver.h"
#include "imports.h"

int mcp_receive_message_hook(int queueid, uint32_t* message, uint32_t flags)
{
    wupserver_init();

    return IOS_ReceiveMessage(queueid, message, flags);
}
