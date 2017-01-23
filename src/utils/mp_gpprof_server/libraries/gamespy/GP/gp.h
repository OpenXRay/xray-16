/*
gp.h
GameSpy Presence SDK 
Dan "Mr. Pants" Schoenblum

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

***********************************************************************
Please see the GameSpy Presence SDK documentation for more information
**********************************************************************/
#ifndef _GP_H_
#define _GP_H_

// necessary for gsi_char and UNICODE support
#include "../common/gsCommon.h"

#ifdef __cplusplus
extern "C" {
#endif

//ENUMS
////////
typedef enum _GPEnum
{
	// Callbacks
	////////////
	GP_ERROR = 0,
	GP_RECV_BUDDY_REQUEST,
	GP_RECV_BUDDY_STATUS,
	GP_RECV_BUDDY_MESSAGE,
	GP_RECV_BUDDY_UTM,
	GP_RECV_GAME_INVITE,
	GP_TRANSFER_CALLBACK,
	GP_RECV_BUDDY_AUTH,
	GP_RECV_BUDDY_REVOKE,
		
	// Global States.
	/////////////////
	GP_INFO_CACHING = 0x0100,
	GP_SIMULATION,
	GP_INFO_CACHING_BUDDY_AND_BLOCK_ONLY,

	// Blocking
	///////////
	GP_BLOCKING = 1,
	GP_NON_BLOCKING = 0,

	// Firewall
	///////////
	GP_FIREWALL = 1,
	GP_NO_FIREWALL = 0,

	// Check Cache
	//////////////
	GP_CHECK_CACHE = 1,
	GP_DONT_CHECK_CACHE = 0,

	// Is Valid Email.
	// PANTS|02.15.00
	//////////////////
	GP_VALID = 1,
	GP_INVALID = 0,

	// Fatal Error.
	///////////////
	GP_FATAL = 1,
	GP_NON_FATAL = 0,

	// Sex
	//////
	GP_MALE = 0x0500,
	GP_FEMALE,
	GP_PAT,

	// Profile Search.
	//////////////////
	GP_MORE = 0x0600,
	GP_DONE,

	// Set Info
	///////////
	GP_NICK = 0x0700,
	GP_UNIQUENICK,
	GP_EMAIL,
	GP_PASSWORD,
	GP_FIRSTNAME,
	GP_LASTNAME,
	GP_ICQUIN,
	GP_HOMEPAGE,
	GP_ZIPCODE,
	GP_COUNTRYCODE,
	GP_BIRTHDAY,
	GP_SEX,
	GP_CPUBRANDID,
	GP_CPUSPEED,
	GP_MEMORY,
	GP_VIDEOCARD1STRING,
	GP_VIDEOCARD1RAM,
	GP_VIDEOCARD2STRING,
	GP_VIDEOCARD2RAM,
	GP_CONNECTIONID,
	GP_CONNECTIONSPEED,
	GP_HASNETWORK,
	GP_OSSTRING,
	GP_AIMNAME,  // PANTS|03.20.01
	GP_PIC,
	GP_OCCUPATIONID,
	GP_INDUSTRYID,
	GP_INCOMEID,
	GP_MARRIEDID,
	GP_CHILDCOUNT,
	GP_INTERESTS1,

	// New Profile.
	///////////////
	GP_REPLACE = 1,
	GP_DONT_REPLACE = 0,

	// Is Connected.
	////////////////
	GP_CONNECTED = 1,
	GP_NOT_CONNECTED = 0,

	// Public mask.
	///////////////
	GP_MASK_NONE        = 0x00000000,
	GP_MASK_HOMEPAGE    = 0x00000001,
	GP_MASK_ZIPCODE     = 0x00000002,
	GP_MASK_COUNTRYCODE = 0x00000004,
	GP_MASK_BIRTHDAY    = 0x00000008,
	GP_MASK_SEX         = 0x00000010,
	GP_MASK_EMAIL       = 0x00000020,
	GP_MASK_ALL         = 0xFFFFFFFF,

	// Status
	/////////
	GP_OFFLINE  = 0,
	GP_ONLINE   = 1,
	GP_PLAYING  = 2,
	GP_STAGING  = 3,
	GP_CHATTING = 4,
	GP_AWAY     = 5,

	// Session flags
	/////////////////
	GP_SESS_IS_CLOSED = 0x00000001,
	GP_SESS_IS_OPEN = 0x00000002,
	GP_SESS_HAS_PASSWORD = 0x00000004,
	GP_SESS_IS_BEHIND_NAT = 0x00000008,
	GP_SESS_IS_RANKED = 0x000000010,
	

	// CPU Brand ID
	///////////////
	GP_INTEL = 1,
	GP_AMD,
	GP_CYRIX,
	GP_MOTOROLA,
	GP_ALPHA,

	// Connection ID.
	/////////////////
	GP_MODEM = 1,
	GP_ISDN,
	GP_CABLEMODEM,
	GP_DSL,
	GP_SATELLITE,
	GP_ETHERNET,
	GP_WIRELESS,

	// Transfer callback type.
	// *** the transfer is ended when these types are received
	//////////////////////////
	GP_TRANSFER_SEND_REQUEST = 0x800,  // arg->num == numFiles
	GP_TRANSFER_ACCEPTED,
	GP_TRANSFER_REJECTED,        // ***
	GP_TRANSFER_NOT_ACCEPTING,   // ***
	GP_TRANSFER_NO_CONNECTION,   // ***
	GP_TRANSFER_DONE,            // ***
	GP_TRANSFER_CANCELLED,       // ***
	GP_TRANSFER_LOST_CONNECTION, // ***
	GP_TRANSFER_ERROR,           // ***
	GP_TRANSFER_THROTTLE,  // arg->num == Bps
	GP_FILE_BEGIN,
	GP_FILE_PROGRESS,  // arg->num == numBytes
	GP_FILE_END,
	GP_FILE_DIRECTORY,
	GP_FILE_SKIP,
	GP_FILE_FAILED,  // arg->num == error

	// GP_FILE_FAILED error
	///////////////////////
	GP_FILE_READ_ERROR = 0x900,
	GP_FILE_WRITE_ERROR,
	GP_FILE_DATA_ERROR,

	// Transfer Side.
	/////////////////
	GP_TRANSFER_SENDER = 0xA00,
	GP_TRANSFER_RECEIVER,

	// UTM send options.
	////////////////////
	GP_DONT_ROUTE = 0xB00, // only send direct

	// Quiet mode flags.
	////////////////////
	GP_SILENCE_NONE       = 0x00000000,
	GP_SILENCE_MESSAGES   = 0x00000001,
	GP_SILENCE_UTMS       = 0x00000002,
	GP_SILENCE_LIST       = 0x00000004, // includes requests, auths, and revokes
	GP_SILENCE_ALL        = 0xFFFFFFFF,

	// Flags for checking if newer version of status info is supported
	GP_NEW_STATUS_INFO_SUPPORTED = 0xC00,
	GP_NEW_STATUS_INFO_NOT_SUPPORTED = 0xC01
} GPEnum;

//RESULTS
//////////
typedef enum _GPResult
{
	GP_NO_ERROR,			
	GP_MEMORY_ERROR,
	GP_PARAMETER_ERROR,
	GP_NETWORK_ERROR,
	GP_SERVER_ERROR,
	GP_MISC_ERROR,
	GP_COUNT
} GPResult;

//ERROR CODES
/////////////
//#define GP_ERROR_TYPE(errorCode)  ((errorCode) >> 8)
typedef enum _GPErrorCode
{
	// General.
	///////////
	GP_GENERAL = 0x0000,
	GP_PARSE,
	GP_NOT_LOGGED_IN,
	GP_BAD_SESSKEY,
	GP_DATABASE,
	GP_NETWORK,
	GP_FORCED_DISCONNECT,
	GP_CONNECTION_CLOSED,
	GP_UDP_LAYER,

	// Login.
	/////////
	GP_LOGIN = 0x0100,
	GP_LOGIN_TIMEOUT,

	GP_LOGIN_BAD_NICK,
	GP_LOGIN_BAD_EMAIL,
	GP_LOGIN_BAD_PASSWORD,
	GP_LOGIN_BAD_PROFILE,
	GP_LOGIN_PROFILE_DELETED,
	GP_LOGIN_CONNECTION_FAILED,
	GP_LOGIN_SERVER_AUTH_FAILED,
	GP_LOGIN_BAD_UNIQUENICK,
	GP_LOGIN_BAD_PREAUTH,

	// Newuser.
	///////////
	GP_NEWUSER = 0x0200,
	GP_NEWUSER_BAD_NICK,
	GP_NEWUSER_BAD_PASSWORD,
	GP_NEWUSER_UNIQUENICK_INVALID,
	GP_NEWUSER_UNIQUENICK_INUSE,

	// Updateui.
	////////////
	GP_UPDATEUI = 0x0300,
	GP_UPDATEUI_BAD_EMAIL,

	// Newprofile.
	//////////////
	GP_NEWPROFILE = 0x0400,
	GP_NEWPROFILE_BAD_NICK,
	GP_NEWPROFILE_BAD_OLD_NICK,

	// Updatepro.
	/////////////
	GP_UPDATEPRO = 0x0500,
	GP_UPDATEPRO_BAD_NICK,

	// Addbuddy.
	////////////
	GP_ADDBUDDY = 0x0600,
	GP_ADDBUDDY_BAD_FROM,
	GP_ADDBUDDY_BAD_NEW,
	GP_ADDBUDDY_ALREADY_BUDDY,

	// Authadd.
	///////////
	GP_AUTHADD = 0x0700,
	GP_AUTHADD_BAD_FROM,
	GP_AUTHADD_BAD_SIG,

	// Status.
	//////////
	GP_STATUS = 0x0800,

	// Bm.
	//////
	GP_BM = 0x0900,
	GP_BM_NOT_BUDDY,
	GP_BM_EXT_INFO_NOT_SUPPORTED,
	GP_BM_BUDDY_OFFLINE,

	// Getprofile.
	//////////////
	GP_GETPROFILE = 0x0A00,
	GP_GETPROFILE_BAD_PROFILE,

	// Delbuddy.
	////////////
	GP_DELBUDDY = 0x0B00,
	GP_DELBUDDY_NOT_BUDDY,

	// Delprofile.
	/////////////
	GP_DELPROFILE = 0x0C00,
	GP_DELPROFILE_LAST_PROFILE,

	// Search.
	//////////
	GP_SEARCH = 0x0D00,
	GP_SEARCH_CONNECTION_FAILED,
	GP_SEARCH_TIMED_OUT,

	// Check.
	/////////
	GP_CHECK = 0x0E00,
	GP_CHECK_BAD_EMAIL,
	GP_CHECK_BAD_NICK,
	GP_CHECK_BAD_PASSWORD,

	// Revoke.
	//////////
	GP_REVOKE = 0x0F00,
	GP_REVOKE_NOT_BUDDY,

	// Registeruniquenick.
	//////////////////////
	GP_REGISTERUNIQUENICK = 0x1000,
	GP_REGISTERUNIQUENICK_TAKEN,
	GP_REGISTERUNIQUENICK_RESERVED,
	GP_REGISTERUNIQUENICK_BAD_NAMESPACE,

	// Register cdkey.
	//////////////////
	GP_REGISTERCDKEY = 0x1100,
	GP_REGISTERCDKEY_BAD_KEY,
	GP_REGISTERCDKEY_ALREADY_SET,
	GP_REGISTERCDKEY_ALREADY_TAKEN,

	// AddBlock.
	////////////
	GP_ADDBLOCK = 0x1200,
	GP_ADDBLOCK_ALREADY_BLOCKED,

	// RemoveBlock.
	///////////////
	GP_REMOVEBLOCK = 0x1300,
	GP_REMOVEBLOCK_NOT_BLOCKED

} GPErrorCode;

//STRING LENGTHS
////////////////
#define GP_NICK_LEN                 31
#define GP_UNIQUENICK_LEN           21
#define GP_FIRSTNAME_LEN            31
#define GP_LASTNAME_LEN             31
#define GP_EMAIL_LEN                51
#define GP_PASSWORD_LEN             31
#define GP_PASSWORDENC_LEN          ((((GP_PASSWORD_LEN+2)*4)/3)+1)
#define GP_HOMEPAGE_LEN             76
#define GP_ZIPCODE_LEN              11
#define GP_COUNTRYCODE_LEN          3
#define GP_PLACE_LEN                128
#define GP_AIMNAME_LEN              51
#define GP_REASON_LEN               1025
#define GP_STATUS_STRING_LEN        256
#define GP_LOCATION_STRING_LEN      256
#define GP_ERROR_STRING_LEN         256
#define GP_AUTHTOKEN_LEN            256
#define GP_PARTNERCHALLENGE_LEN     256
#define GP_CDKEY_LEN                65
#define GP_CDKEYENC_LEN             ((((GP_CDKEY_LEN+2)*4)/3)+1)
#define GP_LOGIN_TICKET_LEN         25

#define GP_RICH_STATUS_LEN          256
#define GP_STATUS_BASIC_STR_LEN     33

// Random number seed for PASSWORDENC and CDKEYENC 
//   MUST MATCH SERVER - If you change this, you'll have to 
//                       release an updated server
#define GP_XOR_SEED                 0x79707367 // "gspy"

// Well known values for partner ID.
#define GP_PARTNERID_GAMESPY		0
#define GP_PARTNERID_IGN			10

// Maximum number of namespaces that can be searched for a uniquenick
#define GP_MAX_NAMESPACEIDS         16

//TYPES
////////
// GPConnection
///////////////
typedef void * GPConnection;

// GPProfile
////////////
typedef int GPProfile;

// GPTransfer
/////////////
typedef int GPTransfer;

// GPCallback
/////////////
typedef void (* GPCallback)(
  GPConnection * connection,
  void * arg,
  void * param
);

//STRUCTURES
/////////////
// GPErrorArg
/////////////
typedef struct
{
  GPResult result;
  GPErrorCode errorCode;
  gsi_char * errorString;
  GPEnum fatal;
} GPErrorArg;

// GPConnectResponseArg
////////////////////////
typedef struct
{
  GPResult result;
  GPProfile profile;
  gsi_char uniquenick[GP_UNIQUENICK_LEN];
} GPConnectResponseArg;

// GPNewUserResponseArg
///////////////////////
typedef struct
{
  GPResult result;
  GPProfile profile;
} GPNewUserResponseArg;

// GPCheckResponseArg
/////////////////////
typedef struct
{
  GPResult result;
  GPProfile profile;
} GPCheckResponseArg;

// GPSuggestUniqueNickResponseArg
/////////////////////////////////
typedef struct
{
  GPResult result;
  int numSuggestedNicks;
  gsi_char ** suggestedNicks;
} GPSuggestUniqueNickResponseArg;

// GPRegisterUniqueNickResponseArg
//////////////////////////////////
typedef struct
{
	GPResult result;
} GPRegisterUniqueNickResponseArg;

// GPRegisterCdKeyResponseArg
//////////////////////////////////
typedef struct
{
	GPResult result;
} GPRegisterCdKeyResponseArg;

// GPNewProfileResponseArg
//////////////////////////
typedef struct
{
  GPResult result;
  GPProfile profile;
} GPNewProfileResponseArg;

// GPDeleteProfileResponseArg
/////////////////////////////

typedef struct  
{
	GPResult result;
	GPProfile profile;
} GPDeleteProfileResponseArg;

// GPProfileSearchMatch
///////////////////////
typedef struct
{
  GPProfile profile;
  gsi_char nick[GP_NICK_LEN];
  gsi_char uniquenick[GP_UNIQUENICK_LEN];
  int namespaceID;
  gsi_char firstname[GP_FIRSTNAME_LEN];
  gsi_char lastname[GP_LASTNAME_LEN];
  gsi_char email[GP_EMAIL_LEN];
} GPProfileSearchMatch;

// GPProfileSearchResponseArg
/////////////////////////////
typedef struct
{
  GPResult result;
  int numMatches;
  GPEnum more;
  GPProfileSearchMatch * matches;
} GPProfileSearchResponseArg;

// GPGetInfoResponseArg
///////////////////////
typedef struct
{
  GPResult result;
  GPProfile profile;
  gsi_char nick[GP_NICK_LEN];
  gsi_char uniquenick[GP_UNIQUENICK_LEN];
  gsi_char email[GP_EMAIL_LEN];
  gsi_char firstname[GP_FIRSTNAME_LEN];
  gsi_char lastname[GP_LASTNAME_LEN];
  gsi_char homepage[GP_HOMEPAGE_LEN];
  int icquin;
  gsi_char zipcode[GP_ZIPCODE_LEN];
  gsi_char countrycode[GP_COUNTRYCODE_LEN];
  float longitude; // negative is west, positive is east.  (0, 0) means unknown.
  float latitude;  // negative is south, positive is north.  (0, 0) means unknown.
  gsi_char place[GP_PLACE_LEN];  // e.g., "USA|California|Irvine", "South Korea|Seoul", "Turkey"
  int birthday;
  int birthmonth;
  int birthyear;
  GPEnum sex;
  GPEnum publicmask;
  gsi_char aimname[GP_AIMNAME_LEN];
  int pic;
  int occupationid;
  int industryid;
  int incomeid;
  int marriedid;
  int childcount;
  int interests1;
  int ownership1;
  int conntypeid;
} GPGetInfoResponseArg;

// GPRecvBuddyRequestArg
////////////////////////
typedef struct
{ 
  GPProfile profile;
  unsigned int date;
  gsi_char reason[GP_REASON_LEN];
} GPRecvBuddyRequestArg;

// GPBuddyStatus
////////////////
typedef struct
{ 
  GPProfile profile; 
  GPEnum status;
  gsi_char statusString[GP_STATUS_STRING_LEN];
  gsi_char locationString[GP_LOCATION_STRING_LEN];
  unsigned int ip; 
  int port;
  GPEnum quietModeFlags;
} GPBuddyStatus;


// BETA
//GPBuddyStatusInfo
///////////////////
typedef struct
{
	GPProfile profile;
	GPEnum statusState;
	unsigned int buddyIp;
	unsigned short buddyPort;
	unsigned int hostIp;
	unsigned int hostPrivateIp;
	unsigned short queryPort;
	unsigned short hostPort;
	unsigned int sessionFlags;
	gsi_char richStatus[GP_RICH_STATUS_LEN];
	gsi_char gameType[GP_STATUS_BASIC_STR_LEN];
	gsi_char gameVariant[GP_STATUS_BASIC_STR_LEN];
	gsi_char gameMapName[GP_STATUS_BASIC_STR_LEN];
	GPEnum quietModeFlags;
	GPEnum newStatusInfoFlag;
} GPBuddyStatusInfo;

//GPGetBuddyStatusInfoKeysArg
/////////////////////////////
typedef struct  
{
	GPProfile profile;
	gsi_char **keys;
	gsi_char **values;
	int numKeys;

} GPGetBuddyStatusInfoKeysArg;


// GPRecvBuddyStatusArg
///////////////////////
typedef struct
{
	GPProfile profile; 
	unsigned int date;
	int index;
} GPRecvBuddyStatusArg;

// GPRecvBuddyMessageArg
////////////////////////
typedef struct
{ 
  GPProfile profile;
  unsigned int date;
  gsi_char * message;
} GPRecvBuddyMessageArg;

typedef struct 
{
  GPProfile profile;
  unsigned int date;
  gsi_char * message;
} GPRecvBuddyUTMArg;

typedef struct  
{
	GPProfile profile;
	unsigned int date;
} GPRecvBuddyAuthArg;

typedef struct 
{
	GPProfile profile;
	unsigned int date;
} GPRecvBuddyRevokeArg;

// GPTransferCallbackArg;
/////////////////////////
typedef struct
{
  GPTransfer transfer;
  GPEnum type;
  int index;
  int num;
  gsi_char * message;
} GPTransferCallbackArg;

// GPIsValidEmailResponseArg
////////////////////////////
typedef struct
{
  GPResult result;
  gsi_char email[GP_EMAIL_LEN];
  GPEnum isValid;
} GPIsValidEmailResponseArg;

// GPGetUserNicksResponseArg
////////////////////////////
typedef struct
{
  GPResult result;
  gsi_char email[GP_EMAIL_LEN];
  int numNicks;  // If 0, then the email/password did not match.
  gsi_char ** nicks;
  gsi_char ** uniquenicks;
} GPGetUserNicksResponseArg;

// GPRecvGameInviteArg
//////////////////////
typedef struct
{
  GPProfile profile;
  int productID;
  gsi_char location[GP_LOCATION_STRING_LEN];
} GPRecvGameInviteArg;

// GPFindPlayerMatch
////////////////////
typedef struct
{
	GPProfile profile;
	gsi_char nick[GP_NICK_LEN];
	GPEnum status;
	gsi_char statusString[GP_STATUS_STRING_LEN];
} GPFindPlayerMatch;

// GPFindPlayersResponseArg
///////////////////////////
typedef struct
{
  GPResult result;
  int productID;  //PANTS|06.06.00 - added by request for JED
  int numMatches;
  GPFindPlayerMatch * matches;
} GPFindPlayersResponseArg;

// GPGetReverseBuddiesResponseArg
/////////////////////////////////
typedef struct
{
	GPResult result;
	int numProfiles;
	GPProfileSearchMatch * profiles;
} GPGetReverseBuddiesResponseArg;

typedef struct  
{
	GPProfile profile;
	gsi_char uniqueNick[GP_UNIQUENICK_LEN];
} GPUniqueMatch;

typedef struct  
{
	GPResult result;
	int numOfUniqueMatchs;
	GPUniqueMatch *matches;
} GPGetReverseBuddiesListResponseArg;


//GLOBALS
/////////
/* The hostnames of the connection manager
server and the search manager server.
If the app resolves either or both hostnames,
the IP(s) can be stored in the string(s) before
calling gpInitialize */
extern char GPConnectionManagerHostname[64];
extern char GPSearchManagerHostname[64];


//FUNCTIONS
////////////
#ifndef GSI_UNICODE
#define gpConnect			gpConnectA
#define gpConnectNewUser	gpConnectNewUserA
#define gpConnectUniqueNick gpConnectUniqueNickA
#define gpConnectPreAuthenticated  gpConnectPreAuthenticatedA
#define gpCheckUser			gpCheckUserA
#define gpNewUser			gpNewUserA
#define gpSuggestUniqueNick gpSuggestUniqueNickA
#define gpRegisterUniqueNick       gpRegisterUniqueNickA
#define gpRegisterCdKey     gpRegisterCdKeyA
#define gpGetErrorString	gpGetErrorStringA
#define gpNewProfile		gpNewProfileA
#define gpProfileSearch		gpProfileSearchA
#define gpProfileSearchUniquenick	gpProfileSearchUniquenickA
#define gpSetInfos			gpSetInfosA
#define gpSendBuddyRequest	gpSendBuddyRequestA
#ifndef GP_NEW_STATUS_INFO
#define gpSetStatus			gpSetStatusA
#endif
#ifdef GP_NEW_STATUS_INFO
// BETA
#define gpSetStatusInfo     gpSetStatusInfoA
#endif
#define gpSendBuddyMessage	gpSendBuddyMessageA
#define gpSendBuddyUTM		gpSendBuddyUTMA
#define gpIsValidEmail		gpIsValidEmailA
#define gpGetUserNicks		gpGetUserNicksA
#define gpSetInfoCacheFilename     gpSetInfoCacheFilenameA
#define gpSendFiles			gpSendFilesA
#define gpAcceptTransfer	gpAcceptTransferA
#define gpRejectTransfer	gpRejectTransferA
#define gpSetTransferDirectory     gpSetTransferDirectoryA
#define gpGetFileName		gpGetFileNameA
#define gpGetFilePath		gpGetFilePathA
#define gpInvitePlayer      gpInvitePlayerA
#ifdef GP_NEW_STATUS_INFO
// BETA
#define gpAddStatusInfoKey       gpAddStatusInfoKeyA
#define gpSetStatusInfoKey       gpSetStatusInfoKeyA
#define gpGetStatusInfoKeyVal    gpGetStatusInfoKeyValA
#define gpDelStatusInfoKey       gpDelStatusInfoKeyA
#endif
#else
#define gpConnect			gpConnectW
#define gpConnectNewUser	gpConnectNewUserW
#define gpConnectUniqueNick gpConnectUniqueNickW
#define gpConnectPreAuthenticated  gpConnectPreAuthenticatedW
#define gpCheckUser			gpCheckUserW
#define gpNewUser			gpNewUserW
#define gpSuggestUniqueNick gpSuggestUniqueNickW
#define gpRegisterUniqueNick       gpRegisterUniqueNickW
#define gpRegisterCdKey     gpRegisterCdKeyW
#define gpGetErrorString	gpGetErrorStringW
#define gpNewProfile		gpNewProfileW
#define gpProfileSearch		gpProfileSearchW
#define gpProfileSearchUniquenick	gpProfileSearchUniquenickW
#define gpSetInfos			gpSetInfosW
#define gpSendBuddyRequest	gpSendBuddyRequestW
#ifndef GP_NEW_STATUS_INFO
#define gpSetStatus			gpSetStatusW
#endif
#ifdef GP_NEW_STATUS_INFO
// BETA
#define gpSetStatusInfo     gpSetStatusInfoW
#endif
#define gpSendBuddyMessage	gpSendBuddyMessageW
#define gpSendBuddyUTM		gpSendBuddyUTMW
#define gpIsValidEmail		gpIsValidEmailW
#define gpGetUserNicks		gpGetUserNicksW
#define gpSetInfoCacheFilename     gpSetInfoCacheFilenameW
#define gpSendFiles			gpSendFilesW
#define gpAcceptTransfer	gpAcceptTransferW
#define gpRejectTransfer	gpRejectTransferW
#define gpSetTransferDirectory     gpSetTransferDirectoryW
#define gpGetFileName		gpGetFileNameW
#define gpGetFilePath		gpGetFilePathW
#define gpInvitePlayer      gpInvitePlayerW
// #ifdef GP_NEW_STATUS_INFO
// BETA
// #define gpAddStatusInfoKey       gpAddStatusInfoKeyW
// #define gpSetStatusInfoKey       gpSetStatusInfoKeyW
// #define gpGetStatusInfoKeyVal    gpGetStatusInfoKeyValW
// #define gpDelStatusInfoKey       gpDelStatusInfoKeyW
// #endif
#endif

// gpInitialize
///////////////
GPResult gpInitialize
(
  GPConnection * connection,
  int productID,				// The productID is a unique ID that identifies your product
  int namespaceID,				// The namespaceID identified which namespace to login under. A namespaceID of 0 indicates that no 
								// namespace should be used. A namespaceID of 1 represents the default GameSpy namespace 
  int partnerID					// The partnerID identifies the account system being used.
								// Use GP_PARTNERID_GAMESPY for GSID accounts.
								// Use GP_PARTNERID_IGN for IGN accounts.
);

// gpDestroy
////////////
void gpDestroy(
  GPConnection * connection
);

// gpEnable
///////////
GPResult gpEnable
(
  GPConnection * connection, 
  GPEnum state
);

// gpDisable
////////////
GPResult gpDisable
(
  GPConnection * connection, 
  GPEnum state
);

// gpProcess
////////////
GPResult gpProcess
(
  GPConnection * connection
);

// gpSetCallback
////////////////
GPResult gpSetCallback
(
  GPConnection * connection,
  GPEnum func,
  GPCallback callback,
  void * param
);

// gpConnect
////////////
GPResult gpConnect
(
  GPConnection * connection,
  const gsi_char nick[GP_NICK_LEN],
  const gsi_char email[GP_EMAIL_LEN],
  const gsi_char password[GP_PASSWORD_LEN],
  GPEnum firewall,
  GPEnum blocking,
  GPCallback callback,
  void * param
);

// gpConnectNewUser
///////////////////
GPResult gpConnectNewUser
(
  GPConnection * connection,
  const gsi_char nick[GP_NICK_LEN],
  const gsi_char uniquenick[GP_UNIQUENICK_LEN],
  const gsi_char email[GP_EMAIL_LEN],
  const gsi_char password[GP_PASSWORD_LEN],
  const gsi_char cdkey[GP_CDKEY_LEN],
  GPEnum firewall,
  GPEnum blocking,
  GPCallback callback,
  void * param
);

// gpConnectUniqueNick
//////////////////////
GPResult gpConnectUniqueNick
(
  GPConnection * connection,
  const gsi_char uniquenick[GP_UNIQUENICK_LEN],
  const gsi_char password[GP_PASSWORD_LEN],
  GPEnum firewall,
  GPEnum blocking,
  GPCallback callback,
  void * param
);

// gpConnectPreAuthenticated
////////////////////////////
GPResult gpConnectPreAuthenticated
(
  GPConnection * connection,
  const gsi_char authtoken[GP_AUTHTOKEN_LEN],
  const gsi_char partnerchallenge[GP_PARTNERCHALLENGE_LEN],
  GPEnum firewall,
  GPEnum blocking,
  GPCallback callback,
  void * param
);

// gpDisconnect
///////////////
void gpDisconnect
(
  GPConnection * connection
);

// gpIsConnected
////////////////
GPResult gpIsConnected
(
  GPConnection * connection,
  GPEnum * connected
);

// gpCheckUser
//////////////
GPResult gpCheckUser
(
  GPConnection * connection,
  const gsi_char nick[GP_NICK_LEN],
  const gsi_char email[GP_EMAIL_LEN],
  const gsi_char password[GP_PASSWORD_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
);

// gpNewUser
////////////
GPResult gpNewUser
(
  GPConnection * connection,
  const gsi_char nick[GP_NICK_LEN],
  const gsi_char uniquenick[GP_UNIQUENICK_LEN],
  const gsi_char email[GP_EMAIL_LEN],
  const gsi_char password[GP_PASSWORD_LEN],
  const gsi_char cdkey[GP_CDKEY_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
);

// gpSuggestUniqueNick
//////////////////////
GPResult gpSuggestUniqueNick
(
  GPConnection * connection,
  const gsi_char desirednick[GP_UNIQUENICK_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
);

// gpRegisterUniqueNick
///////////////////////
GPResult gpRegisterUniqueNick
(
  GPConnection * connection,
  const gsi_char uniquenick[GP_UNIQUENICK_LEN],
  const gsi_char cdkey[GP_CDKEY_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
);

// gpRegisterCdKey
///////////////////////
GPResult gpRegisterCdKey
(
  GPConnection * connection,
  const gsi_char cdkey[GP_CDKEY_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
);

// gpGetErrorCode
/////////////////
GPResult gpGetErrorCode(
  GPConnection * connection,
  GPErrorCode * errorCode
);

// gpGetErrorString
///////////////////
GPResult gpGetErrorString(
  GPConnection * connection,
  gsi_char errorString[GP_ERROR_STRING_LEN]
);

// gpNewProfile
///////////////
GPResult gpNewProfile(
  GPConnection * connection,
  const gsi_char nick[GP_NICK_LEN],
  GPEnum replace,
  GPEnum blocking,
  GPCallback callback,
  void * param
);

// gpDeleteProfile
//////////////////
GPResult gpDeleteProfile(
    GPConnection * connection,
    GPCallback callback,
	void * param
);

// gpProfileFromID
// PANTS|09.11.00 - A GPProfile is now the same
// as a profileid.  This function is no longer needed
// and will be removed in a future version of GP.
/////////////////////////////////////////////////////
GPResult gpProfileFromID(
  GPConnection * connection, 
  GPProfile * profile, 
  int id
);

// gpIDFromProfile
// PANTS|09.11.00 - A GPProfile is now the same
// as a profileid.  This function is no longer needed
// and will be removed in a future version of GP.
/////////////////////////////////////////////////////
GPResult gpIDFromProfile(
  GPConnection * connection,
  GPProfile profile,
  int * id
);

// gpUserIDFromProfile
//////////////////
GPResult gpUserIDFromProfile(
  GPConnection * connection,
  GPProfile profile,
  int * userid
);

// gpProfileSearch
//////////////////
GPResult gpProfileSearch(
  GPConnection * connection,
  const gsi_char nick[GP_NICK_LEN],
  const gsi_char uniquenick[GP_UNIQUENICK_LEN],
  const gsi_char email[GP_EMAIL_LEN],
  const gsi_char firstname[GP_FIRSTNAME_LEN],
  const gsi_char lastname[GP_LASTNAME_LEN],
  int icquin,
  GPEnum blocking,
  GPCallback callback,
  void * param
);

// gpProfileSearchUniquenick
////////////////////////////
GPResult gpProfileSearchUniquenick(
  GPConnection * connection,
  const gsi_char uniquenick[GP_UNIQUENICK_LEN],
  const int namespaceIDs[GP_MAX_NAMESPACEIDS],
  int numNamespaces,
  GPEnum blocking,
  GPCallback callback,
  void * param
);

// gpGetInfo
////////////
GPResult gpGetInfo(
  GPConnection * connection,
  GPProfile profile, 
  GPEnum checkCache,
  GPEnum blocking,
  GPCallback callback,
  void * param
);

// gpGetInfoNoWait
//////////////////
GPResult gpGetInfoNoWait(
  GPConnection * connection,
  GPProfile profile,
  GPGetInfoResponseArg * arg
);

// gpSetInfoi
/////////////
GPResult gpSetInfoi(
  GPConnection * connection, 
  GPEnum info, 
  int value
);

// gpSetInfos
/////////////
GPResult gpSetInfos(
  GPConnection * connection,
  GPEnum info,
  const gsi_char * value
);

// gpSetInfod
/////////////
GPResult gpSetInfod(
  GPConnection * connection,
  GPEnum info,
  int day,
  int month,
  int year
);

// gpSetInfoMask
////////////////
GPResult gpSetInfoMask(
  GPConnection * connection,
  GPEnum mask
);

// gpSendBuddyRequest
/////////////////////
GPResult gpSendBuddyRequest(
  GPConnection * connection,
  GPProfile profile,
  const gsi_char reason[GP_REASON_LEN]
);

// gpAuthBuddyRequest
/////////////////////
GPResult gpAuthBuddyRequest(
  GPConnection * connection,
  GPProfile profile
);

// gpDenyBuddyRequest
// PANTS|09.11.00
/////////////////////
GPResult gpDenyBuddyRequest(
  GPConnection * connection,
  GPProfile profile
);

// gpDeleteBuddy
////////////////
GPResult gpDeleteBuddy(
  GPConnection * connection,
  GPProfile profile
);

// gpAddToBlockedList
/////////////////////
GPResult gpAddToBlockedList(
  GPConnection * connection,
  GPProfile profile
);

// gpRemoveFromBlockedList
/////////////////////
GPResult gpRemoveFromBlockedList(
  GPConnection * connection,
  GPProfile profile
);

// gpGetNumBlocked
//////////////////
GPResult gpGetNumBlocked(
  GPConnection * connection,
  int * numBlocked
);

// gpGetBlockedProfile
/////////////////////
GPResult gpGetBlockedProfile(
  GPConnection * connection, 
  int index,
  GPProfile * profile
);

// gpIsBlocked
// returns gsi_true if blocked, gsi_false if not blocked
////////////////////////////////////////////////////////
gsi_bool gpIsBlocked(
  GPConnection * connection,
  GPProfile profile
);

// gpGetNumBuddies
//////////////////
GPResult gpGetNumBuddies(
  GPConnection * connection,
  int * numBuddies
);

// gpGetBuddyStatus
///////////////////
#ifndef GP_NEW_STATUS_INFO
GPResult gpGetBuddyStatus(
  GPConnection * connection,
  int index,
  GPBuddyStatus * status
);
#endif

#ifdef GP_NEW_STATUS_INFO
//
//////////////////////////////
GPResult gpGetBuddyStatusInfo(
	GPConnection * connection,
	int index, 
	GPBuddyStatusInfo * statusInfo
);

GPResult gpSetBuddyAddr(
	GPConnection *connection, 
	int index,
	unsigned int buddyIp,
	unsigned short buddyPort
);
#endif
// gpGetBuddyIndex
//////////////////
GPResult gpGetBuddyIndex(
  GPConnection * connection, 
  GPProfile profile, 
  int * index
);

// gpIsBuddy
// returns 1 if a buddy, 0 if not a buddy
////////////
int gpIsBuddy(
  GPConnection * connection,
  GPProfile profile
);

int gpIsBuddyConnectionOpen(
  GPConnection * connection,
  GPProfile profile
);

// gpSetStatus
//////////////
#ifndef GP_NEW_STATUS_INFO
GPResult gpSetStatus(
  GPConnection * connection,
  GPEnum status,
  const gsi_char statusString[GP_STATUS_STRING_LEN],
  const gsi_char locationString[GP_LOCATION_STRING_LEN]
);
#endif

#ifdef GP_NEW_STATUS_INFO
GPResult gpSetStatusInfo(
	GPConnection *connection, 
	GPEnum statusState,
	unsigned int hostIp, 
	unsigned int hostPrivateIp,
	unsigned short queryPort,
	unsigned short hostPort,
	unsigned int sessionFlags,
	const gsi_char *richStatus,
	int richStatusLen,
	const gsi_char *gameType,
	int gameTypeLen,
	const gsi_char *gameVariant,
	int gameVariantLen,
	const gsi_char *gameMapName,
	int gameMapNameLen
);

GPResult gpAddStatusInfoKey(GPConnection *connection, const gsi_char *keyName, const gsi_char *keyValue);
GPResult gpSetStatusInfoKey(GPConnection *connection, const gsi_char *keyName, const gsi_char *keyValue);
GPResult gpGetStatusInfoKeyVal(GPConnection *connection, const gsi_char *keyName, gsi_char **keyValue);
GPResult gpDelStatusInfoKey(GPConnection *connection, const gsi_char *keyName);

GPResult gpGetBuddyStatusInfoKeys(GPConnection *connection, int index, GPCallback callback, void *userData);
#endif
// gpSendBuddyMessage
/////////////////////
GPResult gpSendBuddyMessage(
  GPConnection * connection,
  GPProfile profile,
  const gsi_char * message
);

// gpSendBuddyUTM
/////////////////////
GPResult gpSendBuddyUTM(
  GPConnection * connection,
  GPProfile profile,
  const gsi_char * message,
  int sendOption // GP_DONT_ROUTE
);


// PANTS|02.15.00
// Added gpIsValidEmail and gpGetUserNicks for login wizard.
////////////////////////////////////////////////////////////

// gpIsValidEmail
/////////////////
GPResult gpIsValidEmail(
  GPConnection * connection,
  const gsi_char email[GP_EMAIL_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
);

// gpGetUserNicks
/////////////////
GPResult gpGetUserNicks(
  GPConnection * connection,
  const gsi_char email[GP_EMAIL_LEN],
  const gsi_char password[GP_PASSWORD_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
);

// *DEPRECATED*
// gpSetInvitableGames
//////////////////////
GPResult gpSetInvitableGames(
  GPConnection * connection,
  int numProductIDs,
  const int * productIDs
);

// *DEPRECATED*
// gpFindPlayers
////////////////
GPResult gpFindPlayers(
  GPConnection * connection,
  int productID,
  GPEnum blocking,
  GPCallback callback,
  void * param
);

// gpInvitePlayer
/////////////////
GPResult gpInvitePlayer(
  GPConnection * connection,
  GPProfile profile,
  int productID,
  const gsi_char location[GP_LOCATION_STRING_LEN]
);

// gpGetReverseBuddies
// Get profiles that have you on their buddy list.
//////////////////////////////////////////////////
GPResult gpGetReverseBuddies(
	GPConnection * connection,
	GPEnum blocking,
	GPCallback callback,
	void * param
);

// gpGetReverseBuddiesList
// Get profile ids and unique nicks for profiles 
// that have you on their buddy list.
//////////////////////////////////////////////////
GPResult gpGetReversBuddiesList( GPConnection * connection,
	GPProfile *targets, int numOfTargets, 
	GPEnum blocking,
	GPCallback callback,
	void * param
);

// gpRevokeBuddyAuthorization
/////////////////////////////
GPResult gpRevokeBuddyAuthorization(
  GPConnection * connection,
  GPProfile profile
);

// gpSetCdKey
GPResult gpSetCdKey(
  GPConnection * connection,
  const gsi_char cdkeyhash,
  GPCallback callback
);

// gpGetLoginTicket
/////////////////////////////
GPResult gpGetLoginTicket(
  GPConnection * connection,
  char loginTicket[GP_LOGIN_TICKET_LEN]
);

// gpSetQuietMode
/////////////////
GPResult gpSetQuietMode(
  GPConnection * connection,
  GPEnum flags
);

#ifndef NOFILE

// gpiSetInfoCacheFilename
// Should be called before gpIntialize.
///////////////////////////////////////
void gpSetInfoCacheFilename(
  const gsi_char * filename
);

///////////////////
// FILE TRANSFER //
///////////////////
typedef void (* gpSendFilesCallback)(
  GPConnection * connection,
  int index,
  const gsi_char ** path,
  const gsi_char ** name,
  void * param
);

GPResult gpSendFiles(
  GPConnection * connection,
  GPTransfer * transfer,
  GPProfile profile,
  const gsi_char * message,
  gpSendFilesCallback callback,
  void * param
);

GPResult gpAcceptTransfer(
  GPConnection * connection,
  GPTransfer transfer,
  const gsi_char * message
);

GPResult gpRejectTransfer(
  GPConnection * connection,
  GPTransfer transfer,
  const gsi_char * message
);

GPResult gpFreeTransfer(
  GPConnection * connection,
  GPTransfer transfer
);

GPResult gpSetTransferData(
  GPConnection * connection,
  GPTransfer transfer,
  void * userData
);

void * gpGetTransferData(
  GPConnection * connection,
  GPTransfer transfer
);

GPResult gpSetTransferDirectory(
  GPConnection * connection,
  GPTransfer transfer,
  const gsi_char * directory
);

// NOTE: THROTTLING IS NOT CURRENTLY IMPLEMENTED
GPResult gpSetTransferThrottle(
  GPConnection * connection,
  GPTransfer transfer,
  int throttle
);

// NOTE: THROTTLING IS NOT CURRENTLY IMPLEMENTED
GPResult gpGetTransferThrottle(
  GPConnection * connection,
  GPTransfer transfer,
  int * throttle
);

GPResult gpGetTransferProfile(
  GPConnection * connection,
  GPTransfer transfer,
  GPProfile * profile
);

GPResult gpGetTransferSide(
  GPConnection * connection,
  GPTransfer transfer,
  GPEnum * side
);

GPResult gpGetTransferSize(
  GPConnection * connection,
  GPTransfer transfer,
  int * size
);

GPResult gpGetTransferProgress(
  GPConnection * connection,
  GPTransfer transfer,
  int * progress
);

GPResult gpGetNumFiles(
  GPConnection * connection,
  GPTransfer transfer,
  int * num
);

GPResult gpGetCurrentFile(
  GPConnection * connection,
  GPTransfer transfer,
  int * index
);

GPResult gpSkipFile(
  GPConnection * connection,
  GPTransfer transfer,
  int index
);

GPResult gpGetFileName(
  GPConnection * connection,
  GPTransfer transfer,
  int index,
  gsi_char ** name
);

GPResult gpGetFilePath(
  GPConnection * connection,
  GPTransfer transfer,
  int index,
  gsi_char ** path
);

GPResult gpGetFileSize(
  GPConnection * connection,
  GPTransfer transfer,
  int index,
  int * size
);

GPResult gpGetFileProgress(
  GPConnection * connection,
  GPTransfer transfer,
  int index,
  int * progress
);

GPResult gpGetFileModificationTime(
  GPConnection * connection,
  GPTransfer transfer,
  int index,
  gsi_time * modTime
);

GPResult gpGetNumTransfers(
  GPConnection * connection,
  int * num
);

GPResult gpGetTransfer(
  GPConnection * connection,
  int index,
  GPTransfer * transfer
);
#endif

#ifdef _DEBUG
// gpProfilesReport
// PANTS|09.11.00
///////////////////
void gpProfilesReport(
  GPConnection * connection,
  void (* report)(const char * output)
);
#endif

#ifdef __cplusplus
}
#endif

#endif
