/*==========================================================================
 *
 *	Copyright (C) 1998-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *	File:		DPlay8.h
 *	Content:	DirectPlay8 include file
 *
 ***************************************************************************/

#ifndef __DIRECTPLAY8_H__
#define __DIRECTPLAY8_H__

#include <ole2.h>	   // for DECLARE_INTERFACE_ and HRESULT

#include "dpaddr.h"


#ifdef __cplusplus
extern "C" {
#endif



/****************************************************************************
 *
 * DirectPlay8 CLSIDs
 *
 ****************************************************************************/

// {743F1DC6-5ABA-429f-8BDF-C54D03253DC2}
DEFINE_GUID(CLSID_DirectPlay8Client,
0x743f1dc6, 0x5aba, 0x429f, 0x8b, 0xdf, 0xc5, 0x4d, 0x3, 0x25, 0x3d, 0xc2);

// {DA825E1B-6830-43d7-835D-0B5AD82956A2}
DEFINE_GUID(CLSID_DirectPlay8Server,
0xda825e1b, 0x6830, 0x43d7, 0x83, 0x5d, 0xb, 0x5a, 0xd8, 0x29, 0x56, 0xa2);

// {286F484D-375E-4458-A272-B138E2F80A6A}
DEFINE_GUID(CLSID_DirectPlay8Peer,
0x286f484d, 0x375e, 0x4458, 0xa2, 0x72, 0xb1, 0x38, 0xe2, 0xf8, 0xa, 0x6a);


// CLSIDs added for DirectX 9

// {FC47060E-6153-4b34-B975-8E4121EB7F3C}
DEFINE_GUID(CLSID_DirectPlay8ThreadPool, 
0xfc47060e, 0x6153, 0x4b34, 0xb9, 0x75, 0x8e, 0x41, 0x21, 0xeb, 0x7f, 0x3c);

// {E4C1D9A2-CBF7-48bd-9A69-34A55E0D8941}
DEFINE_GUID(CLSID_DirectPlay8NATResolver, 
0xe4c1d9a2, 0xcbf7, 0x48bd, 0x9a, 0x69, 0x34, 0xa5, 0x5e, 0xd, 0x89, 0x41);

/****************************************************************************
 *
 * DirectPlay8 Interface IIDs
 *
 ****************************************************************************/

typedef REFIID	DP8REFIID;


// {5102DACD-241B-11d3-AEA7-006097B01411}
DEFINE_GUID(IID_IDirectPlay8Client,
0x5102dacd, 0x241b, 0x11d3, 0xae, 0xa7, 0x0, 0x60, 0x97, 0xb0, 0x14, 0x11);

// {5102DACE-241B-11d3-AEA7-006097B01411}
DEFINE_GUID(IID_IDirectPlay8Server,
0x5102dace, 0x241b, 0x11d3, 0xae, 0xa7, 0x0, 0x60, 0x97, 0xb0, 0x14, 0x11);

// {5102DACF-241B-11d3-AEA7-006097B01411}
DEFINE_GUID(IID_IDirectPlay8Peer,
0x5102dacf, 0x241b, 0x11d3, 0xae, 0xa7, 0x0, 0x60, 0x97, 0xb0, 0x14, 0x11);


// IIDs added for DirectX 9

// {0D22EE73-4A46-4a0d-89B2-045B4D666425}
DEFINE_GUID(IID_IDirectPlay8ThreadPool, 
0xd22ee73, 0x4a46, 0x4a0d, 0x89, 0xb2, 0x4, 0x5b, 0x4d, 0x66, 0x64, 0x25);

// {A9E213F2-9A60-486f-BF3B-53408B6D1CBB}
DEFINE_GUID(IID_IDirectPlay8NATResolver, 
0xa9e213f2, 0x9a60, 0x486f, 0xbf, 0x3b, 0x53, 0x40, 0x8b, 0x6d, 0x1c, 0xbb);

/****************************************************************************
 *
 * DirectPlay8 Service Provider GUIDs
 *
 ****************************************************************************/


// {53934290-628D-11D2-AE0F-006097B01411}
DEFINE_GUID(CLSID_DP8SP_IPX,
0x53934290, 0x628d, 0x11d2, 0xae, 0xf, 0x0, 0x60, 0x97, 0xb0, 0x14, 0x11);


// {6D4A3650-628D-11D2-AE0F-006097B01411}
DEFINE_GUID(CLSID_DP8SP_MODEM,
0x6d4a3650, 0x628d, 0x11d2, 0xae, 0xf, 0x0, 0x60, 0x97, 0xb0, 0x14, 0x11);


// {743B5D60-628D-11D2-AE0F-006097B01411}
DEFINE_GUID(CLSID_DP8SP_SERIAL,
0x743b5d60, 0x628d, 0x11d2, 0xae, 0xf, 0x0, 0x60, 0x97, 0xb0, 0x14, 0x11);


// {EBFE7BA0-628D-11D2-AE0F-006097B01411}
DEFINE_GUID(CLSID_DP8SP_TCPIP,
0xebfe7ba0, 0x628d, 0x11d2, 0xae, 0xf, 0x0, 0x60, 0x97, 0xb0, 0x14, 0x11);


// Service providers added for DirectX 9


// {995513AF-3027-4b9a-956E-C772B3F78006}
DEFINE_GUID(CLSID_DP8SP_BLUETOOTH, 
0x995513af, 0x3027, 0x4b9a, 0x95, 0x6e, 0xc7, 0x72, 0xb3, 0xf7, 0x80, 0x6);


/****************************************************************************
 *
 * DirectPlay8 Interface Pointer definitions
 *
 ****************************************************************************/

typedef	struct IDirectPlay8Client			*PDIRECTPLAY8CLIENT;

typedef	struct IDirectPlay8Server			*PDIRECTPLAY8SERVER;

typedef	struct IDirectPlay8Peer				*PDIRECTPLAY8PEER;


// Interface pointers added for DirectX 9

typedef	struct IDirectPlay8ThreadPool		*PDIRECTPLAY8THREADPOOL;

typedef	struct IDirectPlay8NATResolver		*PDIRECTPLAY8NATRESOLVER;

/****************************************************************************
 *
 * DirectPlay8 Forward Declarations For External Types
 *
 ****************************************************************************/

typedef struct IDirectPlay8LobbiedApplication	*PDNLOBBIEDAPPLICATION;
typedef struct IDirectPlay8Address				IDirectPlay8Address;

/****************************************************************************
 *
 * DirectPlay8 Callback Functions
 *
 ****************************************************************************/

//
// Callback Function Type Definition
//
typedef HRESULT (WINAPI *PFNDPNMESSAGEHANDLER)(PVOID,DWORD,PVOID);

/****************************************************************************
 *
 * DirectPlay8 Datatypes (Non-Structure / Non-Message)
 *
 ****************************************************************************/

//
// Player IDs.  Used to uniquely identify a player in a session
//
typedef DWORD	DPNID,		*PDPNID;

//
// Used as identifiers for operations
//
typedef	DWORD	DPNHANDLE,	*PDPNHANDLE;




/****************************************************************************
 *
 * DirectPlay8 Message Identifiers
 *
 ****************************************************************************/

#define DPN_MSGID_OFFSET					0xFFFF0000
#define DPN_MSGID_ADD_PLAYER_TO_GROUP		( DPN_MSGID_OFFSET | 0x0001 )
#define DPN_MSGID_APPLICATION_DESC			( DPN_MSGID_OFFSET | 0x0002 )
#define DPN_MSGID_ASYNC_OP_COMPLETE			( DPN_MSGID_OFFSET | 0x0003 )
#define DPN_MSGID_CLIENT_INFO				( DPN_MSGID_OFFSET | 0x0004 )
#define DPN_MSGID_CONNECT_COMPLETE			( DPN_MSGID_OFFSET | 0x0005 )
#define DPN_MSGID_CREATE_GROUP				( DPN_MSGID_OFFSET | 0x0006 )
#define DPN_MSGID_CREATE_PLAYER				( DPN_MSGID_OFFSET | 0x0007 )
#define DPN_MSGID_DESTROY_GROUP				( DPN_MSGID_OFFSET | 0x0008 )
#define DPN_MSGID_DESTROY_PLAYER			( DPN_MSGID_OFFSET | 0x0009 )
#define DPN_MSGID_ENUM_HOSTS_QUERY			( DPN_MSGID_OFFSET | 0x000a )
#define DPN_MSGID_ENUM_HOSTS_RESPONSE		( DPN_MSGID_OFFSET | 0x000b )
#define DPN_MSGID_GROUP_INFO				( DPN_MSGID_OFFSET | 0x000c )
#define DPN_MSGID_HOST_MIGRATE				( DPN_MSGID_OFFSET | 0x000d )
#define DPN_MSGID_INDICATE_CONNECT			( DPN_MSGID_OFFSET | 0x000e )
#define DPN_MSGID_INDICATED_CONNECT_ABORTED	( DPN_MSGID_OFFSET | 0x000f )
#define DPN_MSGID_PEER_INFO					( DPN_MSGID_OFFSET | 0x0010 )
#define DPN_MSGID_RECEIVE					( DPN_MSGID_OFFSET | 0x0011 )
#define DPN_MSGID_REMOVE_PLAYER_FROM_GROUP	( DPN_MSGID_OFFSET | 0x0012 )
#define DPN_MSGID_RETURN_BUFFER				( DPN_MSGID_OFFSET | 0x0013 )
#define DPN_MSGID_SEND_COMPLETE				( DPN_MSGID_OFFSET | 0x0014 )
#define DPN_MSGID_SERVER_INFO				( DPN_MSGID_OFFSET | 0x0015 )
#define DPN_MSGID_TERMINATE_SESSION			( DPN_MSGID_OFFSET | 0x0016 )

// Messages added for DirectX 9
#define DPN_MSGID_CREATE_THREAD				( DPN_MSGID_OFFSET | 0x0017 )
#define DPN_MSGID_DESTROY_THREAD			( DPN_MSGID_OFFSET | 0x0018 )
#define DPN_MSGID_NAT_RESOLVER_QUERY		( DPN_MSGID_OFFSET | 0x0101 )

/****************************************************************************
 *
 * DirectPlay8 Constants
 *
 ****************************************************************************/

#define DPNID_ALL_PLAYERS_GROUP				0

//
// DESTROY_GROUP reasons
//
#define DPNDESTROYGROUPREASON_NORMAL				0x0001
#define DPNDESTROYGROUPREASON_AUTODESTRUCTED		0x0002
#define DPNDESTROYGROUPREASON_SESSIONTERMINATED		0x0003

//
// DESTROY_PLAYER reasons
//
#define DPNDESTROYPLAYERREASON_NORMAL				0x0001
#define DPNDESTROYPLAYERREASON_CONNECTIONLOST		0x0002
#define DPNDESTROYPLAYERREASON_SESSIONTERMINATED	0x0003
#define DPNDESTROYPLAYERREASON_HOSTDESTROYEDPLAYER	0x0004

#define DPN_MAX_APPDESC_RESERVEDDATA_SIZE			64



/****************************************************************************
 *
 * DirectPlay8 Flags
 *
 ****************************************************************************/

//
// Asynchronous operation flags (for Async Ops)
//
#define DPNOP_SYNC								0x80000000

//
// Add player to group flags (for AddPlayerToGroup)
//
#define DPNADDPLAYERTOGROUP_SYNC				DPNOP_SYNC

//
// Cancel flags
//
#define DPNCANCEL_CONNECT						0x00000001
#define DPNCANCEL_ENUM							0x00000002
#define DPNCANCEL_SEND							0x00000004
#define DPNCANCEL_ALL_OPERATIONS				0x00008000
// Flags added for DirectX 9
#define DPNCANCEL_PLAYER_SENDS					0x80000000
#define DPNCANCEL_PLAYER_SENDS_PRIORITY_HIGH	(DPNCANCEL_PLAYER_SENDS | 0x00010000)
#define DPNCANCEL_PLAYER_SENDS_PRIORITY_NORMAL	(DPNCANCEL_PLAYER_SENDS | 0x00020000)
#define DPNCANCEL_PLAYER_SENDS_PRIORITY_LOW		(DPNCANCEL_PLAYER_SENDS | 0x00040000)

//
// Close flags (for Close, added for DirectX 9)
//
#define DPNCLOSE_IMMEDIATE						0x00000001

//
// Connect flags (for Connect)
//
#define DPNCONNECT_SYNC							DPNOP_SYNC
#define DPNCONNECT_OKTOQUERYFORADDRESSING		0x0001

//
// Create group flags (for CreateGroup)
//
#define DPNCREATEGROUP_SYNC						DPNOP_SYNC

//
// Destroy group flags (for DestroyGroup)
//
#define DPNDESTROYGROUP_SYNC					DPNOP_SYNC

//
// Enumerate clients and groups flags (for EnumPlayersAndGroups)
//
#define DPNENUM_PLAYERS							0x0001
#define DPNENUM_GROUPS							0x0010

//
// Enum hosts flags (for EnumHosts)
//
#define DPNENUMHOSTS_SYNC						DPNOP_SYNC
#define DPNENUMHOSTS_OKTOQUERYFORADDRESSING		0x0001
#define DPNENUMHOSTS_NOBROADCASTFALLBACK		0x0002

//
// Enum service provider flags (for EnumSP)
//
#define DPNENUMSERVICEPROVIDERS_ALL				0x0001

//
// GetLocalHostAddresses flags (added for DirectX 9)
//
#define DPNGETLOCALHOSTADDRESSES_COMBINED		0x0001

//
// Get send queue info flags (for GetSendQueueInfo)
//
#define DPNGETSENDQUEUEINFO_PRIORITY_NORMAL		0x0001
#define DPNGETSENDQUEUEINFO_PRIORITY_HIGH		0x0002
#define DPNGETSENDQUEUEINFO_PRIORITY_LOW		0x0004

//
// Group information flags (for Group Info)
//
#define DPNGROUP_AUTODESTRUCT					0x0001

//
// Host flags (for Host)
//
#define DPNHOST_OKTOQUERYFORADDRESSING			0x0001

//
// Set info
//
#define DPNINFO_NAME							0x0001
#define DPNINFO_DATA							0x0002

//
// Initialize flags (for Initialize)
//
#define DPNINITIALIZE_DISABLEPARAMVAL			0x0001
// Flags added for DirectX 9
#define DPNINITIALIZE_HINT_LANSESSION			0x0002
#define DPNINITIALIZE_DISABLELINKTUNING			0x0004


//
// Register Lobby flags
//
#define DPNLOBBY_REGISTER						0x0001
#define DPNLOBBY_UNREGISTER						0x0002

//
// Player information flags (for Player Info / Player Messages)
//
#define DPNPLAYER_LOCAL							0x0002
#define DPNPLAYER_HOST							0x0004

//
// Receive indication flags (added for DirectX 9)
//
#define DPNRECEIVE_GUARANTEED					0x0001
#define DPNRECEIVE_COALESCED					0x0002

//
// Remove player from group flags (for RemovePlayerFromGroup)
//
#define DPNREMOVEPLAYERFROMGROUP_SYNC			DPNOP_SYNC

//
// Send flags (for Send/SendTo)
//
#define DPNSEND_SYNC							DPNOP_SYNC
#define DPNSEND_NOCOPY							0x0001
#define DPNSEND_NOCOMPLETE						0x0002
#define DPNSEND_COMPLETEONPROCESS				0x0004
#define DPNSEND_GUARANTEED						0x0008
#define DPNSEND_NONSEQUENTIAL					0x0010
#define DPNSEND_NOLOOPBACK						0x0020
#define DPNSEND_PRIORITY_LOW					0x0040
#define DPNSEND_PRIORITY_HIGH					0x0080
// Flag added for DirectX 9
#define DPNSEND_COALESCE						0x0100

//
// Send complete indication flags (added for DirectX 9)
//
#define DPNSENDCOMPLETE_GUARANTEED				0x0001
#define DPNSENDCOMPLETE_COALESCED				0x0002

//
// Session Flags (for DPN_APPLICATION_DESC)
//
#define DPNSESSION_CLIENT_SERVER				0x0001
#define DPNSESSION_MIGRATE_HOST					0x0004
#define DPNSESSION_NODPNSVR						0x0040
#define DPNSESSION_REQUIREPASSWORD				0x0080
// Flag added for DirectX 9
#define DPNSESSION_NOENUMS						0x0100
#define DPNSESSION_FAST_SIGNED					0x0200
#define DPNSESSION_FULL_SIGNED					0x0400

//
// Set client info flags (for SetClientInfo)
//
#define DPNSETCLIENTINFO_SYNC					DPNOP_SYNC

//
// Set group info flags (for SetGroupInfo)
//
#define DPNSETGROUPINFO_SYNC					DPNOP_SYNC

//
// Set peer info flags (for SetPeerInfo)
//
#define DPNSETPEERINFO_SYNC						DPNOP_SYNC

//
// Set server info flags (for SetServerInfo)
//
#define DPNSETSERVERINFO_SYNC					DPNOP_SYNC

//
// SP capabilities flags
//
#define DPNSPCAPS_SUPPORTSDPNSVR				0x0001
#define DPNSPCAPS_SUPPORTSDPNSRV				DPNSPCAPS_SUPPORTSDPNSVR
#define DPNSPCAPS_SUPPORTSBROADCAST				0x0002
#define DPNSPCAPS_SUPPORTSALLADAPTERS			0x0004
// Flags added for DirectX 9
#define DPNSPCAPS_SUPPORTSTHREADPOOL			0x0008
#define DPNSPCAPS_NETWORKSIMULATOR				0x0010

//
// SP information flags (added for DirectX 9)
//
#define DPNSPINFO_NETWORKSIMULATORDEVICE		0x0001

/****************************************************************************
 *
 * DirectPlay8 Structures (Non-Message)
 *
 ****************************************************************************/

//
// Application description
//
typedef struct	_DPN_APPLICATION_DESC
{
	DWORD	dwSize;							// Size of this structure
	DWORD	dwFlags;						// Flags (DPNSESSION_...)
	GUID	guidInstance;					// Instance GUID
	GUID	guidApplication;				// Application GUID
	DWORD	dwMaxPlayers;					// Maximum # of players allowed (0=no limit)
	DWORD	dwCurrentPlayers;				// Current # of players allowed
	WCHAR	*pwszSessionName;				// Name of the session
	WCHAR	*pwszPassword;					// Password for the session
	PVOID	pvReservedData;					
	DWORD	dwReservedDataSize;
	PVOID	pvApplicationReservedData;
	DWORD	dwApplicationReservedDataSize;
} DPN_APPLICATION_DESC, *PDPN_APPLICATION_DESC;

//
// Generic Buffer Description
//
typedef struct	_BUFFERDESC
{
	DWORD	dwBufferSize;		
	BYTE * 	pBufferData;		
} BUFFERDESC, DPN_BUFFER_DESC, *PDPN_BUFFER_DESC;

typedef BUFFERDESC	FAR * PBUFFERDESC;

//
// DirectPlay8 capabilities
//
typedef struct	_DPN_CAPS
{
	DWORD	dwSize;						// Size of this structure
	DWORD	dwFlags;						// Flags
	DWORD	dwConnectTimeout;			// ms before a connect request times out
	DWORD	dwConnectRetries;				// # of times to attempt the connection
	DWORD	dwTimeoutUntilKeepAlive;		// ms of inactivity before a keep alive is sent
} DPN_CAPS, *PDPN_CAPS;

//
// Extended capabilities structures (added for DirectX 9)
//
typedef struct	_DPN_CAPS_EX
{
	DWORD	dwSize;						// Size of this structure
	DWORD	dwFlags;						// Flags
	DWORD	dwConnectTimeout;			// ms before a connect request times out
	DWORD	dwConnectRetries;				// # of times to attempt the connection
	DWORD	dwTimeoutUntilKeepAlive;		// ms of inactivity before a keep alive is sent
	DWORD	dwMaxRecvMsgSize;			// maximum size in bytes of message that can be received
	DWORD	dwNumSendRetries;			// maximum number of send retries before link is considered dead
	DWORD	dwMaxSendRetryInterval;		// maximum period in msec between send retries
	DWORD	dwDropThresholdRate;			// percentage of dropped packets before throttling
	DWORD	dwThrottleRate;				// percentage amount to reduce send window when throttling
	DWORD	dwNumHardDisconnectSends;	// number of hard disconnect frames to send when close immediate flag is specified
	DWORD	dwMaxHardDisconnectPeriod;	// maximum period between hard disconnect sends
} DPN_CAPS_EX, *PDPN_CAPS_EX;

//
// Connection Statistics information
//
typedef struct _DPN_CONNECTION_INFO
{
	DWORD	dwSize;
	DWORD	dwRoundTripLatencyMS;
	DWORD	dwThroughputBPS;
	DWORD	dwPeakThroughputBPS;

	DWORD	dwBytesSentGuaranteed;
	DWORD	dwPacketsSentGuaranteed;
	DWORD	dwBytesSentNonGuaranteed;
	DWORD	dwPacketsSentNonGuaranteed;

	DWORD	dwBytesRetried;		// Guaranteed only
	DWORD	dwPacketsRetried;	// Guaranteed only
	DWORD	dwBytesDropped;		// Non Guaranteed only
	DWORD	dwPacketsDropped;	// Non Guaranteed only

	DWORD	dwMessagesTransmittedHighPriority;
	DWORD	dwMessagesTimedOutHighPriority;
	DWORD	dwMessagesTransmittedNormalPriority;
	DWORD	dwMessagesTimedOutNormalPriority;
	DWORD	dwMessagesTransmittedLowPriority;
	DWORD	dwMessagesTimedOutLowPriority;

	DWORD	dwBytesReceivedGuaranteed;
	DWORD	dwPacketsReceivedGuaranteed;
	DWORD	dwBytesReceivedNonGuaranteed;
	DWORD	dwPacketsReceivedNonGuaranteed;
	DWORD	dwMessagesReceived;

} DPN_CONNECTION_INFO, *PDPN_CONNECTION_INFO;


//
// Group information structure
//
typedef struct	_DPN_GROUP_INFO
{
	DWORD	dwSize;				// size of this structure
	DWORD	dwInfoFlags;		// information contained
	PWSTR	pwszName;			// Unicode Name
	PVOID	pvData;				// data block
	DWORD	dwDataSize;			// size in BYTES of data block
	DWORD	dwGroupFlags;		// group flags (DPNGROUP_...)
} DPN_GROUP_INFO, *PDPN_GROUP_INFO;

//
// Player information structure
//
typedef struct	_DPN_PLAYER_INFO
{
	DWORD	dwSize;				// size of this structure
	DWORD	dwInfoFlags;		// information contained
	PWSTR	pwszName;			// Unicode Name
	PVOID	pvData;				// data block
	DWORD	dwDataSize;			// size in BYTES of data block
	DWORD	dwPlayerFlags;		// player flags (DPNPLAYER_...)
} DPN_PLAYER_INFO, *PDPN_PLAYER_INFO;

typedef struct _DPN_SECURITY_CREDENTIALS	DPN_SECURITY_CREDENTIALS, *PDPN_SECURITY_CREDENTIALS;
typedef struct _DPN_SECURITY_DESC			DPN_SECURITY_DESC, *PDPN_SECURITY_DESC;

//
// Service provider & adapter enumeration structure
//
typedef struct _DPN_SERVICE_PROVIDER_INFO
{
	DWORD		dwFlags;
	GUID		guid;		// SP Guid
	WCHAR		*pwszName;	// Friendly Name
	PVOID		pvReserved;	
	DWORD		dwReserved;
} DPN_SERVICE_PROVIDER_INFO, *PDPN_SERVICE_PROVIDER_INFO;

//
// Service provider caps structure
//
typedef struct _DPN_SP_CAPS
{
	DWORD	dwSize;							// Size of this structure
	DWORD	dwFlags;						// Flags (DPNSPCAPS_...)
	DWORD	dwNumThreads;					// # of worker threads to use
	DWORD	dwDefaultEnumCount;				// default # of enum requests
	DWORD	dwDefaultEnumRetryInterval;		// default ms between enum requests
	DWORD	dwDefaultEnumTimeout;			// default enum timeout
	DWORD	dwMaxEnumPayloadSize;			// maximum size in bytes for enum payload data
	DWORD	dwBuffersPerThread;				// number of receive buffers per thread
	DWORD	dwSystemBufferSize;				// amount of buffering to do in addition to posted receive buffers
} DPN_SP_CAPS, *PDPN_SP_CAPS;


/****************************************************************************
 *
 * IDirectPlay8 message handler call back structures
 *
 ****************************************************************************/

//
// Add player to group structure for message handler
// (DPN_MSGID_ADD_PLAYER_TO_GROUP)
//
typedef struct	_DPNMSG_ADD_PLAYER_TO_GROUP
{
	DWORD	dwSize;				// Size of this structure
	DPNID	dpnidGroup;			// DPNID of group
	PVOID	pvGroupContext;		// Group context value
	DPNID	dpnidPlayer;		// DPNID of added player
	PVOID	pvPlayerContext;	// Player context value
} DPNMSG_ADD_PLAYER_TO_GROUP, *PDPNMSG_ADD_PLAYER_TO_GROUP;

//
// Async operation completion structure for message handler
// (DPN_MSGID_ASYNC_OP_COMPLETE)
//
typedef struct	_DPNMSG_ASYNC_OP_COMPLETE
{
	DWORD		dwSize;			// Size of this structure
	DPNHANDLE	hAsyncOp;		// DirectPlay8 async operation handle
	PVOID		pvUserContext;	// User context supplied
	HRESULT		hResultCode;	// HRESULT of operation
} DPNMSG_ASYNC_OP_COMPLETE, *PDPNMSG_ASYNC_OP_COMPLETE;

//
// Client info structure for message handler
// (DPN_MSGID_CLIENT_INFO)
//
typedef struct	_DPNMSG_CLIENT_INFO
{
	DWORD	dwSize;				// Size of this structure
	DPNID	dpnidClient;		// DPNID of client
	PVOID	pvPlayerContext;	// Player context value
} DPNMSG_CLIENT_INFO, *PDPNMSG_CLIENT_INFO;

//
// Connect complete structure for message handler
// (DPN_MSGID_CONNECT_COMPLETE)
//
typedef struct	_DPNMSG_CONNECT_COMPLETE
{
	DWORD		dwSize;						// Size of this structure
	DPNHANDLE	hAsyncOp;					// DirectPlay8 Async operation handle
	PVOID		pvUserContext;				// User context supplied at Connect
	HRESULT		hResultCode;				// HRESULT of connection attempt
	PVOID		pvApplicationReplyData;		// Connection reply data from Host/Server
	DWORD		dwApplicationReplyDataSize;	// Size (in bytes) of pvApplicationReplyData

	// Fields added for DirectX 9
	DPNID		dpnidLocal;					// DPNID of local player
} DPNMSG_CONNECT_COMPLETE, *PDPNMSG_CONNECT_COMPLETE;

//
// Create group structure for message handler
// (DPN_MSGID_CREATE_GROUP)
//
typedef struct	_DPNMSG_CREATE_GROUP
{
	DWORD	dwSize;				// Size of this structure
	DPNID	dpnidGroup;			// DPNID of new group
	DPNID	dpnidOwner;			// Owner of newgroup
	PVOID	pvGroupContext;		// Group context value

	// Fields added for DirectX 9
	PVOID	pvOwnerContext;		// Owner context value
} DPNMSG_CREATE_GROUP, *PDPNMSG_CREATE_GROUP;

//
// Create player structure for message handler
// (DPN_MSGID_CREATE_PLAYER)
//
typedef struct	_DPNMSG_CREATE_PLAYER
{
	DWORD	dwSize;				// Size of this structure
	DPNID	dpnidPlayer;		// DPNID of new player
	PVOID	pvPlayerContext;	// Player context value
} DPNMSG_CREATE_PLAYER, *PDPNMSG_CREATE_PLAYER;

//
// Destroy group structure for message handler
// (DPN_MSGID_DESTROY_GROUP)
//
typedef struct	_DPNMSG_DESTROY_GROUP
{
	DWORD	dwSize;				// Size of this structure
	DPNID	dpnidGroup;			// DPNID of destroyed group
	PVOID	pvGroupContext;		// Group context value
	DWORD	dwReason;			// Information only
} DPNMSG_DESTROY_GROUP, *PDPNMSG_DESTROY_GROUP;

//
// Destroy player structure for message handler
// (DPN_MSGID_DESTROY_PLAYER)
//
typedef struct	_DPNMSG_DESTROY_PLAYER
{
	DWORD	dwSize;				// Size of this structure
	DPNID	dpnidPlayer;		// DPNID of leaving player
	PVOID	pvPlayerContext;	// Player context value
	DWORD	dwReason;			// Information only
} DPNMSG_DESTROY_PLAYER, *PDPNMSG_DESTROY_PLAYER;

//
// Enumeration request received structure for message handler
// (DPN_MSGID_ENUM_HOSTS_QUERY)
//
typedef	struct	_DPNMSG_ENUM_HOSTS_QUERY
{
	DWORD				dwSize;				 // Size of this structure.
	IDirectPlay8Address *pAddressSender;		// Address of client who sent the request
	IDirectPlay8Address	*pAddressDevice;		// Address of device request was received on
	PVOID				pvReceivedData;		 // Request data (set on client)
	DWORD				dwReceivedDataSize;	 // Request data size (set on client)
	DWORD				dwMaxResponseDataSize;	// Max allowable size of enum response
	PVOID				pvResponseData;			// Optional query repsonse (user set)
	DWORD				dwResponseDataSize;		// Optional query response size (user set)
	PVOID				pvResponseContext;		// Optional query response context (user set)
} DPNMSG_ENUM_HOSTS_QUERY, *PDPNMSG_ENUM_HOSTS_QUERY;

//
// Enumeration response received structure for message handler
// (DPN_MSGID_ENUM_HOSTS_RESPONSE)
//
typedef	struct	_DPNMSG_ENUM_HOSTS_RESPONSE
{
	DWORD						dwSize;					 // Size of this structure
	IDirectPlay8Address			*pAddressSender;			// Address of host who responded
	IDirectPlay8Address			*pAddressDevice;			// Device response was received on
	const DPN_APPLICATION_DESC	*pApplicationDescription;	// Application description for the session
	PVOID						pvResponseData;			 // Optional response data (set on host)
	DWORD						dwResponseDataSize;		 // Optional response data size (set on host)
	PVOID						pvUserContext;				// Context value supplied for enumeration
	DWORD						dwRoundTripLatencyMS;		// Round trip latency in MS
} DPNMSG_ENUM_HOSTS_RESPONSE, *PDPNMSG_ENUM_HOSTS_RESPONSE;

//
// Group info structure for message handler
// (DPN_MSGID_GROUP_INFO)
//
typedef struct	_DPNMSG_GROUP_INFO
{
	DWORD	dwSize;					// Size of this structure
	DPNID	dpnidGroup;				// DPNID of group
	PVOID	pvGroupContext;			// Group context value
} DPNMSG_GROUP_INFO, *PDPNMSG_GROUP_INFO;

//
// Migrate host structure for message handler
// (DPN_MSGID_HOST_MIGRATE)
//
typedef struct	_DPNMSG_HOST_MIGRATE
{
	DWORD	dwSize;					// Size of this structure
	DPNID	dpnidNewHost;			// DPNID of new Host player
	PVOID	pvPlayerContext;		// Player context value
} DPNMSG_HOST_MIGRATE, *PDPNMSG_HOST_MIGRATE;

//
// Indicate connect structure for message handler
// (DPN_MSGID_INDICATE_CONNECT)
//
typedef struct	_DPNMSG_INDICATE_CONNECT
{
	DWORD				dwSize;					// Size of this structure
	PVOID				pvUserConnectData;		// Connecting player data
	DWORD				dwUserConnectDataSize;	// Size (in bytes) of pvUserConnectData
	PVOID				pvReplyData;			// Connection reply data
	DWORD				dwReplyDataSize;		// Size (in bytes) of pvReplyData
	PVOID				pvReplyContext;			// Buffer context for pvReplyData
	PVOID				pvPlayerContext;		// Player context preset
	IDirectPlay8Address	*pAddressPlayer;		// Address of connecting player
	IDirectPlay8Address	*pAddressDevice;		// Address of device receiving connect attempt
} DPNMSG_INDICATE_CONNECT, *PDPNMSG_INDICATE_CONNECT;

//
// Indicated connect aborted structure for message handler
// (DPN_MSGID_INDICATED_CONNECT_ABORTED)
//
typedef struct	_DPNMSG_INDICATED_CONNECT_ABORTED
{
	DWORD		dwSize;				// Size of this structure
	PVOID		pvPlayerContext;	// Player context preset from DPNMSG_INDICATE_CONNECT
} DPNMSG_INDICATED_CONNECT_ABORTED, *PDPNMSG_INDICATED_CONNECT_ABORTED;

//
// Peer info structure for message handler
// (DPN_MSGID_PEER_INFO)
//
typedef struct	_DPNMSG_PEER_INFO
{
	DWORD	dwSize;					// Size of this structure
	DPNID	dpnidPeer;				// DPNID of peer
	PVOID	pvPlayerContext;		// Player context value
} DPNMSG_PEER_INFO, *PDPNMSG_PEER_INFO;

//
// Receive structure for message handler
// (DPN_MSGID_RECEIVE)
//
typedef struct	_DPNMSG_RECEIVE
{
	DWORD		dwSize;				// Size of this structure
	DPNID		dpnidSender;		// DPNID of sending player
	PVOID		pvPlayerContext;	// Player context value of sending player
	PBYTE		pReceiveData;		// Received data
	DWORD		dwReceiveDataSize;	// Size (in bytes) of pReceiveData
	DPNHANDLE	hBufferHandle;		// Buffer handle for pReceiveData

	// Fields added for DirectX 9
	DWORD		dwReceiveFlags;		// Flags describing how message was received
} DPNMSG_RECEIVE, *PDPNMSG_RECEIVE;

//
// Remove player from group structure for message handler
// (DPN_MSGID_REMOVE_PLAYER_FROM_GROUP)
//
typedef struct	_DPNMSG_REMOVE_PLAYER_FROM_GROUP
{
	DWORD	dwSize;					// Size of this structure
	DPNID	dpnidGroup;				// DPNID of group
	PVOID	pvGroupContext;			// Group context value
	DPNID	dpnidPlayer;			// DPNID of deleted player
	PVOID	pvPlayerContext;		// Player context value
} DPNMSG_REMOVE_PLAYER_FROM_GROUP, *PDPNMSG_REMOVE_PLAYER_FROM_GROUP;

//
// Returned buffer structure for message handler
// (DPN_MSGID_RETURN_BUFFER)
//
typedef struct	_DPNMSG_RETURN_BUFFER
{
	DWORD		dwSize;				// Size of this structure
	HRESULT		hResultCode;		// Return value of operation
	PVOID		pvBuffer;			// Buffer being returned
	PVOID		pvUserContext;		// Context associated with buffer
} DPNMSG_RETURN_BUFFER, *PDPNMSG_RETURN_BUFFER;

//
// Send complete structure for message handler
// (DPN_MSGID_SEND_COMPLETE)
//
typedef struct	_DPNMSG_SEND_COMPLETE
{
	DWORD				dwSize;					// Size of this structure
	DPNHANDLE			hAsyncOp;				// DirectPlay8 Async operation handle
	PVOID				pvUserContext;			// User context supplied at Send/SendTo
	HRESULT				hResultCode;			// HRESULT of send
	DWORD				dwSendTime;				// Send time in ms

	// Fields added for DirectX 9
	DWORD				dwFirstFrameRTT;		// RTT of the first frame in the message
	DWORD				dwFirstFrameRetryCount;	// Retry count of the first frame
	DWORD				dwSendCompleteFlags;	// Flags describing how message was sent
	DPN_BUFFER_DESC		*pBuffers;				// Pointer to array of buffers sent, if DirectPlay did not make a copy
	DWORD				dwNumBuffers;			// Number of buffers in previous array
} DPNMSG_SEND_COMPLETE, *PDPNMSG_SEND_COMPLETE;

//
// Server info structure for message handler
// (DPN_MSGID_SERVER_INFO)
//
typedef struct	_DPNMSG_SERVER_INFO
{
	DWORD	dwSize;					// Size of this structure
	DPNID	dpnidServer;			// DPNID of server
	PVOID	pvPlayerContext;		// Player context value
} DPNMSG_SERVER_INFO, *PDPNMSG_SERVER_INFO;

//
// Terminated session structure for message handler
// (DPN_MSGID_TERMINATE_SESSION)
//
typedef struct	_DPNMSG_TERMINATE_SESSION
{
	DWORD		dwSize;				// Size of this structure
	HRESULT		hResultCode;		// Reason
	PVOID		pvTerminateData;	// Data passed from Host/Server
	DWORD		dwTerminateDataSize;// Size (in bytes) of pvTerminateData
} DPNMSG_TERMINATE_SESSION, *PDPNMSG_TERMINATE_SESSION;


//
// Message structures added for DirectX 9
//

//
// Create thread info structure for message handler
// (DPN_MSGID_CREATE_THREAD)
//
typedef struct	_DPNMSG_CREATE_THREAD
{
	DWORD	dwSize;				// Size of this structure
	DWORD	dwFlags;			// Flags describing this thread
	DWORD	dwProcessorNum;		// Index of processor to which thread is bound
	PVOID	pvUserContext;		// Thread context value
} DPNMSG_CREATE_THREAD, *PDPNMSG_CREATE_THREAD;

//
// Destroy thread info structure for message handler
// (DPN_MSGID_DESTROY_THREAD)
//
typedef struct	_DPNMSG_DESTROY_THREAD
{
	DWORD	dwSize;				// Size of this structure
	DWORD	dwProcessorNum;		// Index of processor to which thread was bound
	PVOID	pvUserContext;		// Thread context value
} DPNMSG_DESTROY_THREAD, *PDPNMSG_DESTROY_THREAD;


//
// Query-to-resolve-NAT-address structure for message handler
// (DPN_MSGID_NAT_RESOLVER_QUERY)
//
typedef	struct	_DPNMSG_NAT_RESOLVER_QUERY
{
	DWORD				dwSize;				// Size of this structure.
	IDirectPlay8Address	*pAddressSender;	// Address of client that sent the query
	IDirectPlay8Address	*pAddressDevice;	// Address of device on which query was received
	WCHAR				*pwszUserString;	// User specified string, or NULL if none
} DPNMSG_NAT_RESOLVER_QUERY, *PDPNMSG_NAT_RESOLVER_QUERY;

/****************************************************************************
 *
 * DirectPlay8 Functions
 *
 ****************************************************************************/



/*
 * This function is no longer supported.  It is recommended that CoCreateInstance be used to create 
 * DirectPlay8 objects.
 *
 * extern HRESULT WINAPI DirectPlay8Create( const CLSID * pcIID, void **ppvInterface, IUnknown *pUnknown );
 * 
 */


/****************************************************************************
 *
 * DirectPlay8 Application Interfaces
 *
 ****************************************************************************/

//
// COM definition for DirectPlay8 Client interface
//
#undef INTERFACE				// External COM Implementation
#define INTERFACE IDirectPlay8Client
DECLARE_INTERFACE_(IDirectPlay8Client,IUnknown)
{
	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)			(THIS_ DP8REFIID riid, LPVOID *ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)			(THIS) PURE;
	STDMETHOD_(ULONG,Release)			(THIS) PURE;
	/*** IDirectPlay8Client methods ***/
	STDMETHOD(Initialize)				(THIS_ PVOID const pvUserContext, const PFNDPNMESSAGEHANDLER pfn, const DWORD dwFlags) PURE;
	STDMETHOD(EnumServiceProviders)		(THIS_ const GUID *const pguidServiceProvider, const GUID *const pguidApplication, DPN_SERVICE_PROVIDER_INFO *const pSPInfoBuffer, PDWORD const pcbEnumData, PDWORD const pcReturned, const DWORD dwFlags) PURE;
	STDMETHOD(EnumHosts)				(THIS_ PDPN_APPLICATION_DESC const pApplicationDesc,IDirectPlay8Address *const pAddrHost,IDirectPlay8Address *const pDeviceInfo,PVOID const pUserEnumData,const DWORD dwUserEnumDataSize,const DWORD dwEnumCount,const DWORD dwRetryInterval,const DWORD dwTimeOut,PVOID const pvUserContext,DPNHANDLE *const pAsyncHandle,const DWORD dwFlags) PURE;
	STDMETHOD(CancelAsyncOperation)		(THIS_ const DPNHANDLE hAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(Connect)					(THIS_ const DPN_APPLICATION_DESC *const pdnAppDesc,IDirectPlay8Address *const pHostAddr,IDirectPlay8Address *const pDeviceInfo,const DPN_SECURITY_DESC *const pdnSecurity,const DPN_SECURITY_CREDENTIALS *const pdnCredentials,const void *const pvUserConnectData,const DWORD dwUserConnectDataSize,void *const pvAsyncContext,DPNHANDLE *const phAsyncHandle,const DWORD dwFlags) PURE;
	STDMETHOD(Send)						(THIS_ const DPN_BUFFER_DESC *const prgBufferDesc,const DWORD cBufferDesc,const DWORD dwTimeOut,void *const pvAsyncContext,DPNHANDLE *const phAsyncHandle,const DWORD dwFlags) PURE;
	STDMETHOD(GetSendQueueInfo)			(THIS_ DWORD *const pdwNumMsgs, DWORD *const pdwNumBytes, const DWORD dwFlags) PURE;
	STDMETHOD(GetApplicationDesc)		(THIS_ DPN_APPLICATION_DESC *const pAppDescBuffer, DWORD *const pcbDataSize, const DWORD dwFlags) PURE;
	STDMETHOD(SetClientInfo)			(THIS_ const DPN_PLAYER_INFO *const pdpnPlayerInfo,PVOID const pvAsyncContext,DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(GetServerInfo)			(THIS_ DPN_PLAYER_INFO *const pdpnPlayerInfo,DWORD *const pdwSize,const DWORD dwFlags) PURE;
	STDMETHOD(GetServerAddress)			(THIS_ IDirectPlay8Address **const pAddress,const DWORD dwFlags) PURE;
	STDMETHOD(Close)					(THIS_ const DWORD dwFlags) PURE;
	STDMETHOD(ReturnBuffer)				(THIS_ const DPNHANDLE hBufferHandle,const DWORD dwFlags) PURE;
	STDMETHOD(GetCaps)					(THIS_ DPN_CAPS *const pdpCaps,const DWORD dwFlags) PURE;
	STDMETHOD(SetCaps)					(THIS_ const DPN_CAPS *const pdpCaps, const DWORD dwFlags) PURE;
	STDMETHOD(SetSPCaps)				(THIS_ const GUID * const pguidSP, const DPN_SP_CAPS *const pdpspCaps, const DWORD dwFlags ) PURE;
	STDMETHOD(GetSPCaps)				(THIS_ const GUID * const pguidSP,DPN_SP_CAPS *const pdpspCaps,const DWORD dwFlags) PURE;
	STDMETHOD(GetConnectionInfo)		(THIS_ DPN_CONNECTION_INFO *const pdpConnectionInfo,const DWORD dwFlags) PURE;
	STDMETHOD(RegisterLobby)			(THIS_ const DPNHANDLE dpnHandle, struct IDirectPlay8LobbiedApplication *const pIDP8LobbiedApplication,const DWORD dwFlags) PURE;
};

//
// COM definition for DirectPlay8 Server interface
//
#undef INTERFACE				// External COM Implementation
#define INTERFACE IDirectPlay8Server
DECLARE_INTERFACE_(IDirectPlay8Server,IUnknown)
{
	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)			(THIS_ DP8REFIID riid, LPVOID *ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)			(THIS) PURE;
	STDMETHOD_(ULONG,Release)			(THIS) PURE;
	/*** IDirectPlay8Server methods ***/
	STDMETHOD(Initialize)				(THIS_ PVOID const pvUserContext, const PFNDPNMESSAGEHANDLER pfn, const DWORD dwFlags) PURE;
	STDMETHOD(EnumServiceProviders)		(THIS_ const GUID *const pguidServiceProvider,const GUID *const pguidApplication,DPN_SERVICE_PROVIDER_INFO *const pSPInfoBuffer,PDWORD const pcbEnumData,PDWORD const pcReturned,const DWORD dwFlags) PURE;
	STDMETHOD(CancelAsyncOperation)		(THIS_ const DPNHANDLE hAsyncHandle,const DWORD dwFlags) PURE;
	STDMETHOD(GetSendQueueInfo)			(THIS_ const DPNID dpnid,DWORD *const pdwNumMsgs, DWORD *const pdwNumBytes, const DWORD dwFlags) PURE;
	STDMETHOD(GetApplicationDesc)		(THIS_ DPN_APPLICATION_DESC *const pAppDescBuffer, DWORD *const pcbDataSize, const DWORD dwFlags) PURE;
	STDMETHOD(SetServerInfo)			(THIS_ const DPN_PLAYER_INFO *const pdpnPlayerInfo,PVOID const pvAsyncContext, DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(GetClientInfo)			(THIS_ const DPNID dpnid,DPN_PLAYER_INFO *const pdpnPlayerInfo,DWORD *const pdwSize,const DWORD dwFlags) PURE;
	STDMETHOD(GetClientAddress)			(THIS_ const DPNID dpnid,IDirectPlay8Address **const pAddress,const DWORD dwFlags) PURE;
	STDMETHOD(GetLocalHostAddresses)	(THIS_ IDirectPlay8Address **const prgpAddress,DWORD *const pcAddress,const DWORD dwFlags) PURE;
	STDMETHOD(SetApplicationDesc)		(THIS_ const DPN_APPLICATION_DESC *const pad, const DWORD dwFlags) PURE;
	STDMETHOD(Host)						(THIS_ const DPN_APPLICATION_DESC *const pdnAppDesc,IDirectPlay8Address **const prgpDeviceInfo,const DWORD cDeviceInfo,const DPN_SECURITY_DESC *const pdnSecurity,const DPN_SECURITY_CREDENTIALS *const pdnCredentials,void *const pvPlayerContext,const DWORD dwFlags) PURE;
	STDMETHOD(SendTo)					(THIS_ const DPNID dpnid,const DPN_BUFFER_DESC *const prgBufferDesc,const DWORD cBufferDesc,const DWORD dwTimeOut,void *const pvAsyncContext,DPNHANDLE *const phAsyncHandle,const DWORD dwFlags) PURE;
	STDMETHOD(CreateGroup)				(THIS_ const DPN_GROUP_INFO *const pdpnGroupInfo,void *const pvGroupContext,void *const pvAsyncContext,DPNHANDLE *const phAsyncHandle,const DWORD dwFlags) PURE;
	STDMETHOD(DestroyGroup)				(THIS_ const DPNID idGroup, PVOID const pvAsyncContext, DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(AddPlayerToGroup)			(THIS_ const DPNID idGroup, const DPNID idClient, PVOID const pvAsyncContext, DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(RemovePlayerFromGroup)	(THIS_ const DPNID idGroup, const DPNID idClient, PVOID const pvAsyncContext, DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(SetGroupInfo)				(THIS_ const DPNID dpnid,DPN_GROUP_INFO *const pdpnGroupInfo,PVOID const pvAsyncContext,DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(GetGroupInfo)				(THIS_ const DPNID dpnid,DPN_GROUP_INFO *const pdpnGroupInfo,DWORD *const pdwSize,const DWORD dwFlags) PURE;
	STDMETHOD(EnumPlayersAndGroups)		(THIS_ DPNID *const prgdpnid, DWORD *const pcdpnid, const DWORD dwFlags) PURE;
	STDMETHOD(EnumGroupMembers)			(THIS_ const DPNID dpnid, DPNID *const prgdpnid, DWORD *const pcdpnid, const DWORD dwFlags) PURE;
	STDMETHOD(Close)					(THIS_ const DWORD dwFlags) PURE;
	STDMETHOD(DestroyClient)			(THIS_ const DPNID dpnidClient, const void *const pvDestroyData, const DWORD dwDestroyDataSize, const DWORD dwFlags) PURE;
	STDMETHOD(ReturnBuffer)				(THIS_ const DPNHANDLE hBufferHandle,const DWORD dwFlags) PURE;
	STDMETHOD(GetPlayerContext)			(THIS_ const DPNID dpnid,PVOID *const ppvPlayerContext,const DWORD dwFlags) PURE;
	STDMETHOD(GetGroupContext)			(THIS_ const DPNID dpnid,PVOID *const ppvGroupContext,const DWORD dwFlags) PURE;
	STDMETHOD(GetCaps)					(THIS_ DPN_CAPS *const pdpCaps,const DWORD dwFlags) PURE;
	STDMETHOD(SetCaps)					(THIS_ const DPN_CAPS *const pdpCaps, const DWORD dwFlags) PURE;
	STDMETHOD(SetSPCaps)				(THIS_ const GUID * const pguidSP, const DPN_SP_CAPS *const pdpspCaps, const DWORD dwFlags ) PURE;
	STDMETHOD(GetSPCaps)				(THIS_ const GUID * const pguidSP, DPN_SP_CAPS *const pdpspCaps,const DWORD dwFlags) PURE;
	STDMETHOD(GetConnectionInfo)		(THIS_ const DPNID dpnid, DPN_CONNECTION_INFO *const pdpConnectionInfo,const DWORD dwFlags) PURE;
	STDMETHOD(RegisterLobby)			(THIS_ const DPNHANDLE dpnHandle, struct IDirectPlay8LobbiedApplication *const pIDP8LobbiedApplication,const DWORD dwFlags) PURE;
};

//
// COM definition for DirectPlay8 Peer interface
//
#undef INTERFACE				// External COM Implementation
#define INTERFACE IDirectPlay8Peer
DECLARE_INTERFACE_(IDirectPlay8Peer,IUnknown)
{
	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)			(THIS_ DP8REFIID riid, LPVOID *ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)			(THIS) PURE;
	STDMETHOD_(ULONG,Release)			(THIS) PURE;
	/*** IDirectPlay8Peer methods ***/
	STDMETHOD(Initialize)				(THIS_ PVOID const pvUserContext, const PFNDPNMESSAGEHANDLER pfn, const DWORD dwFlags) PURE;
	STDMETHOD(EnumServiceProviders)		(THIS_ const GUID *const pguidServiceProvider, const GUID *const pguidApplication, DPN_SERVICE_PROVIDER_INFO *const pSPInfoBuffer, DWORD *const pcbEnumData, DWORD *const pcReturned, const DWORD dwFlags) PURE;
	STDMETHOD(CancelAsyncOperation)		(THIS_ const DPNHANDLE hAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(Connect)					(THIS_ const DPN_APPLICATION_DESC *const pdnAppDesc,IDirectPlay8Address *const pHostAddr,IDirectPlay8Address *const pDeviceInfo,const DPN_SECURITY_DESC *const pdnSecurity,const DPN_SECURITY_CREDENTIALS *const pdnCredentials,const void *const pvUserConnectData,const DWORD dwUserConnectDataSize,void *const pvPlayerContext,void *const pvAsyncContext,DPNHANDLE *const phAsyncHandle,const DWORD dwFlags) PURE;
	STDMETHOD(SendTo)					(THIS_ const DPNID dpnid,const DPN_BUFFER_DESC *const prgBufferDesc,const DWORD cBufferDesc,const DWORD dwTimeOut,void *const pvAsyncContext,DPNHANDLE *const phAsyncHandle,const DWORD dwFlags) PURE;
	STDMETHOD(GetSendQueueInfo)			(THIS_ const DPNID dpnid, DWORD *const pdwNumMsgs, DWORD *const pdwNumBytes, const DWORD dwFlags) PURE;
	STDMETHOD(Host)						(THIS_ const DPN_APPLICATION_DESC *const pdnAppDesc,IDirectPlay8Address **const prgpDeviceInfo,const DWORD cDeviceInfo,const DPN_SECURITY_DESC *const pdnSecurity,const DPN_SECURITY_CREDENTIALS *const pdnCredentials,void *const pvPlayerContext,const DWORD dwFlags) PURE;
	STDMETHOD(GetApplicationDesc)		(THIS_ DPN_APPLICATION_DESC *const pAppDescBuffer, DWORD *const pcbDataSize, const DWORD dwFlags) PURE;
	STDMETHOD(SetApplicationDesc)		(THIS_ const DPN_APPLICATION_DESC *const pad, const DWORD dwFlags) PURE;
	STDMETHOD(CreateGroup)				(THIS_ const DPN_GROUP_INFO *const pdpnGroupInfo,void *const pvGroupContext,void *const pvAsyncContext,DPNHANDLE *const phAsyncHandle,const DWORD dwFlags) PURE;
	STDMETHOD(DestroyGroup)				(THIS_ const DPNID idGroup, PVOID const pvAsyncContext, DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(AddPlayerToGroup)			(THIS_ const DPNID idGroup, const DPNID idClient, PVOID const pvAsyncContext, DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(RemovePlayerFromGroup)	(THIS_ const DPNID idGroup, const DPNID idClient, PVOID const pvAsyncContext, DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(SetGroupInfo)				(THIS_ const DPNID dpnid,DPN_GROUP_INFO *const pdpnGroupInfo,PVOID const pvAsyncContext,DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(GetGroupInfo)				(THIS_ const DPNID dpnid,DPN_GROUP_INFO *const pdpnGroupInfo,DWORD *const pdwSize,const DWORD dwFlags) PURE;
	STDMETHOD(EnumPlayersAndGroups)		(THIS_ DPNID *const prgdpnid, DWORD *const pcdpnid, const DWORD dwFlags) PURE;
	STDMETHOD(EnumGroupMembers)			(THIS_ const DPNID dpnid, DPNID *const prgdpnid, DWORD *const pcdpnid, const DWORD dwFlags) PURE;
	STDMETHOD(SetPeerInfo)				(THIS_ const DPN_PLAYER_INFO *const pdpnPlayerInfo,PVOID const pvAsyncContext,DPNHANDLE *const phAsyncHandle, const DWORD dwFlags) PURE;
	STDMETHOD(GetPeerInfo)				(THIS_ const DPNID dpnid,DPN_PLAYER_INFO *const pdpnPlayerInfo,DWORD *const pdwSize,const DWORD dwFlags) PURE;
	STDMETHOD(GetPeerAddress)			(THIS_ const DPNID dpnid,IDirectPlay8Address **const ppAddress,const DWORD dwFlags) PURE;
	STDMETHOD(GetLocalHostAddresses)	(THIS_ IDirectPlay8Address **const prgpAddress,DWORD *const pcAddress,const DWORD dwFlags) PURE;
	STDMETHOD(Close)					(THIS_ const DWORD dwFlags) PURE;
	STDMETHOD(EnumHosts)				(THIS_ PDPN_APPLICATION_DESC const pApplicationDesc,IDirectPlay8Address *const pAddrHost,IDirectPlay8Address *const pDeviceInfo,PVOID const pUserEnumData,const DWORD dwUserEnumDataSize,const DWORD dwEnumCount,const DWORD dwRetryInterval,const DWORD dwTimeOut,PVOID const pvUserContext,DPNHANDLE *const pAsyncHandle,const DWORD dwFlags) PURE;
	STDMETHOD(DestroyPeer)				(THIS_ const DPNID dpnidClient, const void *const pvDestroyData, const DWORD dwDestroyDataSize, const DWORD dwFlags) PURE;
	STDMETHOD(ReturnBuffer)				(THIS_ const DPNHANDLE hBufferHandle,const DWORD dwFlags) PURE;
	STDMETHOD(GetPlayerContext)			(THIS_ const DPNID dpnid,PVOID *const ppvPlayerContext,const DWORD dwFlags) PURE;
	STDMETHOD(GetGroupContext)			(THIS_ const DPNID dpnid,PVOID *const ppvGroupContext,const DWORD dwFlags) PURE;
	STDMETHOD(GetCaps)					(THIS_ DPN_CAPS *const pdpCaps,const DWORD dwFlags) PURE;
	STDMETHOD(SetCaps)					(THIS_ const DPN_CAPS *const pdpCaps, const DWORD dwFlags) PURE;
	STDMETHOD(SetSPCaps)				(THIS_ const GUID * const pguidSP, const DPN_SP_CAPS *const pdpspCaps, const DWORD dwFlags ) PURE;
	STDMETHOD(GetSPCaps)				(THIS_ const GUID * const pguidSP, DPN_SP_CAPS *const pdpspCaps,const DWORD dwFlags) PURE;
	STDMETHOD(GetConnectionInfo)		(THIS_ const DPNID dpnid, DPN_CONNECTION_INFO *const pdpConnectionInfo,const DWORD dwFlags) PURE;
	STDMETHOD(RegisterLobby)			(THIS_ const DPNHANDLE dpnHandle, struct IDirectPlay8LobbiedApplication *const pIDP8LobbiedApplication,const DWORD dwFlags) PURE;
	STDMETHOD(TerminateSession)			(THIS_ void *const pvTerminateData,const DWORD dwTerminateDataSize,const DWORD dwFlags) PURE;
};



//
// COM definition for DirectPlay8 Thread Pool interface
//
#undef INTERFACE				// External COM Implementation
#define INTERFACE IDirectPlay8ThreadPool
DECLARE_INTERFACE_(IDirectPlay8ThreadPool,IUnknown)
{
	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)			(THIS_ DP8REFIID riid, LPVOID *ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)			(THIS) PURE;
	STDMETHOD_(ULONG,Release)			(THIS) PURE;
	/*** IDirectPlay8ThreadPool methods ***/
	STDMETHOD(Initialize)				(THIS_ PVOID const pvUserContext, const PFNDPNMESSAGEHANDLER pfn, const DWORD dwFlags) PURE;
	STDMETHOD(Close)					(THIS_ const DWORD dwFlags) PURE;
	STDMETHOD(GetThreadCount)			(THIS_ const DWORD dwProcessorNum, DWORD *const pdwNumThreads, const DWORD dwFlags) PURE;
	STDMETHOD(SetThreadCount)			(THIS_ const DWORD dwProcessorNum, const DWORD dwNumThreads, const DWORD dwFlags) PURE;
	STDMETHOD(DoWork)					(THIS_ const DWORD dwAllowedTimeSlice, const DWORD dwFlags) PURE;
};


//
// COM definition for DirectPlay8 NAT Resolver interface
//
#undef INTERFACE				// External COM Implementation
#define INTERFACE IDirectPlay8NATResolver
DECLARE_INTERFACE_(IDirectPlay8NATResolver,IUnknown)
{
	/*** IUnknown methods ***/
	STDMETHOD(QueryInterface)			(THIS_ DP8REFIID riid, LPVOID *ppvObj) PURE;
	STDMETHOD_(ULONG,AddRef)			(THIS) PURE;
	STDMETHOD_(ULONG,Release)			(THIS) PURE;
	/*** IDirectPlay8NATResolver methods ***/
	STDMETHOD(Initialize)				(THIS_ PVOID const pvUserContext, const PFNDPNMESSAGEHANDLER pfn, const DWORD dwFlags) PURE;
	STDMETHOD(Start)					(THIS_ IDirectPlay8Address **const ppDevices, const DWORD dwNumDevices, const DWORD dwFlags) PURE;
	STDMETHOD(Close)					(THIS_ const DWORD dwFlags) PURE;
	STDMETHOD(EnumDevices)				(THIS_ DPN_SERVICE_PROVIDER_INFO *const pSPInfoBuffer, PDWORD const pdwBufferSize, PDWORD const pdwNumDevices, const DWORD dwFlags) PURE;
	STDMETHOD(GetAddresses)				(THIS_ IDirectPlay8Address **const ppAddresses, DWORD *const pdwNumAddresses, const DWORD dwFlags) PURE;
};


/****************************************************************************
 *
 * IDirectPlay8 application interface macros
 *
 ****************************************************************************/

#if !defined(__cplusplus) || defined(CINTERFACE)

#define IDirectPlay8Client_QueryInterface(p,a,b)					(p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectPlay8Client_AddRef(p)								(p)->lpVtbl->AddRef(p)
#define IDirectPlay8Client_Release(p)								(p)->lpVtbl->Release(p)
#define IDirectPlay8Client_Initialize(p,a,b,c)						(p)->lpVtbl->Initialize(p,a,b,c)
#define IDirectPlay8Client_EnumServiceProviders(p,a,b,c,d,e,f)		(p)->lpVtbl->EnumServiceProviders(p,a,b,c,d,e,f)
#define IDirectPlay8Client_EnumHosts(p,a,b,c,d,e,f,g,h,i,j,k)		(p)->lpVtbl->EnumHosts(p,a,b,c,d,e,f,g,h,i,j,k)
#define IDirectPlay8Client_CancelAsyncOperation(p,a,b)				(p)->lpVtbl->CancelAsyncOperation(p,a,b)
#define IDirectPlay8Client_Connect(p,a,b,c,d,e,f,g,h,i,j)			(p)->lpVtbl->Connect(p,a,b,c,d,e,f,g,h,i,j)
#define IDirectPlay8Client_Send(p,a,b,c,d,e,f)						(p)->lpVtbl->Send(p,a,b,c,d,e,f)
#define IDirectPlay8Client_GetSendQueueInfo(p,a,b,c)				(p)->lpVtbl->GetSendQueueInfo(p,a,b,c)
#define IDirectPlay8Client_GetApplicationDesc(p,a,b,c)				(p)->lpVtbl->GetApplicationDesc(p,a,b,c)
#define IDirectPlay8Client_SetClientInfo(p,a,b,c,d)					(p)->lpVtbl->SetClientInfo(p,a,b,c,d)
#define IDirectPlay8Client_GetServerInfo(p,a,b,c)					(p)->lpVtbl->GetServerInfo(p,a,b,c)
#define IDirectPlay8Client_GetServerAddress(p,a,b)					(p)->lpVtbl->GetServerAddress(p,a,b)
#define IDirectPlay8Client_Close(p,a)								(p)->lpVtbl->Close(p,a)
#define IDirectPlay8Client_ReturnBuffer(p,a,b)						(p)->lpVtbl->ReturnBuffer(p,a,b)
#define IDirectPlay8Client_GetCaps(p,a,b)							(p)->lpVtbl->GetCaps(p,a,b)
#define IDirectPlay8Client_SetCaps(p,a,b)							(p)->lpVtbl->SetCaps(p,a,b)
#define IDirectPlay8Client_SetSPCaps(p,a,b,c)						(p)->lpVtbl->SetSPCaps(p,a,b,c)
#define IDirectPlay8Client_GetSPCaps(p,a,b,c)						(p)->lpVtbl->GetSPCaps(p,a,b,c)
#define IDirectPlay8Client_GetConnectionInfo(p,a,b)					(p)->lpVtbl->GetConnectionInfo(p,a,b)
#define IDirectPlay8Client_RegisterLobby(p,a,b,c)					(p)->lpVtbl->RegisterLobby(p,a,b,c)

#define IDirectPlay8Server_QueryInterface(p,a,b)					(p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectPlay8Server_AddRef(p)								(p)->lpVtbl->AddRef(p)
#define IDirectPlay8Server_Release(p)								(p)->lpVtbl->Release(p)
#define IDirectPlay8Server_Initialize(p,a,b,c)						(p)->lpVtbl->Initialize(p,a,b,c)
#define IDirectPlay8Server_EnumServiceProviders(p,a,b,c,d,e,f)		(p)->lpVtbl->EnumServiceProviders(p,a,b,c,d,e,f)
#define IDirectPlay8Server_CancelAsyncOperation(p,a,b)				(p)->lpVtbl->CancelAsyncOperation(p,a,b)
#define IDirectPlay8Server_GetSendQueueInfo(p,a,b,c,d)				(p)->lpVtbl->GetSendQueueInfo(p,a,b,c,d)
#define IDirectPlay8Server_GetApplicationDesc(p,a,b,c)				(p)->lpVtbl->GetApplicationDesc(p,a,b,c)
#define IDirectPlay8Server_SetServerInfo(p,a,b,c,d)					(p)->lpVtbl->SetServerInfo(p,a,b,c,d)
#define IDirectPlay8Server_GetClientInfo(p,a,b,c,d)					(p)->lpVtbl->GetClientInfo(p,a,b,c,d)
#define IDirectPlay8Server_GetClientAddress(p,a,b,c)				(p)->lpVtbl->GetClientAddress(p,a,b,c)
#define IDirectPlay8Server_GetLocalHostAddresses(p,a,b,c)			(p)->lpVtbl->GetLocalHostAddresses(p,a,b,c)
#define IDirectPlay8Server_SetApplicationDesc(p,a,b)				(p)->lpVtbl->SetApplicationDesc(p,a,b)
#define IDirectPlay8Server_Host(p,a,b,c,d,e,f,g)					(p)->lpVtbl->Host(p,a,b,c,d,e,f,g)
#define IDirectPlay8Server_SendTo(p,a,b,c,d,e,f,g)					(p)->lpVtbl->SendTo(p,a,b,c,d,e,f,g)
#define IDirectPlay8Server_CreateGroup(p,a,b,c,d,e)					(p)->lpVtbl->CreateGroup(p,a,b,c,d,e)
#define IDirectPlay8Server_DestroyGroup(p,a,b,c,d)					(p)->lpVtbl->DestroyGroup(p,a,b,c,d)
#define IDirectPlay8Server_AddPlayerToGroup(p,a,b,c,d,e)			(p)->lpVtbl->AddPlayerToGroup(p,a,b,c,d,e)
#define IDirectPlay8Server_RemovePlayerFromGroup(p,a,b,c,d,e)		(p)->lpVtbl->RemovePlayerFromGroup(p,a,b,c,d,e)
#define IDirectPlay8Server_SetGroupInfo(p,a,b,c,d,e)				(p)->lpVtbl->SetGroupInfo(p,a,b,c,d,e)
#define IDirectPlay8Server_GetGroupInfo(p,a,b,c,d)					(p)->lpVtbl->GetGroupInfo(p,a,b,c,d)
#define IDirectPlay8Server_EnumPlayersAndGroups(p,a,b,c)			(p)->lpVtbl->EnumPlayersAndGroups(p,a,b,c)
#define IDirectPlay8Server_EnumGroupMembers(p,a,b,c,d)				(p)->lpVtbl->EnumGroupMembers(p,a,b,c,d)
#define IDirectPlay8Server_Close(p,a)								(p)->lpVtbl->Close(p,a)
#define IDirectPlay8Server_DestroyClient(p,a,b,c,d)					(p)->lpVtbl->DestroyClient(p,a,b,c,d)
#define IDirectPlay8Server_ReturnBuffer(p,a,b)						(p)->lpVtbl->ReturnBuffer(p,a,b)
#define IDirectPlay8Server_GetPlayerContext(p,a,b,c)				(p)->lpVtbl->GetPlayerContext(p,a,b,c)
#define IDirectPlay8Server_GetGroupContext(p,a,b,c)					(p)->lpVtbl->GetGroupContext(p,a,b,c)
#define IDirectPlay8Server_GetCaps(p,a,b)							(p)->lpVtbl->GetCaps(p,a,b)
#define IDirectPlay8Server_SetCaps(p,a,b)							(p)->lpVtbl->SetCaps(p,a,b)
#define IDirectPlay8Server_SetSPCaps(p,a,b,c)						(p)->lpVtbl->SetSPCaps(p,a,b,c)
#define IDirectPlay8Server_GetSPCaps(p,a,b,c)						(p)->lpVtbl->GetSPCaps(p,a,b,c)
#define IDirectPlay8Server_GetConnectionInfo(p,a,b,c)				(p)->lpVtbl->GetConnectionInfo(p,a,b,c)
#define IDirectPlay8Server_RegisterLobby(p,a,b,c)					(p)->lpVtbl->RegisterLobby(p,a,b,c)

#define IDirectPlay8Peer_QueryInterface(p,a,b)						(p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectPlay8Peer_AddRef(p)									(p)->lpVtbl->AddRef(p)
#define IDirectPlay8Peer_Release(p)									(p)->lpVtbl->Release(p)
#define IDirectPlay8Peer_Initialize(p,a,b,c)						(p)->lpVtbl->Initialize(p,a,b,c)
#define IDirectPlay8Peer_EnumServiceProviders(p,a,b,c,d,e,f)		(p)->lpVtbl->EnumServiceProviders(p,a,b,c,d,e,f)
#define IDirectPlay8Peer_CancelAsyncOperation(p,a,b)				(p)->lpVtbl->CancelAsyncOperation(p,a,b)
#define IDirectPlay8Peer_Connect(p,a,b,c,d,e,f,g,h,i,j,k)			(p)->lpVtbl->Connect(p,a,b,c,d,e,f,g,h,i,j,k)
#define IDirectPlay8Peer_SendTo(p,a,b,c,d,e,f,g)					(p)->lpVtbl->SendTo(p,a,b,c,d,e,f,g)
#define IDirectPlay8Peer_GetSendQueueInfo(p,a,b,c,d)				(p)->lpVtbl->GetSendQueueInfo(p,a,b,c,d)
#define IDirectPlay8Peer_Host(p,a,b,c,d,e,f,g)						(p)->lpVtbl->Host(p,a,b,c,d,e,f,g)
#define IDirectPlay8Peer_GetApplicationDesc(p,a,b,c)				(p)->lpVtbl->GetApplicationDesc(p,a,b,c)
#define IDirectPlay8Peer_SetApplicationDesc(p,a,b)					(p)->lpVtbl->SetApplicationDesc(p,a,b)
#define IDirectPlay8Peer_CreateGroup(p,a,b,c,d,e)					(p)->lpVtbl->CreateGroup(p,a,b,c,d,e)
#define IDirectPlay8Peer_DestroyGroup(p,a,b,c,d)					(p)->lpVtbl->DestroyGroup(p,a,b,c,d)
#define IDirectPlay8Peer_AddPlayerToGroup(p,a,b,c,d,e)				(p)->lpVtbl->AddPlayerToGroup(p,a,b,c,d,e)
#define IDirectPlay8Peer_RemovePlayerFromGroup(p,a,b,c,d,e)			(p)->lpVtbl->RemovePlayerFromGroup(p,a,b,c,d,e)
#define IDirectPlay8Peer_SetGroupInfo(p,a,b,c,d,e)					(p)->lpVtbl->SetGroupInfo(p,a,b,c,d,e)
#define IDirectPlay8Peer_GetGroupInfo(p,a,b,c,d)					(p)->lpVtbl->GetGroupInfo(p,a,b,c,d)
#define IDirectPlay8Peer_EnumPlayersAndGroups(p,a,b,c)				(p)->lpVtbl->EnumPlayersAndGroups(p,a,b,c)
#define IDirectPlay8Peer_EnumGroupMembers(p,a,b,c,d)				(p)->lpVtbl->EnumGroupMembers(p,a,b,c,d)
#define IDirectPlay8Peer_SetPeerInfo(p,a,b,c,d)						(p)->lpVtbl->SetPeerInfo(p,a,b,c,d)
#define IDirectPlay8Peer_GetPeerInfo(p,a,b,c,d)						(p)->lpVtbl->GetPeerInfo(p,a,b,c,d)
#define IDirectPlay8Peer_GetPeerAddress(p,a,b,c)					(p)->lpVtbl->GetPeerAddress(p,a,b,c)
#define IDirectPlay8Peer_GetLocalHostAddresses(p,a,b,c)				(p)->lpVtbl->GetLocalHostAddresses(p,a,b,c)
#define IDirectPlay8Peer_Close(p,a)									(p)->lpVtbl->Close(p,a)
#define IDirectPlay8Peer_EnumHosts(p,a,b,c,d,e,f,g,h,i,j,k)			(p)->lpVtbl->EnumHosts(p,a,b,c,d,e,f,g,h,i,j,k)
#define IDirectPlay8Peer_DestroyPeer(p,a,b,c,d)						(p)->lpVtbl->DestroyPeer(p,a,b,c,d)
#define IDirectPlay8Peer_ReturnBuffer(p,a,b)						(p)->lpVtbl->ReturnBuffer(p,a,b)
#define IDirectPlay8Peer_GetPlayerContext(p,a,b,c)					(p)->lpVtbl->GetPlayerContext(p,a,b,c)
#define IDirectPlay8Peer_GetGroupContext(p,a,b,c)					(p)->lpVtbl->GetGroupContext(p,a,b,c)
#define IDirectPlay8Peer_GetCaps(p,a,b)								(p)->lpVtbl->GetCaps(p,a,b)
#define IDirectPlay8Peer_SetCaps(p,a,b)								(p)->lpVtbl->SetCaps(p,a,b)
#define IDirectPlay8Peer_SetSPCaps(p,a,b,c)							(p)->lpVtbl->SetSPCaps(p,a,b,c)
#define IDirectPlay8Peer_GetSPCaps(p,a,b,c)							(p)->lpVtbl->GetSPCaps(p,a,b,c)
#define IDirectPlay8Peer_GetConnectionInfo(p,a,b,c)					(p)->lpVtbl->GetConnectionInfo(p,a,b,c)
#define IDirectPlay8Peer_RegisterLobby(p,a,b,c)						(p)->lpVtbl->RegisterLobby(p,a,b,c)
#define IDirectPlay8Peer_TerminateSession(p,a,b,c)					(p)->lpVtbl->TerminateSession(p,a,b,c)

#define IDirectPlay8ThreadPool_QueryInterface(p,a,b)				(p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectPlay8ThreadPool_AddRef(p)							(p)->lpVtbl->AddRef(p)
#define IDirectPlay8ThreadPool_Release(p)							(p)->lpVtbl->Release(p)
#define IDirectPlay8ThreadPool_Initialize(p,a,b,c)					(p)->lpVtbl->Initialize(p,a,b,c)
#define IDirectPlay8ThreadPool_Close(p,a)							(p)->lpVtbl->Close(p,a)
#define IDirectPlay8ThreadPool_GetThreadCount(p,a,b,c)				(p)->lpVtbl->GetThreadCount(p,a,b,c)
#define IDirectPlay8ThreadPool_SetThreadCount(p,a,b,c)				(p)->lpVtbl->SetThreadCount(p,a,b,c)
#define IDirectPlay8ThreadPool_DoWork(p,a,b)						(p)->lpVtbl->DoWork(p,a,b)

#define IDirectPlay8NATResolver_QueryInterface(p,a,b)				(p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectPlay8NATResolver_AddRef(p)							(p)->lpVtbl->AddRef(p)
#define IDirectPlay8NATResolver_Release(p)							(p)->lpVtbl->Release(p)
#define IDirectPlay8NATResolver_Initialize(p,a,b,c)					(p)->lpVtbl->Initialize(p,a,b,c)
#define IDirectPlay8NATResolver_Start(p,a,b,c)						(p)->lpVtbl->Start(p,a,b,c)
#define IDirectPlay8NATResolver_Close(p,a)							(p)->lpVtbl->Close(p,a)
#define IDirectPlay8NATResolver_EnumDevices(p,a,b,c,d)				(p)->lpVtbl->EnumDevices(p,a,b,c,d)
#define IDirectPlay8NATResolver_GetAddresses(p,a,b,c)				(p)->lpVtbl->GetAddresses(p,a,b,c)

#else /* C++ */

#define IDirectPlay8Client_QueryInterface(p,a,b)					(p)->QueryInterface(a,b)
#define IDirectPlay8Client_AddRef(p)								(p)->AddRef()
#define IDirectPlay8Client_Release(p)								(p)->Release()
#define IDirectPlay8Client_Initialize(p,a,b,c)						(p)->Initialize(a,b,c)
#define IDirectPlay8Client_EnumServiceProviders(p,a,b,c,d,e,f)		(p)->EnumServiceProviders(a,b,c,d,e,f)
#define IDirectPlay8Client_EnumHosts(p,a,b,c,d,e,f,g,h,i,j,k)		(p)->EnumHosts(a,b,c,d,e,f,g,h,i,j,k)
#define IDirectPlay8Client_CancelAsyncOperation(p,a,b)				(p)->CancelAsyncOperation(a,b)
#define IDirectPlay8Client_Connect(p,a,b,c,d,e,f,g,h,i,j)			(p)->Connect(a,b,c,d,e,f,g,h,i,j)
#define IDirectPlay8Client_Send(p,a,b,c,d,e,f)						(p)->Send(a,b,c,d,e,f)
#define IDirectPlay8Client_GetSendQueueInfo(p,a,b,c)				(p)->GetSendQueueInfo(a,b,c)
#define IDirectPlay8Client_GetApplicationDesc(p,a,b,c)				(p)->GetApplicationDesc(a,b,c)
#define IDirectPlay8Client_SetClientInfo(p,a,b,c,d)					(p)->SetClientInfo(a,b,c,d)
#define IDirectPlay8Client_GetServerInfo(p,a,b,c)					(p)->GetServerInfo(a,b,c)
#define IDirectPlay8Client_GetServerAddress(p,a,b)					(p)->GetServerAddress(a,b)
#define IDirectPlay8Client_Close(p,a)								(p)->Close(a)
#define IDirectPlay8Client_ReturnBuffer(p,a,b)						(p)->ReturnBuffer(a,b)
#define IDirectPlay8Client_GetCaps(p,a,b)							(p)->GetCaps(a,b)
#define IDirectPlay8Client_SetCaps(p,a,b)							(p)->SetCaps(a,b)
#define IDirectPlay8Client_SetSPCaps(p,a,b,c)						(p)->SetSPCaps(a,b,c)
#define IDirectPlay8Client_GetSPCaps(p,a,b,c)						(p)->GetSPCaps(a,b,c)
#define IDirectPlay8Client_GetConnectionInfo(p,a,b)					(p)->GetConnectionInfo(a,b)
#define IDirectPlay8Client_RegisterLobby(p,a,b,c)					(p)->RegisterLobby(a,b,c)

#define IDirectPlay8Server_QueryInterface(p,a,b)					(p)->QueryInterface(a,b)
#define IDirectPlay8Server_AddRef(p)								(p)->AddRef()
#define IDirectPlay8Server_Release(p)								(p)->Release()
#define IDirectPlay8Server_Initialize(p,a,b,c)						(p)->Initialize(a,b,c)
#define IDirectPlay8Server_EnumServiceProviders(p,a,b,c,d,e,f)		(p)->EnumServiceProviders(a,b,c,d,e,f)
#define IDirectPlay8Server_CancelAsyncOperation(p,a,b)				(p)->CancelAsyncOperation(a,b)
#define IDirectPlay8Server_GetSendQueueInfo(p,a,b,c,d)				(p)->GetSendQueueInfo(a,b,c,d)
#define IDirectPlay8Server_GetApplicationDesc(p,a,b,c)				(p)->GetApplicationDesc(a,b,c)
#define IDirectPlay8Server_SetServerInfo(p,a,b,c,d)					(p)->SetServerInfo(a,b,c,d)
#define IDirectPlay8Server_GetClientInfo(p,a,b,c,d)					(p)->GetClientInfo(a,b,c,d)
#define IDirectPlay8Server_GetClientAddress(p,a,b,c)				(p)->GetClientAddress(a,b,c)
#define IDirectPlay8Server_GetLocalHostAddresses(p,a,b,c)			(p)->GetLocalHostAddresses(a,b,c)
#define IDirectPlay8Server_SetApplicationDesc(p,a,b)				(p)->SetApplicationDesc(a,b)
#define IDirectPlay8Server_Host(p,a,b,c,d,e,f,g)					(p)->Host(a,b,c,d,e,f,g)
#define IDirectPlay8Server_SendTo(p,a,b,c,d,e,f,g)					(p)->SendTo(a,b,c,d,e,f,g)
#define IDirectPlay8Server_CreateGroup(p,a,b,c,d,e)					(p)->CreateGroup(a,b,c,d,e)
#define IDirectPlay8Server_DestroyGroup(p,a,b,c,d)					(p)->DestroyGroup(a,b,c,d)
#define IDirectPlay8Server_AddPlayerToGroup(p,a,b,c,d,e)			(p)->AddPlayerToGroup(a,b,c,d,e)
#define IDirectPlay8Server_RemovePlayerFromGroup(p,a,b,c,d,e)		(p)->RemovePlayerFromGroup(a,b,c,d,e)
#define IDirectPlay8Server_SetGroupInfo(p,a,b,c,d,e)				(p)->SetGroupInfo(a,b,c,d,e)
#define IDirectPlay8Server_GetGroupInfo(p,a,b,c,d)					(p)->GetGroupInfo(a,b,c,d)
#define IDirectPlay8Server_EnumPlayersAndGroups(p,a,b,c)			(p)->EnumPlayersAndGroups(a,b,c)
#define IDirectPlay8Server_EnumGroupMembers(p,a,b,c,d)				(p)->EnumGroupMembers(a,b,c,d)
#define IDirectPlay8Server_Close(p,a)								(p)->Close(a)
#define IDirectPlay8Server_DestroyClient(p,a,b,c,d)					(p)->DestroyClient(a,b,c,d)
#define IDirectPlay8Server_ReturnBuffer(p,a,b)						(p)->ReturnBuffer(a,b)
#define IDirectPlay8Server_GetPlayerContext(p,a,b,c)				(p)->GetPlayerContext(a,b,c)
#define IDirectPlay8Server_GetGroupContext(p,a,b,c)					(p)->GetGroupContext(a,b,c)
#define IDirectPlay8Server_GetCaps(p,a,b)							(p)->GetCaps(a,b)
#define IDirectPlay8Server_SetCaps(p,a,b)							(p)->SetCaps(a,b)
#define IDirectPlay8Server_SetSPCaps(p,a,b,c)						(p)->SetSPCaps(a,b,c)
#define IDirectPlay8Server_GetSPCaps(p,a,b,c)						(p)->GetSPCaps(a,b,c)
#define IDirectPlay8Server_GetConnectionInfo(p,a,b,c)				(p)->GetConnectionInfo(a,b,c)
#define IDirectPlay8Server_RegisterLobby(p,a,b,c)					(p)->RegisterLobby(a,b,c)

#define IDirectPlay8Peer_QueryInterface(p,a,b)						(p)->QueryInterface(a,b)
#define IDirectPlay8Peer_AddRef(p)									(p)->AddRef()
#define IDirectPlay8Peer_Release(p)									(p)->Release()
#define IDirectPlay8Peer_Initialize(p,a,b,c)						(p)->Initialize(a,b,c)
#define IDirectPlay8Peer_EnumServiceProviders(p,a,b,c,d,e,f)		(p)->EnumServiceProviders(a,b,c,d,e,f)
#define IDirectPlay8Peer_CancelAsyncOperation(p,a,b)				(p)->CancelAsyncOperation(a,b)
#define IDirectPlay8Peer_Connect(p,a,b,c,d,e,f,g,h,i,j,k)			(p)->Connect(a,b,c,d,e,f,g,h,i,j,k)
#define IDirectPlay8Peer_SendTo(p,a,b,c,d,e,f,g)					(p)->SendTo(a,b,c,d,e,f,g)
#define IDirectPlay8Peer_GetSendQueueInfo(p,a,b,c,d)				(p)->GetSendQueueInfo(a,b,c,d)
#define IDirectPlay8Peer_Host(p,a,b,c,d,e,f,g)						(p)->Host(a,b,c,d,e,f,g)
#define IDirectPlay8Peer_GetApplicationDesc(p,a,b,c)				(p)->GetApplicationDesc(a,b,c)
#define IDirectPlay8Peer_SetApplicationDesc(p,a,b)					(p)->SetApplicationDesc(a,b)
#define IDirectPlay8Peer_CreateGroup(p,a,b,c,d,e)					(p)->CreateGroup(a,b,c,d,e)
#define IDirectPlay8Peer_DestroyGroup(p,a,b,c,d)					(p)->DestroyGroup(a,b,c,d)
#define IDirectPlay8Peer_AddPlayerToGroup(p,a,b,c,d,e)				(p)->AddPlayerToGroup(a,b,c,d,e)
#define IDirectPlay8Peer_RemovePlayerFromGroup(p,a,b,c,d,e)			(p)->RemovePlayerFromGroup(a,b,c,d,e)
#define IDirectPlay8Peer_SetGroupInfo(p,a,b,c,d,e)					(p)->SetGroupInfo(a,b,c,d,e)
#define IDirectPlay8Peer_GetGroupInfo(p,a,b,c,d)					(p)->GetGroupInfo(a,b,c,d)
#define IDirectPlay8Peer_EnumPlayersAndGroups(p,a,b,c)				(p)->EnumPlayersAndGroups(a,b,c)
#define IDirectPlay8Peer_EnumGroupMembers(p,a,b,c,d)				(p)->EnumGroupMembers(a,b,c,d)
#define IDirectPlay8Peer_SetPeerInfo(p,a,b,c,d)						(p)->SetPeerInfo(a,b,c,d)
#define IDirectPlay8Peer_GetPeerInfo(p,a,b,c,d)						(p)->GetPeerInfo(a,b,c,d)
#define IDirectPlay8Peer_GetPeerAddress(p,a,b,c)					(p)->GetPeerAddress(a,b,c)
#define IDirectPlay8Peer_GetLocalHostAddresses(p,a,b,c)				(p)->GetLocalHostAddresses(a,b,c)
#define IDirectPlay8Peer_Close(p,a)									(p)->Close(a)
#define IDirectPlay8Peer_EnumHosts(p,a,b,c,d,e,f,g,h,i,j,k)			(p)->EnumHosts(a,b,c,d,e,f,g,h,i,j,k)
#define IDirectPlay8Peer_DestroyPeer(p,a,b,c,d)						(p)->DestroyPeer(a,b,c,d)
#define IDirectPlay8Peer_ReturnBuffer(p,a,b)						(p)->ReturnBuffer(a,b)
#define IDirectPlay8Peer_GetPlayerContext(p,a,b,c)					(p)->GetPlayerContext(a,b,c)
#define IDirectPlay8Peer_GetGroupContext(p,a,b,c)					(p)->GetGroupContext(a,b,c)
#define IDirectPlay8Peer_GetCaps(p,a,b)								(p)->GetCaps(a,b)
#define IDirectPlay8Peer_SetCaps(p,a,b)								(p)->SetCaps(a,b)
#define IDirectPlay8Peer_SetSPCaps(p,a,b,c)							(p)->SetSPCaps(a,b,c)
#define IDirectPlay8Peer_GetSPCaps(p,a,b,c)							(p)->GetSPCaps(a,b,c)
#define IDirectPlay8Peer_GetConnectionInfo(p,a,b,c)					(p)->GetConnectionInfo(a,b,c)
#define IDirectPlay8Peer_RegisterLobby(p,a,b,c)						(p)->RegisterLobby(a,b,c)
#define IDirectPlay8Peer_TerminateSession(p,a,b,c)					(p)->TerminateSession(a,b,c)

#define IDirectPlay8ThreadPool_QueryInterface(p,a,b)				(p)->QueryInterface(a,b)
#define IDirectPlay8ThreadPool_AddRef(p)							(p)->AddRef()
#define IDirectPlay8ThreadPool_Release(p)							(p)->Release()
#define IDirectPlay8ThreadPool_Initialize(p,a,b,c)					(p)->Initialize(a,b,c)
#define IDirectPlay8ThreadPool_Close(p,a)							(p)->Close(a)
#define IDirectPlay8ThreadPool_GetThreadCount(p,a,b,c)				(p)->GetThreadCount(a,b,c)
#define IDirectPlay8ThreadPool_SetThreadCount(p,a,b,c)				(p)->SetThreadCount(a,b,c)
#define IDirectPlay8ThreadPool_DoWork(p,a,b)						(p)->DoWork(a,b)

#define IDirectPlay8NATResolver_QueryInterface(p,a,b)				(p)->QueryInterface(a,b)
#define IDirectPlay8NATResolver_AddRef(p)							(p)->AddRef()
#define IDirectPlay8NATResolver_Release(p)							(p)->Release()
#define IDirectPlay8NATResolver_Initialize(p,a,b,c)					(p)->Initialize(a,b,c)
#define IDirectPlay8NATResolver_Start(p,a,b,c)						(p)->Start(a,b,c)
#define IDirectPlay8NATResolver_Close(p,a)							(p)->Close(a)
#define IDirectPlay8NATResolver_EnumDevices(p,a,b,c,d)				(p)->EnumDevices(a,b,c,d)
#define IDirectPlay8NATResolver_GetAddresses(p,a,b,c)				(p)->GetAddresses(a,b,c)

#endif



/****************************************************************************
 *
 * DIRECTPLAY8 ERRORS
 *
 * Errors are represented by negative values and cannot be combined.
 *
 ****************************************************************************/

#define _DPN_FACILITY_CODE				0x015
#define _DPNHRESULT_BASE				0x8000
#define MAKE_DPNHRESULT( code )			MAKE_HRESULT( 1, _DPN_FACILITY_CODE, ( code + _DPNHRESULT_BASE ) )

#define DPN_OK							S_OK

#define DPNSUCCESS_EQUAL				MAKE_HRESULT( 0, _DPN_FACILITY_CODE, ( 0x5 + _DPNHRESULT_BASE ) )
#define DPNSUCCESS_NOPLAYERSINGROUP		MAKE_HRESULT( 0, _DPN_FACILITY_CODE, ( 0x8 + _DPNHRESULT_BASE ) )	// added for DirectX 9
#define DPNSUCCESS_NOTEQUAL				MAKE_HRESULT( 0, _DPN_FACILITY_CODE, (0x0A + _DPNHRESULT_BASE ) )
#define DPNSUCCESS_PENDING				MAKE_HRESULT( 0, _DPN_FACILITY_CODE, (0x0e + _DPNHRESULT_BASE ) )

#define DPNERR_ABORTED					MAKE_DPNHRESULT(  0x30 )
#define DPNERR_ADDRESSING				MAKE_DPNHRESULT(  0x40 )
#define DPNERR_ALREADYCLOSING			MAKE_DPNHRESULT(  0x50 )
#define DPNERR_ALREADYCONNECTED			MAKE_DPNHRESULT(  0x60 )
#define DPNERR_ALREADYDISCONNECTING		MAKE_DPNHRESULT(  0x70 )
#define DPNERR_ALREADYINITIALIZED		MAKE_DPNHRESULT(  0x80 )
#define DPNERR_ALREADYREGISTERED		MAKE_DPNHRESULT(  0x90 )
#define DPNERR_BUFFERTOOSMALL			MAKE_DPNHRESULT( 0x100 )
#define DPNERR_CANNOTCANCEL				MAKE_DPNHRESULT( 0x110 )
#define DPNERR_CANTCREATEGROUP			MAKE_DPNHRESULT( 0x120 )
#define DPNERR_CANTCREATEPLAYER			MAKE_DPNHRESULT( 0x130 )
#define DPNERR_CANTLAUNCHAPPLICATION	MAKE_DPNHRESULT( 0x140 )
#define DPNERR_CONNECTING				MAKE_DPNHRESULT( 0x150 )
#define DPNERR_CONNECTIONLOST			MAKE_DPNHRESULT( 0x160 )
#define DPNERR_CONVERSION				MAKE_DPNHRESULT( 0x170 )
#define DPNERR_DATATOOLARGE				MAKE_DPNHRESULT( 0x175 )
#define DPNERR_DOESNOTEXIST				MAKE_DPNHRESULT( 0x180 )
#define DPNERR_DPNSVRNOTAVAILABLE		MAKE_DPNHRESULT( 0x185 )
#define DPNERR_DUPLICATECOMMAND			MAKE_DPNHRESULT( 0x190 )
#define DPNERR_ENDPOINTNOTRECEIVING		MAKE_DPNHRESULT( 0x200 )
#define DPNERR_ENUMQUERYTOOLARGE		MAKE_DPNHRESULT( 0x210 )
#define DPNERR_ENUMRESPONSETOOLARGE		MAKE_DPNHRESULT( 0x220 )
#define DPNERR_EXCEPTION				MAKE_DPNHRESULT( 0x230 )
#define DPNERR_GENERIC					E_FAIL
#define DPNERR_GROUPNOTEMPTY			MAKE_DPNHRESULT( 0x240 )
#define DPNERR_HOSTING					MAKE_DPNHRESULT( 0x250 )
#define DPNERR_HOSTREJECTEDCONNECTION	MAKE_DPNHRESULT( 0x260 )
#define DPNERR_HOSTTERMINATEDSESSION	MAKE_DPNHRESULT( 0x270 )
#define DPNERR_INCOMPLETEADDRESS		MAKE_DPNHRESULT( 0x280 )
#define DPNERR_INVALIDADDRESSFORMAT		MAKE_DPNHRESULT( 0x290 )
#define DPNERR_INVALIDAPPLICATION		MAKE_DPNHRESULT( 0x300 )
#define DPNERR_INVALIDCOMMAND			MAKE_DPNHRESULT( 0x310 )
#define DPNERR_INVALIDDEVICEADDRESS		MAKE_DPNHRESULT( 0x320 )
#define DPNERR_INVALIDENDPOINT			MAKE_DPNHRESULT( 0x330 )
#define DPNERR_INVALIDFLAGS				MAKE_DPNHRESULT( 0x340 )
#define DPNERR_INVALIDGROUP			 	MAKE_DPNHRESULT( 0x350 )
#define DPNERR_INVALIDHANDLE			MAKE_DPNHRESULT( 0x360 )
#define DPNERR_INVALIDHOSTADDRESS		MAKE_DPNHRESULT( 0x370 )
#define DPNERR_INVALIDINSTANCE			MAKE_DPNHRESULT( 0x380 )
#define DPNERR_INVALIDINTERFACE			MAKE_DPNHRESULT( 0x390 )
#define DPNERR_INVALIDOBJECT			MAKE_DPNHRESULT( 0x400 )
#define DPNERR_INVALIDPARAM				E_INVALIDARG
#define DPNERR_INVALIDPASSWORD			MAKE_DPNHRESULT( 0x410 )
#define DPNERR_INVALIDPLAYER			MAKE_DPNHRESULT( 0x420 )
#define DPNERR_INVALIDPOINTER			E_POINTER
#define DPNERR_INVALIDPRIORITY			MAKE_DPNHRESULT( 0x430 )
#define DPNERR_INVALIDSTRING			MAKE_DPNHRESULT( 0x440 )
#define DPNERR_INVALIDURL				MAKE_DPNHRESULT( 0x450 )
#define DPNERR_INVALIDVERSION			MAKE_DPNHRESULT( 0x460 )
#define DPNERR_NOCAPS					MAKE_DPNHRESULT( 0x470 )
#define DPNERR_NOCONNECTION				MAKE_DPNHRESULT( 0x480 )
#define DPNERR_NOHOSTPLAYER				MAKE_DPNHRESULT( 0x490 )
#define DPNERR_NOINTERFACE				E_NOINTERFACE
#define DPNERR_NOMOREADDRESSCOMPONENTS	MAKE_DPNHRESULT( 0x500 )
#define DPNERR_NORESPONSE				MAKE_DPNHRESULT( 0x510 )
#define DPNERR_NOTALLOWED				MAKE_DPNHRESULT( 0x520 )
#define DPNERR_NOTHOST					MAKE_DPNHRESULT( 0x530 )
#define DPNERR_NOTREADY					MAKE_DPNHRESULT( 0x540 )
#define DPNERR_NOTREGISTERED			MAKE_DPNHRESULT( 0x550 )
#define DPNERR_OUTOFMEMORY				E_OUTOFMEMORY
#define DPNERR_PENDING					DPNSUCCESS_PENDING
#define DPNERR_PLAYERALREADYINGROUP		MAKE_DPNHRESULT( 0x560 )
#define DPNERR_PLAYERLOST				MAKE_DPNHRESULT( 0x570 )
#define DPNERR_PLAYERNOTINGROUP			MAKE_DPNHRESULT( 0x580 )
#define DPNERR_PLAYERNOTREACHABLE		MAKE_DPNHRESULT( 0x590 )
#define DPNERR_SENDTOOLARGE				MAKE_DPNHRESULT( 0x600 )
#define DPNERR_SESSIONFULL				MAKE_DPNHRESULT( 0x610 )
#define DPNERR_TABLEFULL				MAKE_DPNHRESULT( 0x620 )
#define DPNERR_TIMEDOUT					MAKE_DPNHRESULT( 0x630 )
#define DPNERR_UNINITIALIZED			MAKE_DPNHRESULT( 0x640 )
#define DPNERR_UNSUPPORTED				E_NOTIMPL
#define DPNERR_USERCANCEL				MAKE_DPNHRESULT( 0x650 )

#ifdef __cplusplus
}
#endif

#endif

