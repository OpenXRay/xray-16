#include "stdafx.h"
#include "windows.h"
#include "xrGameSpy_MainDefs.h"

#include "xrGameSpy_ServerBrowser.h"

#include "gamespy/qr2/qr2regkeys.h"

#define GAMETYPE_NAME_KEY						100
#define	DEDICATED_KEY							101
#define	G_USER_PASSWORD_KEY						135

static unsigned char Fields_Of_Interest[] = 
{ 
		HOSTNAME_KEY, 
		HOSTPORT_KEY,
		NUMPLAYERS_KEY, 
		MAXPLAYERS_KEY, 
		MAPNAME_KEY, 
		GAMETYPE_KEY, 
		GAMEVER_KEY,
		PASSWORD_KEY,
		G_USER_PASSWORD_KEY,
		DEDICATED_KEY,
		GAMETYPE_NAME_KEY
};

XRGAMESPY_API int xrGS_GetQueryVersion ()
{
	return QVERSION_QR2;
}

//XRGAMESPY_API ServerBrowser xrGS_ServerBrowserNew(const gsi_char *queryForGamename, const gsi_char *queryFromGamename, const gsi_char *queryFromKey, int queryFromVersion, int maxConcUpdates, int queryVersion, SBBool bLAN_Only, ServerBrowserCallback callback, void *instance)
XRGAMESPY_API ServerBrowser xrGS_ServerBrowserNewA(SBBool bLAN_Only, ServerBrowserCallback callback, void *instance)
{
//	return ServerBrowserNew(queryForGamename, queryFromGamename, queryFromKey, queryFromVersion, maxConcUpdates, queryVersion, bLAN_Only, callback, instance);
	char SecretKey[16];
	FillSecretKey(SecretKey);
	return ServerBrowserNewA(GAMESPY_GAMENAME, GAMESPY_GAMENAME, SecretKey, 0, GAMESPY_BROWSER_MAX_UPDATES, xrGS_GetQueryVersion(), bLAN_Only, callback, instance);
}

XRGAMESPY_API void xrGS_ServerBrowserFree (ServerBrowser sb)
{
	ServerBrowserFree (sb);
};

XRGAMESPY_API void xrGS_ServerBrowserClear(ServerBrowser sb)
{
	ServerBrowserClear(sb);
};

XRGAMESPY_API SBError xrGS_ServerBrowserThink(ServerBrowser sb)
{
	return ServerBrowserThink(sb);
};

XRGAMESPY_API SBState xrGS_ServerBrowserState(ServerBrowser sb)
{
	return ServerBrowserState(sb);
};

XRGAMESPY_API void xrGS_ServerBrowserHalt(ServerBrowser sb)
{
	ServerBrowserHalt(sb);
};

//XRGAMESPY_API SBError xrGS_ServerBrowserUpdate (ServerBrowser sb, SBBool async, SBBool disconnectOnComplete, const unsigned char *basicFields, int numBasicFields, const gsi_char *serverFilter)
XRGAMESPY_API SBError xrGS_ServerBrowserUpdateA(ServerBrowser sb, SBBool async, SBBool disconnectOnComplete, const gsi_char *serverFilter)
{
	
	int numBasicFields = sizeof(Fields_Of_Interest)/sizeof(Fields_Of_Interest[0]);
	return ServerBrowserUpdateA(sb, async, disconnectOnComplete, Fields_Of_Interest, numBasicFields, serverFilter);
};

XRGAMESPY_API void xrGS_ServerBrowserSortA(ServerBrowser sb, SBBool ascending, const char *sortkey, SBCompareMode comparemode)
{
	ServerBrowserSortA(sb, ascending, sortkey, comparemode);
};

//XRGAMESPY_API SBError xrGS_ServerBrowserLANUpdate(ServerBrowser sb, SBBool async, unsigned short startSearchPort, unsigned short endSearchPort)
XRGAMESPY_API SBError xrGS_ServerBrowserLANUpdate(ServerBrowser sb, SBBool async)
{
	return ServerBrowserLANUpdate(sb, async,  START_PORT_LAN,  END_PORT_LAN);
}

XRGAMESPY_API int xrGS_ServerBrowserCount (ServerBrowser sb)
{
	return ServerBrowserCount (sb);
};

XRGAMESPY_API SBServer xrGS_ServerBrowserGetServer (ServerBrowser sb, int index)
{
	return ServerBrowserGetServer(sb, index);
}
XRGAMESPY_API SBServer xrGS_ServerBrowserGetServerByIPA(ServerBrowser sb, const gsi_char* ip, unsigned short port)
{
	return ServerBrowserGetServerByIPA(sb, ip, port);
};

XRGAMESPY_API char * xrGS_SBServerGetPublicAddress(SBServer server)
{
	return SBServerGetPublicAddress(server);
};
XRGAMESPY_API unsigned short xrGS_SBServerGetPublicQueryPort(SBServer server)
{
	return SBServerGetPublicQueryPort(server);
};
XRGAMESPY_API const gsi_char * xrGS_SBServerGetStringValueA(SBServer server, const gsi_char *keyname, const gsi_char *def)
{
	return SBServerGetStringValueA(server, keyname, def);
};
XRGAMESPY_API int xrGS_SBServerGetIntValueA(SBServer server, const gsi_char *key, int idefault)
{
	return SBServerGetIntValueA(server, key, idefault);
};
XRGAMESPY_API double xrGS_SBServerGetFloatValueA(SBServer server, const gsi_char *key, double fdefault)
{
	return SBServerGetFloatValueA(server, key, fdefault);
};
XRGAMESPY_API SBBool xrGS_SBServerGetBoolValueA(SBServer server, const gsi_char *key, SBBool bdefault)
{
	return SBServerGetBoolValueA(server, key, bdefault);
};
XRGAMESPY_API int xrGS_SBServerGetPing(SBServer server)
{
	return SBServerGetPing(server);
};

XRGAMESPY_API SBError xrGS_ServerBrowserAuxUpdateServer(ServerBrowser sb, SBServer server, SBBool async, SBBool fullUpdate)
{
	return ServerBrowserAuxUpdateServer(sb, server, async, fullUpdate);
}

XRGAMESPY_API SBError xrGS_ServerBrowserAuxUpdateIPA(ServerBrowser sb, const gsi_char *ip, unsigned short port, SBBool viaMaster, SBBool async, SBBool fullUpdate)
{
	return ServerBrowserAuxUpdateIPA(sb, ip, port, viaMaster, async, fullUpdate);
}

XRGAMESPY_API const gsi_char * xrGS_SBServerGetPlayerStringValueA(SBServer server, int playernum, const gsi_char *key, const gsi_char *sdefault)
{
	return SBServerGetPlayerStringValueA(server, playernum, key, sdefault);
}
XRGAMESPY_API int xrGS_SBServerGetPlayerIntValueA(SBServer server, int playernum, const gsi_char *key, int idefault)
{
	return SBServerGetPlayerIntValueA(server, playernum, key, idefault);
}
XRGAMESPY_API double xrGS_SBServerGetPlayerFloatValueA(SBServer server, int playernum, const gsi_char *key, double fdefault)
{
	return SBServerGetPlayerFloatValueA(server, playernum, key, fdefault);
}

XRGAMESPY_API const gsi_char *xrGS_SBServerGetTeamStringValueA(SBServer server, int teamnum, const gsi_char *key, const gsi_char *sdefault)
{
	return SBServerGetTeamStringValueA(server, teamnum, key, sdefault);
}
XRGAMESPY_API int xrGS_SBServerGetTeamIntValueA(SBServer server, int teamnum, const gsi_char *key, int idefault)
{
	return SBServerGetTeamIntValueA(server, teamnum, key, idefault);
}
XRGAMESPY_API double xrGS_SBServerGetTeamFloatValueA(SBServer server, int teamnum, const gsi_char *key, double fdefault)
{
	return SBServerGetTeamFloatValueA(server, teamnum, key, fdefault);
}

XRGAMESPY_API void xrGS_ServerBrowserRemoveIPA(ServerBrowser sb, const gsi_char *ip, unsigned short port)
{
	ServerBrowserRemoveIPA(sb, ip, port);
}
XRGAMESPY_API void xrGS_ServerBrowserRemoveServer(ServerBrowser sb, SBServer server)
{
	ServerBrowserRemoveServer(sb, server);
}

XRGAMESPY_API SBBool  xrGS_SBServerGetConnectionInfo(ServerBrowser sb, SBServer server, gsi_u16 PortToConnectTo, char *ipstring)
{
	return SBServerGetConnectionInfo( sb, server, PortToConnectTo, ipstring);
};

XRGAMESPY_API SBBool xrGS_SBServerDirectConnect (SBServer server)
{
	return SBServerDirectConnect (server);
};

XRGAMESPY_API SBBool xrGS_SBServerHasFullKeys (SBServer server)
{
	return SBServerHasFullKeys(server);
}

XRGAMESPY_API const gsi_char* xrGS_ServerBrowserErrorDescA(ServerBrowser sb, SBError error)
{
	return ServerBrowserErrorDescA(sb, error);
}