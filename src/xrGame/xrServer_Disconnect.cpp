#include "stdafx.h"
#include "xrServer.h"

void xrServer::Disconnect()
{
	IPureServer::Disconnect	();
	SLS_Clear				();
	xr_delete				(game);
}
