///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef __AUTHSERVICE_H__
#define __AUTHSERVICE_H__


// ***** Authentication web services.
//
// ***** PUBLIC INTERFACE AT THE BOTTOM OF THE FILE

#include "../common/gsSoap.h"
#include "../common/gsCrypt.h"
#include "../common/gsLargeInt.h"

#if defined(__cplusplus)
extern "C"
{
#endif

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// URL for sc services.
#define WS_LOGIN_MAX_URL_LEN		  (128)
extern char wsAuthServiceURL[WS_LOGIN_MAX_URL_LEN];


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#define	WSLogin_PARTNERCODE_GAMESPY        0
#define	WSLogin_NAMESPACE_SHARED_NONUNIQUE 0
#define	WSLogin_NAMESPACE_SHARED_UNIQUE    1

typedef enum WSLoginValue
{
	// Login response code (mResponseCode)
	//   -- GameSpy Devs: Must match server
	WSLogin_Success = 0,
	WSLogin_ServerInitFailed,

	WSLogin_UserNotFound,
	WSLogin_InvalidPassword,
	WSLogin_InvalidProfile,
	WSLogin_UniqueNickExpired,

	WSLogin_DBError,
	WSLogin_ServerError,
	WSLogin_FailureMax, // must be the last failure

	// Login result (mLoginResult)
	WSLogin_HttpError = 100,    // ghttp reported an error, response ignored
	WSLogin_ParseError,         // couldn't parse http response
	WSLogin_InvalidCertificate, // login success but certificate was invalid!
	WSLogin_LoginFailed,        // failed login or other error condition
	WSLogin_OutOfMemory,        // could not process due to insufficient memory
	WSLogin_InvalidParameters,  // check the function arguments
	WSLogin_NoAvailabilityCheck,// No availability check was performed
	WSLogin_Cancelled,          // login request was cancelled
	WSLogin_UnknownError        // error occured, but detailed information not available

} WSLoginValue;


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#define WS_LOGIN_SIGKEY_LEN_BITS      (GS_CRYPT_RSA_BINARY_SIZE)
#define WS_LOGIN_PEERKEY_LEN_BITS     (GS_CRYPT_RSA_BINARY_SIZE)

#define WS_LOGIN_NICK_LEN             (30+1)
#define WS_LOGIN_EMAIL_LEN            (50+1)
#define WS_LOGIN_PASSWORD_LEN         (30+1)
#define WS_LOGIN_UNIQUENICK_LEN       (20+1)
#define WS_LOGIN_CDKEY_LEN            (64+1)
#define WS_LOGIN_PEERKEYMOD_LEN       (WS_LOGIN_PEERKEY_LEN_BITS/8)
#define WS_LOGIN_PEERKEYEXP_LEN       (WS_LOGIN_PEERKEY_LEN_BITS/8)
#define WS_LOGIN_PEERKEYPRV_LEN       (WS_LOGIN_PEERKEY_LEN_BITS/8)
#define WS_LOGIN_KEYHASH_LEN          (33) // 16 byte hash in hexstr +1 for NULL
#define WS_LOGIN_SIGNATURE_LEN        (WS_LOGIN_SIGKEY_LEN_BITS/8)
#define WS_LOGIN_SERVERDATA_LEN       (WS_LOGIN_PEERKEY_LEN_BITS/8)
#define WS_LOGIN_AUTHTOKEN_LEN        (256)
#define WS_LOGIN_PARTNERCHALLENGE_LEN (256)

// A user's login certificate, signed by the GameSpy AuthService
// The certificate is public and may be freely passed around
// Avoid use of pointer members so that structure may be easily copied
typedef struct GSLoginCertificate
{
	gsi_bool mIsValid;
	
	gsi_u32 mLength;
	gsi_u32 mVersion;
	gsi_u32 mPartnerCode; // aka Account space
	gsi_u32 mNamespaceId;
	gsi_u32 mUserId;
	gsi_u32 mProfileId;
	gsi_u32 mExpireTime;
	gsi_char mProfileNick[WS_LOGIN_NICK_LEN];
	gsi_char mUniqueNick[WS_LOGIN_UNIQUENICK_LEN];
	gsi_char mCdKeyHash[WS_LOGIN_KEYHASH_LEN];       // hexstr - bigendian
 	gsCryptRSAKey mPeerPublicKey;
	gsi_u8 mSignature[GS_CRYPT_RSA_BYTE_SIZE];   // binary - bigendian
	gsi_u8 mServerData[WS_LOGIN_SERVERDATA_LEN]; // binary - bigendian
} GSLoginCertificate;

// Private information for the owner of the certificate only
// -- careful! private key information must be kept secret --
typedef struct GSLoginCertificatePrivate
{
	gsCryptRSAKey mPeerPrivateKey;
	char mKeyHash[GS_CRYPT_MD5_HASHSIZE];
} GSLoginPrivateData;

//typedef char GSLoginCertificateKeyHash[GS_CRYPT_MD5_HASHSIZE]; // Hash of private key, for simple auth

typedef enum 
{
	wsLoginType_INVALID,
	wsLoginType_PROFILE,
	wsLoginType_UNIQUENICK,
	wsLoginType_GPTICKET,
	wsLoginType_REMOTEAUTH
} WSLoginType;

/*
typedef struct WSLoginProfileRequest
{
	int mPartnerCode;
	char mProfileName[WS_LOGIN_NICK_LEN];
	char mEmailAddress[WS_LOGIN_EMAIL_LEN];
	char mPassword[WS_LOGIN_PASSWORD_LEN];
	char mCdKeyHash[WS_LOGIN_KEYHASH_LEN];
	void * mUserData;
} WSLoginProfileRequest;

typedef struct WSLoginUniqueRequest
{
	int mPartnerCode;
	char mUniqueNick[WS_LOGIN_NICK_LEN];
	char mPassword[WS_LOGIN_PASSWORD_LEN];
	char mCdKeyHash[WS_LOGIN_KEYHASH_LEN];
	void * mUserData;
} WSLoginUniqueRequest;*/


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// CERTIFICATE login callback format 
typedef struct WSLoginResponse
{
	WSLoginValue mLoginResult;        // SDK high level result, e.g. LoginFailed
	WSLoginValue mResponseCode;       // server's result code,  e.g. BadPassword
	GSLoginCertificate mCertificate;  // Show this to others (prooves: "Bill is a valid user")
	GSLoginPrivateData mPrivateData;  // Keep this secret!   (prooves: "I am Bill")
	void * mUserData;
} WSLoginResponse;

typedef void (*WSLoginCallback)(GHTTPResult httpResult, WSLoginResponse * response, void * userData);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// PS3 login callback format 
typedef struct WSLoginPs3CertResponse
{
	WSLoginValue mLoginResult;   // SDK high level result, e.g. LoginFailed
	WSLoginValue mResponseCode;  // server's result code,  e.g. BadPassword
	char mRemoteAuthToken[WS_LOGIN_AUTHTOKEN_LEN];         // Show this to others
	char mPartnerChallenge[WS_LOGIN_PARTNERCHALLENGE_LEN]; // keep this secret! (It's a "password" for the token.)
	void * mUserData;
} WSLoginPs3CertResponse;

typedef void (*WSLoginPs3CertCallback)(GHTTPResult httpResult, WSLoginPs3CertResponse * response, void * userData);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Services to obtain a certificate
gsi_u32 wsLoginProfile(int partnerCode, int namespaceId, const gsi_char * profileNick, const gsi_char * email, const gsi_char * password, const gsi_char * cdkeyhash, WSLoginCallback callback, void * userData);
gsi_u32 wsLoginUnique (int partnerCode, int namespaceId, const gsi_char * uniqueNick, const gsi_char * password, const gsi_char * cdkeyhash, WSLoginCallback callback, void * userData);
gsi_u32 wsLoginRemoteAuth(int partnerCode, int namespaceId, const gsi_char authtoken[WS_LOGIN_AUTHTOKEN_LEN], const gsi_char partnerChallenge[WS_LOGIN_PARTNERCHALLENGE_LEN], WSLoginCallback callback, void * userData);

// Services to obtain a remote auth token
gsi_u32 wsLoginPs3Cert(int gameId, int partnerCode, int namespaceId, const gsi_u8 * ps3cert, int certLen, WSLoginPs3CertCallback callback, void * userData);

// Certificate Utilities, for use after obtaining a certificate
gsi_bool wsLoginCertIsValid    (const GSLoginCertificate * cert);
gsi_bool wsLoginCertWriteXML   (const GSLoginCertificate * cert, const char * anamespace, GSXmlStreamWriter writer);
gsi_bool wsLoginCertWriteBinary(const GSLoginCertificate * cert, char * bufout, unsigned int maxlen, unsigned int * lenout);
gsi_bool wsLoginCertReadBinary (GSLoginCertificate * certOut, char * bufin, unsigned int maxlen);
gsi_bool wsLoginCertReadXML    (GSLoginCertificate * cert, GSXmlStreamReader reader);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#if defined(__cplusplus)
} // extern "C"
#endif

#endif //__AUTHSERVICE_H__
