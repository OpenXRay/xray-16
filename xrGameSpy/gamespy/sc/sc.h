///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef __SC_H__
#define __SC_H__


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "../common/gsCommon.h"
#include "../common/gsRC4.h"
#include "../common/gsAvailable.h"
#include "../ghttp/ghttp.h"
#include "../webservices/AuthService.h"


#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// optional to explicitly use __stdcall, __cdecl, __fastcall
#define SC_CALL


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Set this to define memory settings for the SDK
#define SC_STATIC_MEM

// The initial (or fixed, for static memory) report buffer size
#define SC_REPORT_BUFFER_BYTES 65536

// URL for sc services.
#define SC_SERVICE_MAX_URL_LEN 128
extern char scServiceURL[SC_SERVICE_MAX_URL_LEN];

// Session GUID size - must match backend
//#define SC_SESSION_GUID_SIZE 16
#define SC_AUTHDATA_SIZE 16
#define SC_SESSION_GUID_SIZE 40
#define SC_CONNECTION_GUID_SIZE 40

#define SC_GUID_BINARY_SIZE 16 // convert the 40 byte string guid into an int, 2 shorts and 8 bytes

// Limit to the number of teams
#define SC_MAX_NUM_TEAMS	64

// OPTIONS flags - first two bits reserved for authoritative / final flags
#define SC_OPTIONS_NONE					0


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Result codes
typedef enum
{
	SCResult_NO_ERROR = 0,
	SCResult_NO_AVAILABILITY_CHECK,
	SCResult_INVALID_PARAMETERS,
	SCResult_NOT_INITIALIZED,
	SCResult_CORE_NOT_INITIALIZED,
	SCResult_OUT_OF_MEMORY,
	SCResult_CALLBACK_PENDING,

	SCResult_HTTP_ERROR,
	SCResult_UNKNOWN_RESPONSE, // server reported an error which is unknown to us
	SCResult_RESPONSE_INVALID,

	SCResult_REPORT_INCOMPLETE,
	SCResult_REPORT_INVALID,
	SCResult_SUBMISSION_FAILED,

	SCResult_UNKNOWN_ERROR,

	SCResultMax
} SCResult;


// Game Results
typedef enum
{
	SCGameResult_WIN,
	SCGameResult_LOSS,
	SCGameResult_DRAW,
	SCGameResult_DISCONNECT,
	SCGameResult_DESYNC,
	SCGameResult_NONE,

	SCGameResultMax
} SCGameResult;


// Game Status
typedef enum
{
	SCGameStatus_COMPLETE,
	SCGameStatus_PARTIAL,
	SCGameStatus_BROKEN,

	SCGameStatusMax
} SCGameStatus;


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Data types
typedef void*   SCInterfacePtr;
typedef void*   SCReportPtr;


//typedef gsi_u32 SCTeamCount;
//typedef gsi_u32 SCTeamIndex;

//typedef gsi_u32 SCPlayerCount;
//typedef gsi_u32 SCPlayerIndex;

//typedef gsi_u16 SCKey;

typedef char    SCHiddenData[64];

//typedef enum 
//{
//	SCReportKeyType_SERVER,
//	SCReportKeyType_TEAM,
//	SCReportKeyType_PLAYER
//} SCReportKeyType;


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Callbacks
typedef void (*SCCreateSessionCallback)(const SCInterfacePtr theInterface,
										GHTTPResult          theHttpResult,
										SCResult             theResult,
										void *               theUserData);
typedef void (*SCSetReportIntentionCallback)(const SCInterfacePtr theInterface,
									         GHTTPResult          theHttpResult,
                                             SCResult             theResult,
											 void *	              theUserData);
typedef void (*SCSubmitReportCallback)(const SCInterfacePtr theInterface,
									   GHTTPResult          theHttpResult,
									   SCResult             theResult,
									   void *               theUserData);

/*
typedef void (*SCKeyCallback) (SCReportPtr theReport,
							   SCReportKeyType theKeyType,
							   gsi_u32 theIndex, // how many times this has been called
							   void* theUserParam);
*/

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Main interface functions
SCResult SC_CALL scInitialize(int theGameId,
							  SCInterfacePtr * theInterfaceOut);
SCResult SC_CALL scShutdown  (SCInterfacePtr theInterface);
SCResult SC_CALL scThink     (SCInterfacePtr theInterface);

SCResult SC_CALL scCreateSession(SCInterfacePtr             theInterface,
                                 const GSLoginCertificate * theCertificate,
								 const GSLoginPrivateData * thePrivateData,
								 SCCreateSessionCallback    theCallback,
								 gsi_time                   theTimeoutMs,
								 void *                     theUserData);

// This is a variation of scCreateSession that creates a "matchless" session.
// "matchless" means incoming data will be scrutinized less, and applied to stats immediately instead of when the match is over.
SCResult SC_CALL scCreateMatchlessSession(SCInterfacePtr    theInterface,
								 const GSLoginCertificate * theCertificate,
								 const GSLoginPrivateData * thePrivateData,
								 SCCreateSessionCallback    theCallback,
								 gsi_time                   theTimeoutMs,
								 void *                     theUserData);

SCResult SC_CALL scSetReportIntention(const SCInterfacePtr         theInterface,
									  const gsi_u8                 theConnectionId[SC_CONNECTION_GUID_SIZE],
									  gsi_bool                     isAuthoritative,
  							          const GSLoginCertificate *   theCertificate,
							          const GSLoginPrivateData *   thePrivateData,
							          SCSetReportIntentionCallback theCallback,
							          gsi_time                     theTimeoutMs,
									  void *                 theUserData);

SCResult SC_CALL scSubmitReport      (const SCInterfacePtr       theInterface,
									  const SCReportPtr          theReport,
									  gsi_bool                   isAuthoritative,
									  const GSLoginCertificate * theCertificate,
									  const GSLoginPrivateData * thePrivateData,
                                      SCSubmitReportCallback     theCallback,
                                      gsi_time                   theTimeoutMs,
									  void *                     theUserData);

//SCResult SC_CALL sc
SCResult SC_CALL scSetSessionId(const SCInterfacePtr theInterface, const gsi_u8 theSessionId[SC_SESSION_GUID_SIZE]);

const char * scGetSessionId   (const SCInterfacePtr theInterface);
const char * scGetConnectionId(const SCInterfacePtr theInterface);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Report generation functions

// Create a new (empty) report
//    - Specify player and team count so we can allocate memory
//      and later sanity check against reported data
SCResult SC_CALL scCreateReport(const SCInterfacePtr theInterface, 
								gsi_u32              theHeaderVersion, 
								gsi_u32              thePlayerCount,
								gsi_u32              theTeamCount,
								SCReportPtr *  theReportOut);

//    - Write global data key/values
SCResult SC_CALL scReportBeginGlobalData(SCReportPtr theReportData);
SCResult SC_CALL scReportBeginPlayerData(SCReportPtr theReportData);
SCResult SC_CALL scReportBeginTeamData  (SCReportPtr theReportData);


//    - Write player auth info and key/values
SCResult SC_CALL scReportBeginNewPlayer(SCReportPtr  theReportData);
SCResult SC_CALL scReportSetPlayerData (SCReportPtr  theReport,
									    gsi_u32            thePlayerIndex,
									    const gsi_u8       thePlayerConnectionId[SC_CONNECTION_GUID_SIZE],
									    gsi_u32            thePlayerTeamId,
									    SCGameResult       theResult,
									    gsi_u32            theProfileId,
									    const GSLoginCertificate * theCertificate,
									    const gsi_u8       theAuthData[16]);

//     - Write team info and key/values
SCResult SC_CALL scReportBeginNewTeam(SCReportPtr theReportData);
SCResult SC_CALL scReportSetTeamData (SCReportPtr theReport,
									  gsi_u32           theTeamId,
									  SCGameResult      theResult);

//     - Call this when you're finished writing the report
SCResult SC_CALL scReportEnd(SCReportPtr theReport, 
							 gsi_bool    isAuth, 
							 SCGameStatus theStatus);

// Call this to set the report as "matchless".
// This is needed if the report is being submitted to a "matchless" game session.
SCResult SC_CALL scReportSetAsMatchless(SCReportPtr theReport);


// Utility to record key value pairs
SCResult SC_CALL scReportAddIntValue(SCReportPtr theReportData,
									 gsi_u16           theKeyId,
									 gsi_i32           theValue);
SCResult SC_CALL scReportAddInt64Value(SCReportPtr theReportData,
									   gsi_u16           theKeyId,
									   gsi_i64           theValue);
SCResult SC_CALL scReportAddShortValue(SCReportPtr theReportData,
									 gsi_u16           theKeyId,
									 gsi_i16           theValue);
SCResult SC_CALL scReportAddByteValue(SCReportPtr theReportData,
									 gsi_u16           theKeyId,
									 gsi_i8            theValue);
SCResult SC_CALL scReportAddFloatValue(SCReportPtr theReportData,
									 gsi_u16           theKeyId,
									 float             theValue);
SCResult SC_CALL scReportAddStringValue(SCReportPtr theReportData,
										gsi_u16           theKeyId,
										const gsi_char *  theValue);
SCResult SC_CALL scDestroyReport(SCReportPtr theReport);

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Peer to peer encryption utilities (Will probably be moved to common code)

//     A symmetric cipher key for peer-to-peer communication
//     Will usually have one key for sending and a second key for receiving
typedef struct SCPeerCipher
{
	RC4Context mRC4;
	gsi_u8  mKey[GS_CRYPT_RSA_BYTE_SIZE];
	gsi_u32 mKeyLen;
	gsi_bool mInitialized;
} SCPeerCipher;

typedef char SCPeerKeyExchangeMsg[GS_CRYPT_RSA_BYTE_SIZE];

SCResult SC_CALL scPeerCipherInit(const GSLoginCertificate * theLocalCert, SCPeerCipher * theCipher);

SCResult SC_CALL scPeerCipherCreateKeyExchangeMsg(const GSLoginCertificate * theRemoteCert,
												  const SCPeerCipher *       theCipher, 
												  SCPeerKeyExchangeMsg       theMsgOut);

SCResult SC_CALL scPeerCipherParseKeyExchangeMsg (const GSLoginCertificate * theLocalCert,  
												  const GSLoginPrivateData * theCertPrivateData, 
												  const SCPeerKeyExchangeMsg theMsg, 
												  SCPeerCipher *             theCipherOut);

// Encrypt/Decrypt in place, also the RC4 context is modified everytime encryption/decryption take place
SCResult SC_CALL scPeerCipherEncryptBuffer(SCPeerCipher * theCipher, gsi_u8 * theData, gsi_u32 theLen);
SCResult SC_CALL scPeerCipherDecryptBuffer(SCPeerCipher * theCipher, gsi_u8 * theData, gsi_u32 theLen);

// When using UDP (non-ordered) you must supply a message num
//    - This is less efficient then ecrypting an ordered stream
SCResult SC_CALL scPeerCipherEncryptBufferIV(SCPeerCipher * theCipher, gsi_u32 theMessageNum, gsi_u8 * theData, gsi_u32 theLen);
SCResult SC_CALL scPeerCipherDecryptBufferIV(SCPeerCipher * theCipher, gsi_u32 theMessageNum, gsi_u8 * theData, gsi_u32 theLen);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#endif // __SC_H__
