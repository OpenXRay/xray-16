#ifndef _SB_INTERNAL_H_
#define _SB_INTERNAL_H_


#include "../common/gsCommon.h"
#include "../common/gsAvailable.h"

#include "../darray.h"
#include "../hashtable.h"

#include "sb_serverbrowsing.h"
#include "../qr2/qr2regkeys.h"

#include "sb_crypt.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {	pi_cryptheader,
				pi_fixedheader, 
				pi_keylist,
				pi_uniquevaluelist,
				pi_servers,
				pi_finished } SBListParseState;


typedef enum {sl_lanbrowse, sl_disconnected, sl_connected, sl_mainlist} SBServerListState;


//master server query port
#define MSPORT2 28910

//number of master servers
#define NUM_MASTER_SERVERS 20

//max length of field list to master server
#define MAX_FIELD_LIST_LEN 256

//max number of values in a popular value list
#define MAX_POPULAR_VALUES 255

//max number of maps to track in a map loop
#define MAX_MAPLOOP_LENGTH 16

// max number of bytes that can be received from a single recvfrom call
// This must not be higher than 2048 for PS2 insock compatability
#define MAX_RECVFROM_SIZE 2048

//states for SBServer->state 
#define STATE_BASICKEYS			(1 << 0)
#define STATE_FULLKEYS			(1 << 1)
#define STATE_PENDINGBASICQUERY	(1 << 2)
#define STATE_PENDINGFULLQUERY	(1 << 3)
#define STATE_QUERYFAILED		(1 << 4)
#define STATE_PENDINGICMPQUERY	(1 << 5)
#define STATE_VALIDPING	        (1 << 6)
#define STATE_PENDINGQUERYCHALLENGE (1 << 7)

//how long before a server query times out
#define MAX_QUERY_MSEC 2500

//game server flags
#define UNSOLICITED_UDP_FLAG	1
#define PRIVATE_IP_FLAG			2
#define CONNECT_NEGOTIATE_FLAG	4
#define ICMP_IP_FLAG			8
#define NONSTANDARD_PORT_FLAG	16
#define NONSTANDARD_PRIVATE_PORT_FLAG	32
#define HAS_KEYS_FLAG					64
#define HAS_FULL_RULES_FLAG				128

//backend query flags (set in hbmaster, don't change)
#define QR2_USE_QUERY_CHALLENGE 128

//key types for the key type list
#define KEYTYPE_STRING	0
#define KEYTYPE_BYTE	1
#define KEYTYPE_SHORT	2

//how long to make the outgoing challenge
#define LIST_CHALLENGE_LEN 8

//protocol versions
#define LIST_PROTOCOL_VERSION 1
#define LIST_ENCODING_VERSION 3

//message types for outgoing requests
#define SERVER_LIST_REQUEST		0
#define SERVER_INFO_REQUEST		1
#define SEND_MESSAGE_REQUEST	2
#define KEEPALIVE_REPLY			3
#define MAPLOOP_REQUEST			4
#define PLAYERSEARCH_REQUEST	5

//message types for incoming requests
#define PUSH_KEYS_MESSAGE		1
#define PUSH_SERVER_MESSAGE		2
#define KEEPALIVE_MESSAGE		3
#define DELETE_SERVER_MESSAGE	4
#define MAPLOOP_MESSAGE			5
#define PLAYERSEARCH_MESSAGE	6

//server list update options
#define SEND_FIELDS_FOR_ALL		1
#define NO_SERVER_LIST			2
#define PUSH_UPDATES			4
#define SEND_GROUPS				32
#define NO_LIST_CACHE			64
#define LIMIT_RESULT_COUNT		128

//player search options
#define SEARCH_ALL_GAMES		1
#define SEARCH_LEFT_SUBSTRING	2
#define SEARCH_RIGHT_SUBSTRING	4
#define SEARCH_ANY_SUBSTRING	8

//max number of keys for the basic key list
#define MAX_QUERY_KEYS 20

//how long to search on the LAN
#define SL_LAN_SEARCH_TIME 2000

//MAGIC bytes for the QR2 queries
#define QR2_MAGIC_1 0xFE
#define QR2_MAGIC_2 0xFD

//magic bytes for nat negotiation message
#define NATNEG_MAGIC_LEN 6
#define NN_MAGIC_0 0xFD
#define NN_MAGIC_1 0xFC
#define NN_MAGIC_2 0x1E
#define NN_MAGIC_3 0x66
#define NN_MAGIC_4 0x6A
#define NN_MAGIC_5 0xB2


//query types
#define QTYPE_BASIC 0
#define QTYPE_FULL  1
#define QTYPE_ICMP  2

//query strings for old-style servers
#define BASIC_GOA_QUERY "\\basic\\\\info\\"
#define BASIC_GOA_QUERY_LEN 13
#define FULL_GOA_QUERY "\\status\\"
#define FULL_GOA_QUERY_LEN 8

//maximum length of a sortkey string
#define SORTKEY_LENGTH 255

//include ICMP support by default
#ifndef SB_NO_ICMP_SUPPORT
	#undef SB_ICMP_SUPPORT
	#define SB_ICMP_SUPPORT
#endif

//a key/value pair
typedef struct _SBKeyValuePair
{
	const char *key;
	const char *value;
} SBKeyValuePair;


//a ref-counted string
typedef struct _SBRefString
{
	const char *str;
#ifdef GSI_UNICODE
	const unsigned short *str_W;
#endif
	int refcount;
} SBRefString;


typedef struct _SBServerList *SBServerListPtr;
typedef struct _SBQueryEngine *SBQueryEnginePtr;

//callback types for server lists
typedef enum {slc_serveradded, slc_serverupdated, slc_serverdeleted, slc_initiallistcomplete, slc_disconnected, slc_queryerror, slc_publicipdetermined, slc_serverchallengereceived} SBListCallbackReason;
//callback types for query engine
typedef enum {qe_updatesuccess, qe_updatefailed, qe_engineidle, qe_challengereceived} SBQueryEngineCallbackReason;

//callback function prototypes
typedef void (*SBListCallBackFn)(SBServerListPtr serverlist, SBListCallbackReason reason, SBServer server, void *instance);
typedef void (*SBEngineCallbackFn)(SBQueryEnginePtr engine, SBQueryEngineCallbackReason reason, SBServer server, void *instance);
typedef void (*SBMaploopCallbackFn)(SBServerListPtr serverlist, SBServer server, time_t mapChangeTime, int numMaps, char *mapList[], void *instance);
typedef void (*SBPlayerSearchCallbackFn)(SBServerListPtr serverlist, char *nick, goa_uint32 serverIP, unsigned short serverPort, time_t lastSeenTime, char *gamename, void *instance);

//key information structure
typedef struct _KeyInfo
{
	const char *keyName;
	int keyType;
} KeyInfo;


typedef struct _SBServerList SBServerList;

#ifdef VENGINE_SUPPORT
	#define FTABLE_TYPES
	#include "../../VEngine/ve_gm3ftable.h"
#endif


//keeps track of previous and current sorting information
typedef struct _SortInfo
{
	gsi_char sortkey[SORTKEY_LENGTH];
	SBCompareMode comparemode;	
} SortInfo;

struct _SBServerList
{
	SBServerListState state;
	DArray servers;
	DArray keylist;
	char queryforgamename[36];
	char queryfromgamename[36];
	char queryfromkey[32];
	char mychallenge[LIST_CHALLENGE_LEN];
	char *inbuffer;
	int inbufferlen;
	const char *popularvalues[MAX_POPULAR_VALUES];
	int numpopularvalues;
	int expectedelements;
	
	SBListCallBackFn ListCallback;
	SBMaploopCallbackFn MaploopCallback;
	SBPlayerSearchCallbackFn PlayerSearchCallback;
	void *instance;

	SortInfo currsortinfo;
	SortInfo prevsortinfo;

	SBBool sortascending;
	goa_uint32 mypublicip;
	goa_uint32 srcip;
	unsigned short defaultport;

	char *lasterror;
#ifdef GSI_UNICODE
	unsigned short *lasterror_W;
#endif

	SOCKET slsocket;
	gsi_time lanstarttime;
	int fromgamever;
	GOACryptState cryptkey;
	int queryoptions;
	SBListParseState pstate;
	gsi_u16 backendgameflags;

	const char* mLanAdapterOverride;

	SBServer deadlist;
	
#ifdef VENGINE_SUPPORT
	#define FTABLE_IMPLEMENT
	#include "../../VEngine/ve_gm3ftable.h"
#endif

};


//server object

#ifndef SB_SERVER_DECLARED
#define SB_SERVER_DECLARED

struct _SBServer
{
	goa_uint32 publicip;
	unsigned short publicport;
	goa_uint32 privateip;
	unsigned short privateport;
	goa_uint32 icmpip;	
	unsigned char state;
	unsigned char flags;
	HashTable keyvals;
	gsi_time updatetime;
	gsi_u32 querychallenge;
	struct _SBServer *next;
	gsi_u8 splitResponseBitmap;
};

#endif

typedef struct _SBServerFIFO
{
	SBServer first;
	SBServer last;
	int count;
} SBServerFIFO;

typedef struct _SBQueryEngine
{
	int queryversion;
	int maxupdates;
	SBServerFIFO querylist;
	SBServerFIFO pendinglist;
	SOCKET querysock;
	#if !defined(SN_SYSTEMS)
	SOCKET icmpsock;
	#endif
	goa_uint32 mypublicip;
	unsigned char serverkeys[MAX_QUERY_KEYS];
	int numserverkeys;	
	SBEngineCallbackFn ListCallback;
	void *instance;
} SBQueryEngine;


struct _ServerBrowser
{
	SBQueryEngine engine;
	SBServerList list;
	SBBool disconnectFlag;
	SBBool dontUpdate;
	goa_uint32 triggerIP;
	unsigned short triggerPort;
	ServerBrowserCallback BrowserCallback;
	SBConnectToServerCallback ConnectCallback;
	void *instance;
};

#define SB_ICMP_ECHO 8
#define SB_ICMP_ECHO_REPLY	0

typedef struct _IPHeader {
	gsi_u8	ip_hl_ver;
	gsi_u8  ip_tos;
	gsi_i16   ip_len;
	gsi_u16 ip_id; 
	gsi_i16   ip_off;
	gsi_u8  ip_ttl;
	gsi_u8  ip_p;
	gsi_u16 ip_sum;
	struct  in_addr ip_src,ip_dst; 
} SBIPHeader;



typedef struct _ICMPHeader
{
	gsi_u8		type;
	gsi_u8		code;
	gsi_u16	cksum;
	union {
		struct {
			gsi_u16 id;
			gsi_u16 sequence;
		} echo;
		gsi_u32 idseq;
		gsi_u16 gateway;
		struct {
			gsi_u16 __notused;
			gsi_u16 mtu;
		} frag;
	} un;
} SBICMPHeader;


//server list functions
void SBServerListInit(SBServerList *slist, const char *queryForGamename, const char *queryFromGamename, const char *queryFromKey, int queryFromVersion, SBBool lanBrowse, SBListCallBackFn callback, void *instance);
SBError SBServerListConnectAndQuery(SBServerList *slist, const char *fieldList, const char *serverFilter, int options, int maxServers);
SBError SBServerListGetLANList(SBServerList *slist, unsigned short startport, unsigned short endport, int queryversion);
void SBServerListDisconnect(SBServerList *slist);
void SBServerListCleanup(SBServerList *slist); 
void SBServerListClear(SBServerList *slist);
SBError SBGetServerRulesFromMaster(SBServerList *slist, goa_uint32 ip, unsigned short port);
SBError SBSendMessageToServer(SBServerList *slist, goa_uint32 ip, unsigned short port, const char *data, int len);
SBError SBSendNatNegotiateCookieToServer(SBServerList *slist, goa_uint32 ip, unsigned short port, int cookie);
SBError SBSendMaploopRequest(SBServerList *slist, SBServer server, SBMaploopCallbackFn callback);
SBError SBSendPlayerSearchRequest(SBServerList *slist, char *searchName, int searchOptions, int maxResults, SBPlayerSearchCallbackFn callback);
int SBServerListFindServerByIP(SBServerList *slist, goa_uint32 ip, unsigned short port);
int SBServerListFindServer(SBServerList *slist, SBServer findserver);
void SBServerListRemoveAt(SBServerList *slist, int index);
void SBServerListAppendServer(SBServerList *slist, SBServer server);
void SBServerListSort(SBServerList *slist, SBBool ascending, SortInfo sortinfo);
int SBServerListCount(SBServerList *slist);
SBServer SBServerListNth(SBServerList *slist, int i);
SBError SBListThink(SBServerList *slist);
const char *SBLastListErrorA(SBServerList *slist);
const unsigned short *SBLastListErrorW(SBServerList *slist);
void SBSetLastListErrorPtr(SBServerList *slist, char* theError);
void SBFreeDeadList(SBServerList *slist);
void SBAllocateServerList(SBServerList *slist);

//sbserver functions
SBServer SBAllocServer(SBServerList *slist, goa_uint32 publicip, unsigned short publicport);
void SBServerFree(void *elem);
void SBServerAddKeyValue(SBServer server, const char *keyname, const char *value);
void SBServerAddIntKeyValue(SBServer server, const char *keyname, int value);
void SBServerParseKeyVals(SBServer server, char *keyvals);
void SBServerParseQR2FullKeysSingle(SBServer server, char *data, int len);
void SBServerParseQR2FullKeysSplit(SBServer server, char *data, int len);
void SBServerSetFlags(SBServer server, unsigned char flags);
void SBServerSetPublicAddr(SBServer server, goa_uint32 ip, unsigned short port);
void SBServerSetPrivateAddr(SBServer server, goa_uint32 ip, unsigned short port);
void SBServerSetICMPIP(SBServer server, goa_uint32 icmpip);
void SBServerSetState(SBServer server, unsigned char state);
void SBServerSetNext(SBServer server, SBServer next);
SBServer SBServerGetNext(SBServer server);
unsigned char SBServerGetState(SBServer server);
unsigned char SBServerGetFlags(SBServer server);
unsigned short SBServerGetPublicQueryPortNBO(SBServer server);
int SBIsNullServer(SBServer server);
extern SBServer SBNullServer;


//ref-str functions
const char *SBRefStr(SBServerList *slist,const char *str);
void SBReleaseStr(SBServerList *slist,const char *str);
HashTable SBRefStrHash(SBServerList *slist);
void SBRefStrHashCleanup(SBServerList *slist);

extern char *SBOverrideMasterServer;


//query engine functions
void SBQueryEngineInit(SBQueryEngine *engine, int maxupdates, int queryversion, SBBool lanBrowse, SBEngineCallbackFn callback, void *instance);
void SBQueryEngineUpdateServer(SBQueryEngine *engine, SBServer server, int addfront, int querytype, SBBool usequerychallenge);
void SBQueryEngineSetPublicIP(SBQueryEngine *engine, goa_uint32 mypublicip);
SBServer SBQueryEngineUpdateServerByIP(SBQueryEngine *engine, const char *ip, unsigned short queryport, int addfront, int querytype, SBBool usequerychallenge);
void SBQueryEngineThink(SBQueryEngine *engine);
void SBQueryEngineAddQueryKey(SBQueryEngine *engine, unsigned char keyid);
void SBEngineCleanup(SBQueryEngine *engine);
void SBQueryEngineRemoveServerFromFIFOs(SBQueryEngine *engine, SBServer server);
int NTSLengthSB(char *buf, int len);
void SBEngineHaltUpdates(SBQueryEngine *engine);


//server browser internal function
SBError ServerBrowserBeginUpdate2(ServerBrowser sb, SBBool async, SBBool disconnectOnComplete, const unsigned char *basicFields, int numBasicFields, const char *serverFilter, int updateOptions, int maxServers);


// Ascii versions of functions that should still be available in GSI_UNICODE mode
#ifdef GSI_UNICODE
const char *SBServerGetStringValueA(SBServer server, const char *keyname, const char *def); // for peer SDK
int SBServerGetIntValueA(SBServer server, const char *key, int idefault);
#endif


#ifdef __cplusplus
}
#endif

#endif
