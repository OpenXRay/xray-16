/*
gpi.c
GameSpy Presence SDK 
Dan "Mr. Pants" Schoenblum

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

***********************************************************************
Please see the GameSpy Presence SDK documentation for more information
**********************************************************************/

//INCLUDES
//////////

#ifdef XRAY_DISABLE_GAMESPY_WARNINGS
#pragma warning(disable: 4244) //lines: 345, 362
#endif //#ifdef XRAY_DISABLE_GAMESPY_WARNINGS

#include <stdlib.h>
#include <string.h>
#include "gpi.h"

// DEFINES
//////////
#define KEEPALIVE_TIMEOUT (60 * 2000)

// This is so VisualAssist will know about these functions.
///////////////////////////////////////////////////////////
#if 0
void MD5Init(MD5_CTX *);
void MD5Update(MD5_CTX *, unsigned char *, unsigned int);
void MD5Final(unsigned char [16], MD5_CTX *);
void MD5Print(unsigned char [16], char[33]);
void MD5Digest(unsigned char *, unsigned int, char[33]);
#endif

//FUNCTIONS
///////////
GPResult
gpiInitialize(
  GPConnection * connection,
  int productID,
  int namespaceID,
  int partnerID
)
{
	GPIConnection * iconnection;
	int i;
	GPResult result;

	// Set the connection to NULL in case of error.
	///////////////////////////////////////////////
	*connection = NULL;

	// Allocate the connection.
	///////////////////////////
	iconnection = (GPIConnection *)gsimalloc(sizeof(GPIConnection));
	if(iconnection == NULL)
		return GP_MEMORY_ERROR;

	// Initialize connection-specific variables.
	////////////////////////////////////////////
	memset(iconnection, 0, sizeof(GPIConnection));
	iconnection->errorString[0] = '\0';
	iconnection->errorCode = (GPErrorCode)0;
	iconnection->infoCaching = GPITrue;
	iconnection->infoCachingBuddyAndBlockOnly = GPIFalse;
	iconnection->simulation = GPIFalse;
	iconnection->firewall = GPIFalse;
	iconnection->productID = productID;
	iconnection->namespaceID = namespaceID;
	iconnection->partnerID = partnerID;

#ifdef GSI_UNICODE
	iconnection->errorString_W[0] = '\0';
#endif

	if(!gpiInitProfiles((GPConnection *)&iconnection))
	{
		freeclear(iconnection);
		return GP_MEMORY_ERROR;
	}
	iconnection->diskCache = NULL;
	for(i = 0 ; i < GPI_NUM_CALLBACKS ; i++)
	{
		iconnection->callbacks[i].callback = NULL;
		iconnection->callbacks[i].param = NULL;
	}

	// Reset connection-specific stuff.
	///////////////////////////////////
	result = gpiReset((GPConnection *)&iconnection);
	if(result != GP_NO_ERROR)
	{
		gpiDestroy((GPConnection *)&iconnection);
		return result;
	}

	// Initialize the sockets library.
	//////////////////////////////////
	SocketStartUp();

	// Seed the random number generator.
	////////////////////////////////////
	srand((unsigned int)current_time());

#ifndef NOFILE
	// Load profiles cached on disk.
	////////////////////////////////
	result = gpiLoadDiskProfiles((GPConnection *)&iconnection);
	if(result != GP_NO_ERROR)
	{
		gpiDestroy((GPConnection *)&iconnection);
		return result;
	}
#endif

#ifndef NOFILE
	result = gpiInitTransfers((GPConnection *)&iconnection);
	if(result != GP_NO_ERROR)
	{
		gpiDestroy((GPConnection *)&iconnection);
		return result;
	}
#endif

	// Set the connection.
	//////////////////////
	*connection = (GPConnection)iconnection;

	return GP_NO_ERROR;
}

void
gpiDestroy(
  GPConnection * connection
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;

	// Cleanup connection-specific stuff.
	/////////////////////////////////////
	gpiDisconnect(connection, GPITrue);
	gpiStatusInfoKeysDestroy(connection);

#ifdef _PS3
    // Destroy NP
    /////////////
    if (iconnection->npInitialized)
        gpiDestroyNpBasic(connection);
#endif

#ifndef NOFILE
	// Write the profile info to disk.
	//     BD - Don't update if we never connected.
	//////////////////////////////////
	if(iconnection->infoCaching && iconnection->connectState != GPI_NOT_CONNECTED)
	{
		if(gpiSaveDiskProfiles(connection) != GP_NO_ERROR)
		{
			gsDebugFormat(GSIDebugCat_GP, GSIDebugType_File, GSIDebugLevel_HotError,
				"Error saving profiles to disk.");
		}
	}
#endif

	// Free the profile list.
	/////////////////////////
	TableFree(iconnection->profileList.profileTable);

#ifndef NOFILE
	// Free the transfers.
	//////////////////////
	gpiCleanupTransfers(connection);
#endif

	// Free the memory.
	///////////////////
	freeclear(iconnection);

	// Set the connection pointer to NULL.
	//////////////////////////////////////
	*connection = NULL;
}

static GPIBool
gpiResetProfile(
  GPConnection * connection,
  GPIProfile * profile,
  void * data
)
{
	GSI_UNUSED(connection);
	GSI_UNUSED(data);
    
	profile->buddyStatus = NULL;
	profile->buddyStatusInfo = NULL;
	profile->authSig = NULL;
	profile->requestCount = 0;
	profile->peerSig = NULL;
    profile->blocked = gsi_false;
    profile->buddyOrBlockCache = gsi_false;

	return GPITrue;
}

GPResult
gpiReset(
  GPConnection * connection
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GPResult result;
	iconnection->nick[0] = '\0';
	iconnection->uniquenick[0] = '\0';
	iconnection->email[0] = '\0';
	iconnection->cmSocket = INVALID_SOCKET;
	iconnection->connectState = GPI_NOT_CONNECTED;

	iconnection->socketBuffer.len = 0;
	iconnection->socketBuffer.pos = 0;
	iconnection->socketBuffer.size = 0;
	freeclear(iconnection->socketBuffer.buffer);
	iconnection->socketBuffer.buffer = NULL;

	iconnection->inputBufferSize = 0;
	freeclear(iconnection->inputBuffer);
	iconnection->inputBuffer = NULL;

	iconnection->outputBuffer.len = 0;
	iconnection->outputBuffer.pos = 0;
	iconnection->outputBuffer.size = 0;
	freeclear(iconnection->outputBuffer.buffer);
	iconnection->outputBuffer.buffer = NULL;

	iconnection->updateproBuffer.len = 0;
	iconnection->updateproBuffer.pos = 0;
	iconnection->updateproBuffer.size = 0;
	freeclear(iconnection->updateproBuffer.buffer);
	iconnection->updateproBuffer.buffer = NULL;

	iconnection->updateuiBuffer.len = 0;
	iconnection->updateuiBuffer.pos = 0;
	iconnection->updateuiBuffer.size = 0;
	freeclear(iconnection->updateuiBuffer.buffer);
	iconnection->updateuiBuffer.buffer = NULL;
	gpiStatusInfoKeysDestroy(connection);
	result = gpiStatusInfoKeysInit((GPConnection *)&iconnection);
	if (result != GP_NO_ERROR)
	{
		gpiDestroy((GPConnection *)&iconnection);
		return result;
	}
	//iconnection->peerSocket = INVALID_SOCKET;
	iconnection->nextOperationID = 2;
	while(iconnection->operationList != NULL)
		gpiRemoveOperation(connection, iconnection->operationList);
	iconnection->operationList = NULL;
	iconnection->profileList.numBuddies = 0;
    iconnection->profileList.numBlocked = 0;
	gpiProfileMap(connection, gpiResetProfile, NULL);
	iconnection->userid = 0;
	iconnection->profileid = 0;
	iconnection->sessKey = 0;
	iconnection->numSearches = 0;
	iconnection->fatalError = GPIFalse;
	iconnection->peerList = NULL;
	iconnection->lastStatusState = (GPEnum)-1;
	iconnection->lastStatusString[0] = '\0';
	iconnection->lastLocationString[0] = '\0';
	iconnection->kaTransmit = 0;

#ifdef GSI_UNICODE
	iconnection->nick_W[0] = '\0';
	iconnection->uniquenick_W[0] = '\0';
	iconnection->email_W[0] = '\0';
	iconnection->lastStatusString_W[0] = '\0';
	iconnection->lastLocationString_W[0] = '\0';
#endif

	return GP_NO_ERROR;
}

GPResult
gpiProcessConnectionManager(
  GPConnection * connection
)
{
	char * next;
	char * str;
	int id;
	GPIOperation * operation;
	char * tempPtr;
	int len;
	GPIBool connClosed = GPIFalse;
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GPResult result;
	GPIBool loop;
	gsi_time now = current_time();

	// Loop through the rest while waiting for any blocking operations.
	///////////////////////////////////////////////////////////////////
	do
	{
		// Add any waiting info to the output buffer.
		/////////////////////////////////////////////
		gpiAddLocalInfo(connection, &iconnection->outputBuffer);

		// Send anything that needs to be sent.
		///////////////////////////////////////
		if ( iconnection->outputBuffer.len > 0 )
			iconnection->kaTransmit = now;	// data already being transmitted. We don't need to send keep alives
		CHECK_RESULT(gpiSendFromBuffer(connection, iconnection->cmSocket, &iconnection->outputBuffer, &connClosed, GPITrue, "CM"));

		// Read everything the connection manager sent.
		///////////////////////////////////////////////
		result = gpiRecvToBuffer(connection, iconnection->cmSocket, &iconnection->socketBuffer, &len, &connClosed, "CM");
		if(result != GP_NO_ERROR)
		{
			if(result == GP_NETWORK_ERROR)
				CallbackFatalError(connection, GP_NETWORK_ERROR, GP_NETWORK, "There was an error reading from the server.");
			
			return result;
		}

		// Check if we have a completed command.
		////////////////////////////////////////
		while((next = strstr(iconnection->socketBuffer.buffer, "\\final\\")) != NULL)
		{
			// Received command. Connection is still valid
			//////////////////////////////////////////////
			iconnection->kaTransmit = now;

			// NUL terminate the command.
			/////////////////////////////
			next[0] = '\0';

			gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_RawDump,
				"CMD: %s\n", iconnection->socketBuffer.buffer);

			// Copy the command to the input buffer.
			////////////////////////////////////////
			len = (next - iconnection->socketBuffer.buffer);
			if(len > iconnection->inputBufferSize)
			{
				iconnection->inputBufferSize += max(GPI_READ_SIZE, len);
				tempPtr = (char*)gsirealloc(iconnection->inputBuffer, (unsigned int)iconnection->inputBufferSize + 1);
				if(tempPtr == NULL)
					Error(connection, GP_MEMORY_ERROR, "Out of memory.");
				iconnection->inputBuffer = tempPtr;
			}
			memcpy(iconnection->inputBuffer, iconnection->socketBuffer.buffer, (unsigned int)len + 1);

			// Point to the start of the next one.
			//////////////////////////////////////
			next += 7;

			// Move the rest of the connect buffer up to the front.
			///////////////////////////////////////////////////////
			iconnection->socketBuffer.len -= (next - iconnection->socketBuffer.buffer);
			memmove(iconnection->socketBuffer.buffer, next, (unsigned int)iconnection->socketBuffer.len + 1);

			// Check for an id.
			///////////////////
			str = strstr(iconnection->inputBuffer, "\\id\\");
			if(str != NULL)
			{
				// Get the id.
				//////////////
				id = atoi(str + 4);

				// Try and match the id with an operation.
				//////////////////////////////////////////
				if(!gpiFindOperationByID(connection, &operation, id))
				{
					gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_HotError,
						"No matching operation found for id %d\n", id);
				}
				else
				{
					// Process the operation.
					/////////////////////////
					CHECK_RESULT(gpiProcessOperation(connection, operation, iconnection->inputBuffer));
				}
			}
			// This is an unsolicited message.
			//////////////////////////////////
			else
			{
				// Check for an error.
				//////////////////////
				if(gpiCheckForError(connection, iconnection->inputBuffer, GPITrue))
				{
					return GP_SERVER_ERROR;
				}
				else if(strncmp(iconnection->inputBuffer, "\\bm\\", 4) == 0)
				{
					CHECK_RESULT(gpiProcessRecvBuddyMessage(connection, iconnection->inputBuffer));
				}
				else if(strncmp(iconnection->inputBuffer, "\\ka\\", 4) == 0)
				{
					// Ignore the keep-alive.
					/////////////////////////
				}
				else if(strncmp(iconnection->inputBuffer, "\\lt\\", 4) == 0)
				{
					// Process the login ticket
					/////////////////////////
					gpiValueForKey(iconnection->inputBuffer, "\\lt\\", iconnection->loginTicket, sizeof(iconnection->loginTicket));
				}
				else if(strncmp(iconnection->inputBuffer, "\\bsi\\", 5) == 0)
				{
					CHECK_RESULT(gpiProcessRecvBuddyStatusInfo(connection, iconnection->inputBuffer));
				}                
                else if(strncmp(iconnection->inputBuffer, "\\bdy\\", 5) == 0)
                {
                    // Process the buddy list - retrieved upon login before final login response
                    // * Note: this only gets the list of your buddies so at least you'll know who
                    // is a buddy while the status of each is asynchronously updated.
                    //////////////////////////////////////////////////////////////////////////////
                    CHECK_RESULT(gpiProcessRecvBuddyList(connection, iconnection->inputBuffer));
                }
                else if(strncmp(iconnection->inputBuffer, "\\blk\\", 5) == 0)
                {
                    // Process the block list - retrieved upon login before final login response
                    //////////////////////////////////////////////////////////////////////////////
                    CHECK_RESULT(gpiProcessRecvBlockedList(connection, iconnection->inputBuffer));
                }
				else
				{
					// This is an unrecognized message.
					///////////////////////////////////
					gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Network, GSIDebugLevel_HotError,
						"Received an unrecognized message.\n");
				}
			}
		}

		
		// Check for a closed connection.
		/////////////////////////////////
		if(connClosed && iconnection->connectState != GPI_PROFILE_DELETING)
		{
			// We've been disconnected.
			///////////////////////////
			// Let gpiDisconnect change the state to GPI_DISCONNECTED
			//iconnection->connectState = GPI_DISCONNECTED;
			gpiSetError(connection, GP_CONNECTION_CLOSED, "The server has closed the connection.");
			gpiCallErrorCallback(connection, GP_NETWORK_ERROR, GP_FATAL);
			return GP_NO_ERROR;
		}

		//PANTS|05.23.00 - removed sleep
		//crt - added it back 6/13/00
		//PANTS|07.10.00 - only sleep if looping
		loop = gpiOperationsAreBlocking(connection);
		if(loop)
			msleep(10);
	}
	while(loop);

	// Send Keep-Alive. Just need TCP to ack the data
	/////////////////////////////////////////////////	
	if ( now - iconnection->kaTransmit > KEEPALIVE_TIMEOUT )
	{
		// keep alive packet will be sent next think
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\ka\\\\final\\");
		iconnection->kaTransmit = now;
	}

	return GP_NO_ERROR;
}

GPResult
gpiProcess(
  GPConnection * connection,
  int blockingOperationID
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GPIOperation * operation;
	GPIOperation * delOperation;
	GPResult result = GP_NO_ERROR;
	GPIBool loop;

	assert((iconnection->connectState == GPI_NOT_CONNECTED) ||
	       (iconnection->connectState == GPI_CONNECTING) ||
	       (iconnection->connectState == GPI_NEGOTIATING) ||
	       (iconnection->connectState == GPI_CONNECTED) ||
	       (iconnection->connectState == GPI_DISCONNECTED) ||
		   (iconnection->connectState == GPI_PROFILE_DELETING));

	// Check if no connection was attempted.
	////////////////////////////////////////
/*	if(iconnection->connectState == GPI_NOT_CONNECTED)
		return GP_NO_ERROR;

	// Check for a disconnection.
	/////////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		return GP_NO_ERROR;
*/
	// Check if we're connecting.
	/////////////////////////////
	if(iconnection->connectState == GPI_CONNECTING)
	{
		do
		{
			result = gpiCheckConnect(connection);
			//PANTS|07.10.00 - only sleep if looping
			loop = (((result == GP_NO_ERROR) && (blockingOperationID != 0) && (iconnection->connectState == GPI_CONNECTING))) ? GPITrue:GPIFalse;
			if(loop)
				msleep(10);
		}
		while(loop);

		if(result != GP_NO_ERROR)
		{
			// Find the connect operation.
			//////////////////////////////
			if(gpiFindOperationByID(connection, &operation, 1))
			{
				operation->result = GP_SERVER_ERROR;
			}
			else
			{
				// Couldn't find the connect operation.
				///////////////////////////////////////
				assert(0);
			}
		}
	}

	// Only do this stuff if we're connected.
	/////////////////////////////////////////
	if((iconnection->connectState == GPI_CONNECTED) || (iconnection->connectState == GPI_NEGOTIATING) || 
		(iconnection->connectState == GPI_PROFILE_DELETING))
	{
#ifdef _PS3
        // initialize NP during the sync delay, if initialized wait for status == online
        ////////////////////////////////////////////////////////////////////////////////
        if (iconnection->npInitialized && !iconnection->npStatusRetrieved)
            gpiCheckNpStatus(connection);

        // TODO: handle non-fatal errors (consider all errors from sync non-fatal?)
        if (iconnection->npInitialized && iconnection->npStatusRetrieved)
        {
            // Delay sync after initialization to ensure block list has been received
            /////////////////////////////////////////////////////////////////////////
            if ((current_time() - iconnection->loginTime) > GPI_NP_SYNC_DELAY)
            {
                if (iconnection->npPerformBuddySync)
                    gpiSyncNpBuddies(connection);
                if (iconnection->npPerformBlockSync)
                    gpiSyncNpBlockList(connection);
            }

            // Need to check callback for lookups
            gpiProcessNp(connection);
        }
#endif		
        
        // Process the connection.
		//////////////////////////
		if(result == GP_NO_ERROR)
			result = gpiProcessConnectionManager(connection);

		// Process peer messaging stuff.
		////////////////////////////////
		if(result == GP_NO_ERROR)
			result = gpiProcessPeers(connection);

#ifndef NOFILE
		// Process transfers.
		/////////////////////
		if(result == GP_NO_ERROR)
			result = gpiProcessTransfers(connection);
#endif
	}

	// Process searches.
	////////////////////
	if(result == GP_NO_ERROR)
		result = gpiProcessSearches(connection);

	// Look for failed operations.
	//////////////////////////////
	for(operation = iconnection->operationList ; operation != NULL ; )
	{
		if(operation->result != GP_NO_ERROR)
		{
			gpiFailedOpCallback(connection, operation);
			delOperation = operation;
			operation = operation->pnext;
			gpiRemoveOperation(connection, delOperation);
		}
		else
		{
			operation = operation->pnext;
		}
	}

	// Call callbacks.
	//////////////////
	CHECK_RESULT(gpiProcessCallbacks(connection, blockingOperationID));

	if(iconnection->fatalError)
	{
		gpiDisconnect(connection, GPIFalse);
		gpiReset(connection);
	}
	else
	{
		//assert(!((result != GP_NO_ERROR) && (iconnection->connectState != GPI_CONNECTED)));
	}

	return result;
}

GPResult
gpiEnable(
  GPConnection * connection, 
  GPEnum state
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;

	// Enable the state.
	////////////////////
	switch(state)
	{
	case GP_INFO_CACHING:
		iconnection->infoCaching = GPITrue;
		break;

	case GP_SIMULATION:
		iconnection->simulation = GPITrue;
		break;

	case GP_INFO_CACHING_BUDDY_AND_BLOCK_ONLY:
		iconnection->infoCachingBuddyAndBlockOnly = GPITrue;
		break;

	default:
		Error(connection, GP_PARAMETER_ERROR, "Invalid state.");
	}

	return GP_NO_ERROR;
}

static GPIBool gpiFreeProfileInfo(
  GPConnection * connection,
  GPIProfile * profile,
  void * data
)
{
	GSI_UNUSED(data);

	gpiFreeInfoCache(profile);
	freeclear(profile->peerSig);

	if(gpiCanFreeProfile(profile))
	{
		gpiRemoveProfile(connection, profile);
		return GPIFalse;
	}

	return GPITrue;
}

GPResult
gpiDisable(
  GPConnection * connection, 
  GPEnum state
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;

	if(state == GP_INFO_CACHING)
	{
		iconnection->infoCaching = GPIFalse;

		// freeclear everyone's info.
		////////////////////////
		while(!gpiProfileMap(connection, gpiFreeProfileInfo, NULL))  { };
	}
	else if(state == GP_SIMULATION)
	{
		iconnection->simulation = GPIFalse;
	}
	else if(state == GP_INFO_CACHING_BUDDY_AND_BLOCK_ONLY)
	{
		iconnection->infoCachingBuddyAndBlockOnly = GPIFalse;
	}
	else
	{
		Error(connection, GP_PARAMETER_ERROR, "Invalid state.");
	}

	return GP_NO_ERROR;
}

#ifdef _DEBUG
static int nProfiles;
static int nUserID;
static int nBuddyStatus;
static int nBuddyMemory;
static int nInfoCache;
static int nInfoMemory;
static int nAuthSig;
static int nPeerSig;
static int nTotalMemory;
static int nBlocked;

static GPIBool
gpiReportProfile(
  GPConnection * connection,
  GPIProfile * profile,
  void * data
)
{
	int temp;

	GSI_UNUSED(connection);
	GSI_UNUSED(data);

	nProfiles++;
	nTotalMemory += sizeof(GPIProfile);
	if(profile->userId) nUserID++;
	if(profile->buddyStatus)
	{
		nBuddyStatus++;
		temp = sizeof(GPIBuddyStatus);
		if(profile->buddyStatus->statusString)
			temp += (int)(strlen(profile->buddyStatus->statusString) + 1);
		if(profile->buddyStatus->locationString)
			temp += (int)(strlen(profile->buddyStatus->locationString) + 1);
#ifdef GSI_UNICODE
//		if(profile->buddyStatus->statusString_W)
//			temp += (wcslen(profile->buddyStatus->statusString_W) + 2);
//		if(profile->buddyStatus->locationString_W)
//			temp += (wcslen(profile->buddyStatus->locationString_W) + 2);
#endif
		nBuddyMemory += temp;
		nTotalMemory += temp;
	}
	if(profile->cache)
	{
		nInfoCache++;
		temp = sizeof(GPIInfoCache);
		if(profile->cache->nick)
			temp += (int)(strlen(profile->cache->nick) + 1);
		if(profile->cache->uniquenick)
			temp += (int)(strlen(profile->cache->uniquenick) + 1);
		if(profile->cache->email)
			temp += (int)(strlen(profile->cache->email) + 1);
		if(profile->cache->firstname)
			temp += (int)(strlen(profile->cache->firstname) + 1);
		if(profile->cache->lastname)
			temp += (int)(strlen(profile->cache->lastname) + 1);
		if(profile->cache->homepage)
			temp += (int)(strlen(profile->cache->homepage) + 1);
		nInfoMemory += temp;
		nTotalMemory += temp;
	}
	if(profile->authSig) nAuthSig++;
	if(profile->peerSig) nPeerSig++;
    if(profile->blocked) nBlocked++;

	return GPITrue;
}

void
gpiReport(
  GPConnection * connection,
  void (* report)(const char * output)
)
{
	char buf[128];

	nProfiles = 0;
	nUserID = 0;
	nBuddyStatus = 0;
	nBuddyMemory = 0;
	nInfoCache = 0;
	nInfoMemory = 0;
	nAuthSig = 0;
	nPeerSig = 0;
	nTotalMemory = 0;
    nBlocked = 0;

	report("START PROFILE MAP");
	report("-----------------");
	gpiProfileMap(connection, gpiReportProfile, NULL);

	sprintf(buf, "%d profiles %d bytes (%d avg)", nProfiles, nTotalMemory, nTotalMemory / max(nProfiles, 1));
	report(buf);
	if(nProfiles)
	{
		sprintf(buf, "UserID: %d (%d%%)", nUserID, nUserID * 100 / nProfiles);
		report(buf);
		sprintf(buf, "BuddyStatus: %d (%d%%) %d bytes (%d avg)", nBuddyStatus, nBuddyStatus * 100 / nProfiles, nBuddyMemory, nBuddyMemory / max(nBuddyStatus, 1));
		report(buf);
		sprintf(buf, "InfoCache: %d (%d%%) %d bytes (%d avg)", nInfoCache, nInfoCache * 100 / nProfiles, nInfoMemory, nInfoMemory / max(nInfoCache, 1));
		report(buf);
		sprintf(buf, "AuthSig: %d (%d%%)", nAuthSig, nAuthSig * 100 / nProfiles);
		report(buf);
		sprintf(buf, "PeerSig: %d (%d%%)", nPeerSig, nPeerSig * 100 / nProfiles);
		report(buf);
        sprintf(buf, "Blocked: %d (%d%%)", nBlocked, nBlocked * 100 / nProfiles);
        report(buf);
	}

	report("---------------");
	report("END PROFILE MAP");


}
#endif
