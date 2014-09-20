#pragma once
#include "xrGameSpy_MainDefs.h"

#include "GameSpy\ServerBrowsing\sb_serverbrowsing.h"

extern "C"
{
	EXPORT_FN_DECL(int, GetQueryVersion, ());
//	EXPORT_FN_DECL(ServerBrowser, ServerBrowserNew, (const gsi_char *queryForGamename, const gsi_char *queryFromGamename, const gsi_char *queryFromKey, int queryFromVersion, int maxConcUpdates, int queryVersion, SBBool bLAN_Only, ServerBrowserCallback callback, void *instance));
	EXPORT_FN_DECL(ServerBrowser, ServerBrowserNewA, (SBBool bLAN_Only, ServerBrowserCallback callback, void *instance));
	EXPORT_FN_DECL(void, ServerBrowserFree, (ServerBrowser sb));
	EXPORT_FN_DECL(void, ServerBrowserClear, (ServerBrowser sb));

	EXPORT_FN_DECL(SBError, ServerBrowserThink, (ServerBrowser sb));
	EXPORT_FN_DECL(SBState, ServerBrowserState, (ServerBrowser sb));
	EXPORT_FN_DECL(void, ServerBrowserHalt, (ServerBrowser sb));
//	EXPORT_FN_DECL(SBError, ServerBrowserUpdate, (ServerBrowser sb, SBBool async, SBBool disconnectOnComplete, const unsigned char *basicFields, int numBasicFields, const gsi_char *serverFilter));
	EXPORT_FN_DECL(SBError, ServerBrowserUpdateA, (ServerBrowser sb, SBBool async, SBBool disconnectOnComplete, const gsi_char *serverFilter));

//	EXPORT_FN_DECL(SBError, ServerBrowserLANUpdate, (ServerBrowser sb, SBBool async, unsigned short startSearchPort, unsigned short endSearchPort));
	EXPORT_FN_DECL(SBError, ServerBrowserLANUpdate, (ServerBrowser sb, SBBool async));

	EXPORT_FN_DECL(int, ServerBrowserCount, (ServerBrowser sb));
	EXPORT_FN_DECL(SBServer, ServerBrowserGetServer, (ServerBrowser sb, int index));
	EXPORT_FN_DECL(SBServer, ServerBrowserGetServerByIPA, (ServerBrowser sb, const gsi_char* ip, unsigned short port));

	
	EXPORT_FN_DECL(char *, SBServerGetPublicAddress, (SBServer server));
	EXPORT_FN_DECL(unsigned short, SBServerGetPublicQueryPort, (SBServer server));
	EXPORT_FN_DECL(const gsi_char *, SBServerGetStringValueA, (SBServer server, const gsi_char *keyname, const gsi_char *def));
	EXPORT_FN_DECL(int, SBServerGetIntValueA, (SBServer server, const gsi_char *key, int idefault));
	EXPORT_FN_DECL(double, SBServerGetFloatValueA, (SBServer server, const gsi_char *key, double fdefault));
	EXPORT_FN_DECL(SBBool, SBServerGetBoolValueA, (SBServer server, const gsi_char *key, SBBool bdefault));
	EXPORT_FN_DECL(int, SBServerGetPing, (SBServer server));

	EXPORT_FN_DECL(SBError, ServerBrowserAuxUpdateServer, (ServerBrowser sb, SBServer server, SBBool async, SBBool fullUpdate));
	EXPORT_FN_DECL(SBError, ServerBrowserAuxUpdateIPA, (ServerBrowser sb, const gsi_char *ip, unsigned short port, SBBool viaMaster, SBBool async, SBBool fullUpdate));
	
	EXPORT_FN_DECL(const gsi_char *, SBServerGetPlayerStringValueA, (SBServer server, int playernum, const gsi_char *key, const gsi_char *sdefault));
	EXPORT_FN_DECL(int, SBServerGetPlayerIntValueA, (SBServer server, int playernum, const gsi_char *key, int idefault));
	EXPORT_FN_DECL(double, SBServerGetPlayerFloatValueA, (SBServer server, int playernum, const gsi_char *key, double fdefault));

	EXPORT_FN_DECL(const gsi_char *, SBServerGetTeamStringValueA, (SBServer server, int teamnum, const gsi_char *key, const gsi_char *sdefault));
	EXPORT_FN_DECL(int, SBServerGetTeamIntValueA, (SBServer server, int teamnum, const gsi_char *key, int idefault));
	EXPORT_FN_DECL(double, SBServerGetTeamFloatValueA, (SBServer server, int teamnum, const gsi_char *key, double fdefault));

	EXPORT_FN_DECL(void, ServerBrowserRemoveIPA, (ServerBrowser sb, const gsi_char *ip, unsigned short port));
	EXPORT_FN_DECL(void, ServerBrowserRemoveServer, (ServerBrowser sb, SBServer server));

	EXPORT_FN_DECL(SBBool, SBServerGetConnectionInfo, (ServerBrowser sb, SBServer server, gsi_u16 PortToConnectTo, char *ipstring));
	EXPORT_FN_DECL(SBBool, SBServerDirectConnect, (SBServer server));
	EXPORT_FN_DECL(void,  ServerBrowserSortA, (ServerBrowser sb, SBBool ascending, const char *sortkey, SBCompareMode comparemode));

	EXPORT_FN_DECL(SBBool, SBServerHasFullKeys, (SBServer server));
	EXPORT_FN_DECL(const gsi_char*, ServerBrowserErrorDescA, (ServerBrowser sb, SBError error));
}