/*
gpi.h
GameSpy Presence SDK 
Dan "Mr. Pants" Schoenblum

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

***********************************************************************
Please see the GameSpy Presence SDK documentation for more information
**********************************************************************/

#ifndef _GPI_H_
#define _GPI_H_

//INCLUDES
//////////
#include "../common/gsCommon.h"
#include "../common/gsAvailable.h"
#include "../common/gsUdpEngine.h"
#include "../hashtable.h"
#include "../darray.h"
#include "../md5.h"
#include "gp.h"

// Extended message support
#define GPI_NEW_AUTH_NOTIFICATION	(1<<0)
#define GPI_NEW_REVOKE_NOTIFICATION (1<<1)

// New Status Info support
#define GPI_NEW_STATUS_NOTIFICATION (1<<2)

// Buddy List + Block List retrieval on login
#define GPI_NEW_LIST_RETRIEVAL_ON_LOGIN (1<<3)

// Extended SDK features
#ifndef GPI_SDKREV
#ifdef GP_NEW_STATUS_INFO
#define GPI_SDKREV (GPI_NEW_AUTH_NOTIFICATION | GPI_NEW_REVOKE_NOTIFICATION | GPI_NEW_STATUS_NOTIFICATION | GPI_NEW_LIST_RETRIEVAL_ON_LOGIN)
#else
#define GPI_SDKREV (GPI_NEW_AUTH_NOTIFICATION | GPI_NEW_REVOKE_NOTIFICATION | GPI_NEW_LIST_RETRIEVAL_ON_LOGIN)
#endif
#endif

// New UDP Layer port
#define GPI_PEER_PORT 6500

//TYPES
///////
// Boolean.
///////////
typedef enum _GPIBool
{
	GPIFalse,
	GPITrue
} GPIBool;

#include "gpiUtility.h"
#include "gpiCallback.h"
#include "gpiOperation.h"
#include "gpiConnect.h"
#include "gpiBuffer.h"
#include "gpiInfo.h"
#include "gpiProfile.h"
#include "gpiPeer.h"
#include "gpiSearch.h"
#include "gpiBuddy.h"
#include "gpiTransfer.h"
#include "gpiUnique.h"
#include "gpiKeys.h"

// For PS3 NP Sync functionality
#ifdef _PS3
#include "gpiPS3.h"
#endif

// Connection data.
///////////////////
typedef struct
{
  char errorString[GP_ERROR_STRING_LEN];
  GPIBool infoCaching;
  GPIBool infoCachingBuddyAndBlockOnly;
  GPIBool simulation;
  GPIBool firewall;
  char nick[GP_NICK_LEN];
  char uniquenick[GP_UNIQUENICK_LEN];
  char email[GP_EMAIL_LEN];
  char password[GP_PASSWORD_LEN];
  int sessKey;
  int userid;
  int profileid;
  int partnerID;
  GPICallback callbacks[GPI_NUM_CALLBACKS];
  SOCKET cmSocket;
  int connectState;
  GPIBuffer socketBuffer;
  char * inputBuffer;
  int inputBufferSize;
  GPIBuffer outputBuffer;
  // Replaced by UDP Layer
  //SOCKET peerSocket;
  char mHeader[GS_UDP_MSG_HEADER_LEN];
  unsigned short peerPort;
  int nextOperationID;
  int numSearches;
  
	// new style status info 
	GPEnum lastStatusState;
	unsigned int hostIp;
	unsigned int hostPrivateIp;
	unsigned short queryPort;
	unsigned short hostPort;
	unsigned int sessionFlags;

	char richStatus[GP_RICH_STATUS_LEN];
	char gameType[GP_STATUS_BASIC_STR_LEN];
	char gameVariant[GP_STATUS_BASIC_STR_LEN];
	char gameMapName[GP_STATUS_BASIC_STR_LEN];

	// New Status Info extended info Keys
	DArray extendedInfoKeys;

  // Deprecated
  char lastStatusString[GP_STATUS_STRING_LEN];
  char lastLocationString[GP_LOCATION_STRING_LEN];
  
  GPErrorCode errorCode;
  GPIBool fatalError;
  FILE * diskCache;
  GPIOperation * operationList;
  GPIProfileList profileList;
  GPIPeer * peerList;
  GPICallbackData * callbackList;
  GPICallbackData * lastCallback;
  GPIBuffer updateproBuffer;
  GPIBuffer updateuiBuffer;
  DArray transfers;
  unsigned int nextTransferID;
  int productID;
  int namespaceID;
  char loginTicket[GP_LOGIN_TICKET_LEN];
  GPEnum quietModeFlags;
  gsi_time kaTransmit;
  
#ifdef GSI_UNICODE
  unsigned short errorString_W[GP_ERROR_STRING_LEN];
  unsigned short nick_W[GP_NICK_LEN];
  unsigned short uniquenick_W[GP_UNIQUENICK_LEN];
  unsigned short email_W[GP_EMAIL_LEN];
  unsigned short password_W[GP_PASSWORD_LEN];
  
  // Deprecated
  unsigned short lastStatusString_W[GP_STATUS_STRING_LEN];
  unsigned short lastLocationString_W[GP_LOCATION_STRING_LEN];

  unsigned short richStatus_W[GP_RICH_STATUS_LEN];
  unsigned short gameType_W[GP_STATUS_BASIC_STR_LEN];
  unsigned short gameVariant_W[GP_STATUS_BASIC_STR_LEN];
  unsigned short gameMapName_W[GP_STATUS_BASIC_STR_LEN];
#endif

#ifdef _PS3
  // NP sync info
  gsi_bool  npInitialized;
  gsi_bool  npStatusRetrieved;
  gsi_bool  npBasicGameInitialized;
  gsi_bool  npLookupGameInitialized;
  gsi_bool  npPerformBuddySync;
  gsi_bool  npPerformBlockSync;
  gsi_bool  npSyncLock;
  int       npLookupTitleCtxId;
  DArray    npTransactionList;
  gsi_time  loginTime;
#endif

} GPIConnection;

//FUNCTIONS
///////////
GPResult
gpiInitialize(
  GPConnection * connection,
  int productID,
  int namespaceID,
  int partnerID
);

void
gpiDestroy(
  GPConnection * connection
);

GPResult
gpiReset(
  GPConnection * connection
);

GPResult
gpiProcessConnectionManager(
  GPConnection * connection
);

GPResult
gpiProcess(
  GPConnection * connection,
  int blockingOperationID
);

GPResult
gpiEnable(
  GPConnection * connection, 
  GPEnum state
);

GPResult
gpiDisable(
  GPConnection * connection, 
  GPEnum state
);

#ifdef _DEBUG
void
gpiReport(
  GPConnection * connection,
  void (* report)(const char * output)
);
#endif

#endif
