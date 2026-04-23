#include "stdafx.h"
#include "IClient.h"
#include "BaseServer.h"

IClient::IClient(CTimer* timer): stats(timer), server(NULL)
{
	dwTime_LastUpdate = 0;
	flags.bLocal = FALSE;
	flags.bConnected = FALSE;
	flags.bReconnect = FALSE;
	flags.bVerified = TRUE;
}

IClient::~IClient()
{
}

void IClient::_SendTo_LL(const void* data, u32 size, u32 _flags, u32 timeout)
{
	R_ASSERT(server);
	server->BaseServer::SendTo_LL(ID, const_cast<void*>(data), size, _flags, timeout);
}
