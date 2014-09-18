/*
GameSpy Peer SDK 
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com
*/

/*************
** INCLUDES **
*************/
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "peerMain.h"
#include "peerOperations.h"
#include "peerSB.h"
#include "peerCallbacks.h"
#include "peerMangle.h"
#include "peerRooms.h"
#include "peerAutoMatch.h"
#include "peerQR.h"

/************
** GLOBALS **
************/
int piSBQueryVersion = QVERSION_QR2;

/**********
** GAMES **
**********/
static void piSBGamesListCallback
(
	SBServerListPtr serverlist,
	SBListCallbackReason reason,
	SBServer server,
	void * instance
)
{
	PEER peer = (PEER)instance;
	PEER_CONNECTION;

#if 0
	{
	static const char reasonStrings[][32] =
	{
		"serveradded",
		"serverupdated",
		"serverdeleted",
		"initiallistcomplete",
		"disconnected",
		"queryerror",
		"publicipdetermined"
	};
	char buffer[256];
	sprintf(buffer, "GAMES  | LIST   | %s", reasonStrings[reason]);
	if(server)
		sprintf(buffer + strlen(buffer), " | %s:%d", SBServerGetPublicAddress(server), SBServerGetPublicQueryPort(server));
	strcat(buffer, "\n");
	OutputDebugString(buffer);
	}
#endif

	// handle based on the callback reason
	switch(reason)
	{
	case slc_serveradded:
		// if it's added call the callback and get it updated
		piAddListingGamesCallback(peer, PEERTrue, server, PEER_ADD);
		if(!SBServerHasBasicKeys(server))
		{
			SBBool usequerychallenge = SBFalse;
			if (serverlist->backendgameflags & QR2_USE_QUERY_CHALLENGE)
				usequerychallenge = SBTrue;
			SBQueryEngineUpdateServer(&connection->gameEngine, server, 0, QTYPE_BASIC, usequerychallenge);
		}
		break;

	case slc_serverupdated:
		// if it's updated let the app know
		piAddListingGamesCallback(peer, PEERTrue, server, PEER_UPDATE);
		break;

	case slc_serverdeleted:
		// if it's deleted, call the callback
		if ((server->state & (STATE_PENDINGBASICQUERY|STATE_PENDINGFULLQUERY|STATE_PENDINGICMPQUERY)) != 0) 
			SBQueryEngineRemoveServerFromFIFOs(&connection->gameEngine, server);
		piAddListingGamesCallback(peer, PEERTrue, server, PEER_REMOVE);
		break;

	case slc_initiallistcomplete:
		// done with the initial list
		connection->initialGameList = PEERFalse;
			
		// let the app know
		piAddListingGamesCallback(peer, PEERTrue, NULL, PEER_COMPLETE);

		break;

	case slc_queryerror:
		// call the failed callback
		piAddListingGamesCallback(peer, PEERFalse, NULL, 0);
		break;
		
	case slc_publicipdetermined:
		// let the engine know the IP
		connection->publicIP = serverlist->mypublicip;
		SBQueryEngineSetPublicIP(&connection->gameEngine, serverlist->mypublicip);
		break;

	default:
		break;
	}
}

static void piSBGamesEngineCallback
(
	SBQueryEnginePtr engine,
	SBQueryEngineCallbackReason reason,
	SBServer server,
	void * instance
)
{
	PEER peer = (PEER)instance;
	//PEER_CONNECTION;

#if 0
	{
	static const char reasonStrings[][32] =
	{
		"updatesuccess",
		"updatefailed",
		"engineidle"
	};
	char buffer[256];
	sprintf(buffer, "GAMES  | ENGINE | %s", reasonStrings[reason]);
	if(server)
		sprintf(buffer + strlen(buffer), " | %s:%d", SBServerGetPublicAddress(server), SBServerGetPublicQueryPort(server));
	strcat(buffer, "\n");
	OutputDebugString(buffer);
	}
#endif

	// if this is an updated server, call the callback
	if(reason == qe_updatesuccess)
		piAddListingGamesCallback(peer, PEERTrue, server, PEER_UPDATE);
		
	GSI_UNUSED(engine);
}

PEERBool piSBStartListingGames
(
	PEER peer,
	const unsigned char * fields,
	int numFields,
	const char * filter
)
{
	char groupFilter[32];
	char smartSpyFilter[MAX_FILTER_LEN];
	char fullFields[MAX_FIELD_LIST_LEN];
	int listLen;
	int keyLen;
	int i;

	PEER_CONNECTION;

	// check that we're initialized.
	assert(connection->sbInitialized);
	if(!connection->sbInitialized)
		return PEERFalse;

	// Stop it if it's going.
	/////////////////////////
	piSBStopListingGames(peer);

	// Clear the list.
	//////////////////
	SBServerListClear(&connection->gameList);

	// Filter by group if in one.
	/////////////////////////////
	if(connection->groupID)
		sprintf(groupFilter, "groupid=%d", connection->groupID);
	else
		strcpy(groupFilter, "groupid is null");

	// Setup the actual filter.
	///////////////////////////
	if(filter)
	{
		sprintf(smartSpyFilter, "(%s) AND (", groupFilter);
		strzcat(smartSpyFilter, filter, sizeof(smartSpyFilter) - 1);
		strcat(smartSpyFilter, ")");
	}
	else
	{
		strcpy(smartSpyFilter, groupFilter);
	}

	// Clear this in case it was set.
	/////////////////////////////////
	connection->gameEngine.numserverkeys = 0;

	// We always set these keys.
	////////////////////////////
	strcpy(fullFields, "\\hostname\\gamemode");
	listLen = (int)strlen(fullFields);
	SBQueryEngineAddQueryKey(&connection->gameEngine, HOSTNAME_KEY);
	SBQueryEngineAddQueryKey(&connection->gameEngine, GAMEMODE_KEY);

	// Build the key list.
	//////////////////////
	for(i = 0 ; i < numFields ; i++)
	{
		// Check that we have the space to add the key.
		///////////////////////////////////////////////
		keyLen = (int)strlen(qr2_registered_key_list[fields[i]]);
		if((listLen + keyLen + 1) >= MAX_FIELD_LIST_LEN)
			break;

		// Add the key.
		///////////////
		listLen += sprintf(fullFields + listLen, "\\%s", qr2_registered_key_list[fields[i]]);

		// Add to the engine query list.
		////////////////////////////////
		SBQueryEngineAddQueryKey(&connection->gameEngine, fields[i]);
	}

	// Start updating the list.
	///////////////////////////
	if(SBServerListConnectAndQuery(&connection->gameList, fullFields, smartSpyFilter, PUSH_UPDATES, 0) != sbe_noerror)
	{
		piSBStopListingGames(peer);
		return PEERFalse;
	}

	// Doing the initial listing.
	/////////////////////////////
	connection->initialGameList = PEERTrue;

	// Clear the list before they start getting servers.
	////////////////////////////////////////////////////
	piAddListingGamesCallback(peer, PEERTrue, NULL, PEER_CLEAR);

	return PEERTrue;
}

void piSBStopListingGames
(
	PEER peer
)
{
	PEER_CONNECTION;

	// check that we're initialized.
	assert(connection->sbInitialized);
	if(!connection->sbInitialized)
		return;

	// Stop the listing.
	////////////////////
	SBServerListDisconnect(&connection->gameList);
	
	// Stop the engine.
	///////////////////
	SBEngineHaltUpdates(&connection->gameEngine);

	// Remove all pending game list callbacks.
	//////////////////////////////////////////
	piClearCallbacks(peer, PI_LISTING_GAMES_CALLBACK);
}

/***********
** GROUPS **
***********/
static void piSBGroupsListCallback
(
	SBServerListPtr serverlist,
	SBListCallbackReason reason,
	SBServer server,
	void * instance
)
{
	piOperation * operation;
	PEER peer = (PEER)instance;
	PEER_CONNECTION;

	operation = connection->listingGroupsOperation;

#if 0
	{
	static const char reasonStrings[][32] =
	{
		"serveradded",
		"serverupdated",
		"serverdeleted",
		"initiallistcomplete",
		"disconnected",
		"queryerror",
		"publicipdetermined"
	};
	char buffer[256];
	sprintf(buffer, "GROUPS | LIST   | %s", reasonStrings[reason]);
	if(server)
		sprintf(buffer + strlen(buffer), " | %d", ntohl(SBServerGetPublicInetAddress(server)));
	strcat(buffer, "\n");
	OutputDebugString(buffer);
	}
#endif

	// handle based on the callback reason
	switch(reason)
	{
	// if it's added call the callback
	case slc_serveradded:
		{
		int groupID = (int)ntohl(SBServerGetPublicInetAddress(server));
//#ifndef GSI_PEER_UNICODE
		const char * name = SBServerGetStringValueA(server, "hostname", "(No Name)");
		int numWaiting = SBServerGetIntValueA(server, "numwaiting", 0);
		int maxWaiting = SBServerGetIntValueA(server, "maxwaiting", 0);
		int numGames = SBServerGetIntValueA(server, "numservers", 0);
		int numPlaying = SBServerGetIntValueA(server, "numplayers", 0);
//#else
//		const unsigned short * name = SBServerGetStringValue(server, L"hostname", L"(No Name)");
//		int numWaiting = SBServerGetIntValue(server, L"numwaiting", 0);
//		int maxWaiting = SBServerGetIntValue(server, L"maxwaiting", 0);
//		int numGames = SBServerGetIntValue(server, L"numservers", 0);
//		int numPlaying = SBServerGetIntValue(server, L"numplayers", 0);
//#endif
		piAddListGroupRoomsCallback(peer, PEERTrue, groupID, server, name, numWaiting, maxWaiting, numGames, numPlaying, (peerListGroupRoomsCallback)operation->callback, operation->callbackParam, operation->ID);
		break;
		}

	// initial list completion
	case slc_initiallistcomplete:
		// calling the terminating callback
		piAddListGroupRoomsCallback(peer, PEERTrue, 0, NULL, NULL, 0, 0, 0, 0, (peerListGroupRoomsCallback)operation->callback, operation->callbackParam, operation->ID);
		piRemoveOperation(peer, operation);
		connection->listingGroupsOperation = NULL;
		break;

	// check for a query error
	case slc_queryerror:
		// call the failed callback
		piAddListGroupRoomsCallback(peer, PEERFalse, 0, NULL, NULL, 0, 0, 0, 0, (peerListGroupRoomsCallback)operation->callback, operation->callbackParam, operation->ID);
		piRemoveOperation(peer, operation);
		connection->listingGroupsOperation = NULL;
		break;

	default:
		break;
	}
	
	GSI_UNUSED(serverlist);
}

PEERBool piSBStartListingGroups
(
	PEER peer,
	const char * fields
)
{
	char fullFields[MAX_FIELD_LIST_LEN + 1];
	int len;

	PEER_CONNECTION;

	// check that we're initialized.
	assert(connection->sbInitialized);
	if(!connection->sbInitialized)
		return PEERFalse;

	// Stop it if it's going.
	/////////////////////////
	piSBStopListingGroups(peer);

	// Clear the list.
	//////////////////
	SBServerListClear(&connection->groupList);

	// Setup the fields list.
	/////////////////////////
	strcpy(fullFields, "\\hostname\\numwaiting\\maxwaiting\\numservers\\numplayers");
	len = (int)strlen(fullFields);
	strzcpy(fullFields + len, fields, (unsigned int)MAX_FIELD_LIST_LEN - len);

	// Start updating the list.
	///////////////////////////
	if(SBServerListConnectAndQuery(&connection->groupList, fullFields, "", SEND_GROUPS, 0) != sbe_noerror)
	{
		piSBStopListingGroups(peer);
		return PEERFalse;
	}

	return PEERTrue;
}

void piSBStopListingGroups
(
	PEER peer
)
{
	PEER_CONNECTION;

	// check that we're initialized.
	assert(connection->sbInitialized);
	if(!connection->sbInitialized)
		return;

	// Stop the listing.
	////////////////////
	SBServerListDisconnect(&connection->groupList);
}

/**************
** AUTOMATCH **
**************/
static PEERBool piIsLocalServer(PEER peer, SBServer server)
{
	PEER_CONNECTION;

	// check the public ip
	if(SBServerGetPublicInetAddress(server) != connection->publicIP)
		return PEERFalse;

	// check the private ip and port
	if(SBServerHasPrivateAddress(server))
	{
		if(SBServerGetPrivateInetAddress(server) != connection->privateIP)
			return PEERFalse;

		if(SBServerGetPrivateQueryPort(server) != connection->autoMatchOperation->port)
			return PEERFalse;
	}
	// check the public port
	else
	{
		if(SBServerGetPublicQueryPort(server) != connection->autoMatchOperation->port)
			return PEERFalse;
	}

	return PEERTrue;
}

static int piSBAutoMatchGetServerRating(PEER peer, SBServer server)
{
	PEER_CONNECTION;
		
	// ignore the local machine
	if(connection->autoMatchReporting && piIsLocalServer(peer, server))
		return 0;

	// check if it got the keys.
	if(!SBServerHasFullKeys(server))
		return 0;
	
	// ignore full servers
#ifndef GSI_UNICODE
	if(SBServerGetIntValue(server, "numplayers", 0) >= SBServerGetIntValue(server, "maxplayers", 0))
		return 0;
#else
	if(SBServerGetIntValue(server, L"numplayers", 0) >= SBServerGetIntValue(server, L"maxplayers", 0))
		return 0;
#endif

	// return the rating for this server	
	return piCallAutoMatchRateCallback(peer, server);
}

static void piSBAutoMatchCheckUpdatedServer(PEER peer, SBServer server)
{
	int rating;

	PEER_CONNECTION;
	
	// only do this if we're waiting
	if(connection->autoMatchStatus != PEERWaiting)
		return;
	
	// if we're already joining a staging room, ignore this
	if(connection->enteringRoom[StagingRoom])
		return;

	// get the rating for this server
	rating = piSBAutoMatchGetServerRating(peer, server);

	// if it's not acceptable, ignore
	if(rating <= 0)
		return;

	// stop reporting and leave the room
	piStopAutoMatchReporting(peer);
	piLeaveRoom(peer, StagingRoom, "");

	// join the match
	if(!piJoinAutoMatchRoom(peer, server))
	{
		// it's bad news if it failed to start the op
		piSetAutoMatchStatus(peer, PEERFailed);
	}
}

static void piSBAutoMatchCheckPushedServer(PEER peer, SBServer server)
{
	SortInfo info;
	int aRating;
	PEER_CONNECTION;
	
	// We must be in the waiting or searching phase to rate a server this way
	if(connection->autoMatchStatus != PEERWaiting && connection->autoMatchStatus != PEERSearching)
		return;

	// if we're already joining a staging room, ignore this
	if(connection->enteringRoom[StagingRoom])
		return;

	// Check if this is the only server in our list	
	if (SBServerListCount(&connection->autoMatchList) == 1)
	{
		// get the rating for this server
		aRating = piSBAutoMatchGetServerRating(peer, server);

		// if it's not acceptable, ignore
		if(aRating <= 0)
			return;

		// stop reporting and leave the room
		piStopAutoMatchReporting(peer);
		piLeaveRoom(peer, StagingRoom, "");

		// join the match
		if(!piJoinAutoMatchRoom(peer, server))
		{
			// it's bad news if it failed to start the op
			piSetAutoMatchStatus(peer, PEERFailed);
		}	
	}

	// otherwise we'll need to run through all the servers 
	// rate each of them and join the one with the highest rating.
	else if (SBServerListCount(&connection->autoMatchList) > 1)
	{
		int count, i;
		SBServer aServer;

		// loop through all the servers.
		count = SBServerListCount(&connection->autoMatchList);
		for(i = (count - 1) ; i >= 0 ; i--)
		{
			// get the server.
			aServer = SBServerListNth(&connection->autoMatchList, i);

			// get the rating for this server
			aRating = piSBAutoMatchGetServerRating(peer, aServer);
			if(aRating <= 0)
			{
				SBServerListRemoveAt(&connection->autoMatchList, i);
				continue;
			}

			// store this server's rating
			SBServerAddIntKeyValue(aServer, PI_AUTOMATCH_RATING_KEY, aRating);
		}

		// recount
		count = SBServerListCount(&connection->autoMatchList);

		// no matches?
		if(!count)
		{
			piSetAutoMatchStatus(peer, PEERWaiting);
			return;
		}

		// sort by rating
#ifdef GSI_UNICODE
		_tcscpy(info.sortkey, (const unsigned short *)PI_AUTOMATCH_RATING_KEY);
#else
		_tcscpy(info.sortkey, PI_AUTOMATCH_RATING_KEY);
#endif
		info.comparemode = sbcm_int;
		SBServerListSort(&connection->autoMatchList, SBTrue, info);
		
		piStopAutoMatchReporting(peer);
		piLeaveRoom(peer, StagingRoom, "");

		// join the top rated match
		if(!piJoinAutoMatchRoom(peer, SBServerListNth(&connection->autoMatchList, 0)))
		{
			// it's bad news if it failed to start the op
			piSetAutoMatchStatus(peer, PEERFailed);
		}
	}

}

static void piSBAutoMatchListCallback
(
	SBServerListPtr serverlist,
	SBListCallbackReason reason,
	SBServer server,
	void * instance
)
{
	PEER peer = (PEER)instance;
	PEER_CONNECTION;

#if 0
	{
	static const char reasonStrings[][32] =
	{
		"serveradded",
		"serverupdated",
		"serverdeleted",
		"initiallistcomplete",
		"disconnected",
		"queryerror",
		"publicipdetermined"
	};
	char buffer[256];
	sprintf(buffer, "AUTOMATCH | LIST   | %s", reasonStrings[reason]);
	if(server)
		sprintf(buffer + strlen(buffer), " | %s:%d", SBServerGetPublicAddress(server), SBServerGetPublicQueryPort(server));
	strcat(buffer, "\n");
	OutputDebugString(buffer);
	}
#endif

	// handle based on the callback reason
	switch(reason)
	{
	case slc_serveradded:
		// If we have the server info, or we already requested it
		// just bail
		if(server->state & (STATE_FULLKEYS|STATE_PENDINGFULLQUERY))
			break;

		// If it's behind a firewall, get the keys from the master
		if (!SBServerDirectConnect(server))
			SBGetServerRulesFromMaster(&connection->autoMatchList, SBServerGetPublicInetAddress(server), SBServerGetPublicQueryPortNBO(server));
		else // send normal update
		{
			SBBool usequerychallenge = SBFalse;
			if (serverlist->backendgameflags & QR2_USE_QUERY_CHALLENGE)
				usequerychallenge = SBTrue;
			SBQueryEngineUpdateServer(&connection->autoMatchEngine, server, 0, QTYPE_FULL, usequerychallenge);
		}
		break;

	case slc_serverupdated:
		// If we don't have the full keys, request them
		if(!SBServerHasFullKeys(server) && SBServerDirectConnect(server) && !(server->state & STATE_PENDINGFULLQUERY))
		{
			// Send a normal query, if the server is firewalled we wouldn't be here
			// assert(SBServerDirectConnect(server))
			SBBool usequerychallenge = SBFalse;
			if (serverlist->backendgameflags & QR2_USE_QUERY_CHALLENGE)
				usequerychallenge = SBTrue;
			SBQueryEngineUpdateServer(&connection->autoMatchEngine, server, 0, QTYPE_FULL, usequerychallenge);
		}
		else if(!SBServerHasFullKeys(server) && !SBServerDirectConnect(server))
		{
			SBGetServerRulesFromMaster(&connection->autoMatchList, SBServerGetPublicInetAddress(server), SBServerGetPublicQueryPortNBO(server));
		}
		else
			piSBAutoMatchCheckPushedServer(peer, server);
		break;

	case slc_serverdeleted:
		// make sure it's not in the update queue
		if ((server->state & (STATE_PENDINGBASICQUERY|STATE_PENDINGFULLQUERY)) != 0) 
			SBQueryEngineRemoveServerFromFIFOs(&connection->autoMatchEngine, server);
		break;

	case slc_initiallistcomplete:
		// if no servers were found, start Waiting
		// or if servers were found but all were behind a firewall, start Waiting
		if(!SBServerListCount(&connection->autoMatchList) ||
			(0==connection->autoMatchEngine.querylist.count))
		{
			piSetAutoMatchStatus(peer, PEERWaiting);
		}
		break;
	case slc_queryerror:
		// browsing failed
		connection->autoMatchSBFailed = PEERTrue;
		if(connection->autoMatchStatus == PEERSearching)
			piSetAutoMatchStatus(peer, connection->autoMatchQRFailed?PEERFailed:PEERWaiting);
		break;

	case slc_publicipdetermined:
		// let the engine know the IP
		connection->publicIP = serverlist->mypublicip;
		SBQueryEngineSetPublicIP(&connection->gameEngine, serverlist->mypublicip);
		break;

	default:
		break;
	}
}

static void piSBAutoMatchEngineCallback
(
	SBQueryEnginePtr engine,
	SBQueryEngineCallbackReason reason,
	SBServer server,
	void * instance
)
{
	PEER peer = (PEER)instance;
	int i;
	int count;
	int rating;
	SortInfo info;

	PEER_CONNECTION;

#if 0
	{
	static const char reasonStrings[][32] =
	{
		"updatesuccess",
		"updatefailed",
		"engineidle"
	};
	char buffer[256];
	sprintf(buffer, "AUTOMATCH | ENGINE | %s", reasonStrings[reason]);
	if(server)
		sprintf(buffer + strlen(buffer), " | %s:%d", SBServerGetPublicAddress(server), SBServerGetPublicQueryPort(server));
	strcat(buffer, "\n");
	OutputDebugString(buffer);
	}
#endif

	// handle based on the callback reason
	switch(reason)
	{
	case qe_updatesuccess:
		// check the server
		piSBAutoMatchCheckUpdatedServer(peer, server);
		break;

	case qe_updatefailed:
		if(!SBServerListCount(&connection->autoMatchList))
			piSetAutoMatchStatus(peer, PEERWaiting);
		break;

	// check if the querying is complete.
	case qe_engineidle:
		// make sure we're searching
		if(connection->autoMatchStatus != PEERSearching)
			return;
		
		// if we're already in or joining a staging room, ignore this
		if(connection->inRoom[StagingRoom] || connection->enteringRoom[StagingRoom])
			return;

		// loop through all the servers.
		count = SBServerListCount(&connection->autoMatchList);
		for(i = (count - 1) ; i >= 0 ; i--)
		{
			// get the server.
			server = SBServerListNth(&connection->autoMatchList, i);

			// get the rating for this server
			rating = piSBAutoMatchGetServerRating(peer, server);
			if(rating <= 0)
			{
				SBServerListRemoveAt(&connection->autoMatchList, i);
				continue;
			}

			// store this server's rating
			SBServerAddIntKeyValue(server, PI_AUTOMATCH_RATING_KEY, rating);
		}

		// recount
		count = SBServerListCount(&connection->autoMatchList);

		// no matches?
		if(!count)
		{
			piSetAutoMatchStatus(peer, PEERWaiting);
			return;
		}

		// sort by rating
#ifdef GSI_UNICODE
		_tcscpy(info.sortkey, (const unsigned short *)PI_AUTOMATCH_RATING_KEY);
#else
		_tcscpy(info.sortkey, PI_AUTOMATCH_RATING_KEY);
#endif
		
		info.comparemode = sbcm_int;
		SBServerListSort(&connection->autoMatchList, SBTrue, info);

		// join the top rated match
		if(!piJoinAutoMatchRoom(peer, SBServerListNth(&connection->autoMatchList, 0)))
		{
			// it's bad news if it failed to start the op
			piSetAutoMatchStatus(peer, PEERFailed);
		}

		break;

	default:
		break;
	}
	
	GSI_UNUSED(engine);
}

PEERBool piSBStartListingAutoMatches
(
	PEER peer
)
{
	PEER_CONNECTION;

	// check that we're initialized.
	assert(connection->sbInitialized);
	if(!connection->sbInitialized)
	{
		connection->autoMatchSBFailed = PEERTrue;
		return PEERFalse;
	}

	// stop it if it's going.
	piSBStopListingAutoMatches(peer);

	// clear the list.
	SBServerListClear(&connection->autoMatchList);

	// start updating the list.
	if(SBServerListConnectAndQuery(&connection->autoMatchList, NULL, connection->autoMatchFilter, PUSH_UPDATES, 0) != sbe_noerror)
	{
		piSBStopListingAutoMatches(peer);
		connection->autoMatchSBFailed = PEERTrue;
		return PEERFalse;
	}

	// clear the error flag
	connection->autoMatchSBFailed = PEERFalse;

	// we're browsing
	connection->autoMatchBrowsing = PEERTrue;

	return PEERTrue;
}

void piSBStopListingAutoMatches
(
	PEER peer
)
{
	PEER_CONNECTION;

	// check that we're initialized.
	assert(connection->sbInitialized);
	if(!connection->sbInitialized)
		return;

	// we're done browsing
	connection->autoMatchBrowsing = PEERFalse;

	// stop the listing.
	SBServerListDisconnect(&connection->autoMatchList);
	
	// stop the engine.
	SBEngineHaltUpdates(&connection->autoMatchEngine);
}

/**************
** FUNCTIONS **
**************/
PEERBool piSBInit
(
	PEER peer
)
{
	char autoMatchTitle[PI_TITLE_MAX_LEN];

	PEER_CONNECTION;

	// init the game list and engine
	memset(&connection->gameList, 0, sizeof(connection->gameList));
	SBServerListInit(&connection->gameList, connection->title, connection->sbName, connection->sbSecretKey, connection->sbGameVersion, SBFalse, piSBGamesListCallback, peer);
	SBQueryEngineInit(&connection->gameEngine, connection->sbMaxUpdates, piSBQueryVersion, SBFalse, piSBGamesEngineCallback, peer);

	// init the group list
	memset(&connection->groupList, 0, sizeof(connection->groupList));
	SBServerListInit(&connection->groupList, connection->title, connection->sbName, connection->sbSecretKey, connection->sbGameVersion, SBFalse, piSBGroupsListCallback, peer);

	// init the AutoMatch list and engine
	strzcpy(autoMatchTitle, connection->title, sizeof(autoMatchTitle));
	strzcat(autoMatchTitle, "am", sizeof(autoMatchTitle));
	memset(&connection->autoMatchList, 0, sizeof(connection->autoMatchList));
	SBServerListInit(&connection->autoMatchList, autoMatchTitle, connection->sbName, connection->sbSecretKey, connection->sbGameVersion, SBFalse, piSBAutoMatchListCallback, peer);
	SBQueryEngineInit(&connection->autoMatchEngine, connection->sbMaxUpdates, piSBQueryVersion, SBFalse, piSBAutoMatchEngineCallback, peer);

	// no host server yet
	connection->hostServer = NULL;

	// initialized
	connection->sbInitialized = PEERTrue;

	return PEERTrue;
}

void piSBCleanup
(
	PEER peer
)
{
	PEER_CONNECTION;

	// stop listing
	piSBStopListingGames(peer);
	piSBStopListingGroups(peer);

	// cleanup the SB
	if(connection->sbInitialized)
	{
		SBServerListCleanup(&connection->gameList);
		SBEngineCleanup(&connection->gameEngine);
		SBServerListCleanup(&connection->groupList);
		SBServerListCleanup(&connection->autoMatchList);
		SBEngineCleanup(&connection->autoMatchEngine);
	}

	// AutoMatch cleanup
	connection->autoMatchBrowsing = PEERFalse;
}


void piSBUpdateGame
(
	PEER peer,
	SBServer server,
	PEERBool fullUpdate, 
	PEERBool forceUpdateByMaster,
	PEERBool icmpEcho
)
{
	PEER_CONNECTION;

	// check that we're initialized.
	assert(connection->sbInitialized);
	
	if(!connection->sbInitialized)
		return;
	
	// Changed 08-26-2004
	// Saad Nader 
	// Added force update by master server parameter 
	// for internal update function
	/////////////////////
	if (forceUpdateByMaster)
	{
		// remove from the existing update lists if present
		if(server->flags & UNSOLICITED_UDP_FLAG)
			SBQueryEngineRemoveServerFromFIFOs(&connection->gameEngine, server);
		
		// Obtain the information via the master server
		SBGetServerRulesFromMaster(&connection->gameList, server->publicip, server->publicport);
		return;
	}

	if (icmpEcho && (server->flags & ICMP_IP_FLAG))
	{
		SBBool usequerychallenge = SBFalse;
		if (connection->gameList.backendgameflags & QR2_USE_QUERY_CHALLENGE)
			usequerychallenge = SBTrue;

		if (server->flags & UNSOLICITED_UDP_FLAG)
			SBQueryEngineRemoveServerFromFIFOs(&connection->gameEngine, server);

		SBQueryEngineUpdateServer(&connection->gameEngine, server, 1, QTYPE_ICMP, usequerychallenge);
		return;
	}
	// if the server supports unsolicited UDP, query it directly
	if(server->flags & UNSOLICITED_UDP_FLAG)
	{
		SBBool usequerychallenge = SBFalse;
		if (connection->gameList.backendgameflags & QR2_USE_QUERY_CHALLENGE)
			usequerychallenge = SBTrue;

		// remove from the existing update lists if present
		SBQueryEngineRemoveServerFromFIFOs(&connection->gameEngine, server);

		// query the server
		SBQueryEngineUpdateServer(&connection->gameEngine, server, 1, fullUpdate?QTYPE_FULL:QTYPE_BASIC, usequerychallenge);
	}
	else
	{
		// get the values from the master server
		SBGetServerRulesFromMaster(&connection->gameList, server->publicip, server->publicport);
	}
}

void piSBThink
(
	PEER peer
)
{
	PEER_CONNECTION;

	// check that we're initialized.
	if(!connection->sbInitialized)
		return;

	SBListThink(&connection->gameList);
	SBListThink(&connection->groupList);
	SBQueryEngineThink(&connection->gameEngine);
	SBListThink(&connection->autoMatchList);
	SBQueryEngineThink(&connection->autoMatchEngine);
}

void piSendNatNegotiateCookie
(
	PEER peer,
	unsigned int ip,
	unsigned short port,
	int cookie
)
{
	PEER_CONNECTION;

	// check that we're initialized.
	if(!connection->sbInitialized)
		return;

	// send the cookie
	if (peerIsAutoMatching(peer))
	{
		SBSendNatNegotiateCookieToServer(&connection->autoMatchList, ip, htons(port), cookie);
	}
	else
		SBSendNatNegotiateCookieToServer(&connection->gameList, ip, htons(port), cookie);
}

void piSendMessageToServer
(
	PEER peer,
	unsigned int ip,
	unsigned short port,
	const char * data,
	int len
)
{
	PEER_CONNECTION;

	// check that we're initialized.
	if(!connection->sbInitialized)
		return;

	// send the message
	if(peerIsAutoMatching(peer))
	{
		SBSendMessageToServer(&connection->autoMatchList, ip, port, data, len);
	}
	else
	{
		SBSendMessageToServer(&connection->gameList, ip, port, data, len);
	}
}

static void piSBCloneServerTableMap(void * elem, void * clientData)
{
	SBKeyValuePair * kvPair = (SBKeyValuePair *)elem;
	SBServer clone = (SBServer)clientData;

	SBServerAddKeyValue(clone, kvPair->key, kvPair->value);
}

SBServer piSBCloneServer(SBServer server)
{
	SBServer clone;
	HashTable table;

	// allocate the clone
	clone = SBAllocServer(NULL, server->publicip, server->publicport);

	// save off the table
	table = clone->keyvals;

	// copy the source server
	memcpy(clone, server, sizeof(struct _SBServer));
	clone->keyvals = table;
	clone->next = NULL;

	// copy all the values from the original table
	TableMap(server->keyvals, piSBCloneServerTableMap, clone);

	return clone;
}

void piSBFreeHostServer(PEER peer)
{
	PEER_CONNECTION;

	if(!connection->hostServer)
		return;

	SBServerFree(&connection->hostServer);
	connection->hostServer = NULL;
}
