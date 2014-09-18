/******
querytest.c
GameSpy Server Browsing SDK
  
Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

******

This program is provided as a diagnostic tool for the Query and Reporting 2 SDK, but is
also provided here as a sample to demonstrate how to query a single server for full rules using
the Server Browisng SDK.

******/

#include "../sb_serverbrowsing.h"
#include "../sb_internal.h"
#include "../../qr2/qr2.h"
#include "../../common/gsAvailable.h"
#include <conio.h>



#define NUM_SIMUL_QUERIES 10

void PrintValues(char *key, char *value, void *instance)
{
	printf("%s = %s\n", key, value);
	GSI_UNUSED(instance);
}


void PrintResultsCallback(ServerBrowser sb, SBCallbackReason reason, SBServer server, void *instance)
{
	if (reason == sbc_serverupdated)
	{
		printf("Got server data: %s:%d [%d]\nKeys/Values:\n",inet_ntoa(*(struct in_addr *)&server->publicip), ntohs(server->publicport), SBServerGetPing(server));
		SBServerEnumKeys(server, PrintValues, NULL);

	} else if (reason == sbc_serverupdatefailed)
	{
		printf("Server update timed out\n");
	}
	GSI_UNUSED(instance);
	GSI_UNUSED(sb);
}

int test_main(int argc, char **argp)
{
	ServerBrowser sb;
	GSIACResult result;
	int totalTime = 0;

	// check that the game's backend is available
	GSIStartAvailableCheck("gmtest");
	while((result = GSIAvailableCheckThink()) == GSIACWaiting)
		msleep(5);
	if(result != GSIACAvailable)
	{
		printf("The backend is not available\n");
		return 1;
	}

	if (argc != 3)
	{
		printf("Invalid parameters!\nQueryTest Usage:\nquerytest.exe [ip] [port]\nExample: querytest.exe 1.2.3.4 28000\n");
		return 1;
	}
	sb = ServerBrowserNew ("gmtest", "gmtest", "HA6zkS", 0, 10, QVERSION_QR2, SBFalse, PrintResultsCallback, NULL);
	// port 11111 for qr2csample
	ServerBrowserAuxUpdateIP(sb, argp[1], (unsigned short)atoi(argp[2]), SBFalse, SBTrue, SBTrue);
	while (totalTime < 5000) {
		ServerBrowserThink(sb);
		msleep(50);
		totalTime += 50;
	}
	ServerBrowserFree(sb);
	GSI_UNUSED(argp);
	GSI_UNUSED(argc);
	return 0;
}
