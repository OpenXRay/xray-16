#include "stdafx.h"
#include "xrServer.h"

void xrServer::Disconnect()
{
	SLS_Clear();
	xr_delete(game);
}
