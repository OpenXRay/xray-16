///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "AuthService.h"
#include "../common/gsXML.h"
#include "../common/gsAvailable.h"
#include "../md5.h"

#pragma warning(disable: 4267)

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#define WS_AUTHSERVICE_LOGINPROFILE		 "LoginProfile"
#define WS_AUTHSERVICE_LOGINUNIQUE		 "LoginUniqueNick"
#define WS_AUTHSERVICE_LOGINREMOTEAUTH   "LoginRemoteAuth"
#define WS_AUTHSERVICE_LOGINPS3CERT		 "LoginPs3Cert"
#define WS_AUTHSERVICE_PROTOVERSION      1

#define WS_AUTHSERVICE_NAMESPACE         "ns1"
#define WS_AUTHSERVICE_LOGINPROFILE_SOAP "SOAPAction: \"http://gamespy.net/AuthService/LoginProfile\""
#define WS_AUTHSERVICE_LOGINUNIQUE_SOAP  "SOAPAction: \"http://gamespy.net/AuthService/LoginUniqueNick\""
#define WS_AUTHSERVICE_LOGINREMOTEAUTH_SOAP "SOAPAction: \"http://gamespy.net/AuthService/LoginRemoteAuth\""
#define WS_AUTHSERVICE_LOGINPS3CERT_SOAP "SOAPAction: \"http://gamespy.net/AuthService/LoginPs3Cert\""

#define WS_AUTHSERVICE_NAMESPACE_COUNT  1
const char * WS_AUTHSERVICE_NAMESPACES[WS_AUTHSERVICE_NAMESPACE_COUNT] =
{
	WS_AUTHSERVICE_NAMESPACE "=\"http://gamespy.net/AuthService/\""
};

const char WS_AUTHSERVICE_SIGNATURE_KEY[] = 
	"BF05D63E93751AD4A59A4A7389CF0BE8"
	"A22CCDEEA1E7F12C062D6E194472EFDA"
	"5184CCECEB4FBADF5EB1D7ABFE911814"
	"53972AA971F624AF9BA8F0F82E2869FB"
	"7D44BDE8D56EE50977898F3FEE758696"
	"22C4981F07506248BD3D092E8EA05C12"
	"B2FA37881176084C8F8B8756C4722CDC"
	"57D2AD28ACD3AD85934FB48D6B2D2027";

/*	"D589F4433FAB2855F85A4EB40E6311F0"
	"284C7882A72380938FD0C55CC1D65F7C"
	"6EE79EDEF06C1AE5EDE139BDBBAB4219"
	"B7D2A3F0AC3D3B23F59F580E0E89B9EC"
	"787F2DD5A49788C633D5D3CE79438934"
	"861EA68AE545D5BBCAAAD917CE9F5C7C"
	"7D1452D9214F989861A7511097C35E60"
	"A7273DECEA71CB5F8251653B26ACE781";*/



const char WS_AUTHSERVICE_SIGNATURE_EXP[] =
	"010001";

// This is declared as an extern so it can be overriden when testing
#define WS_LOGIN_SERVICE_URL_FORMAT   "https://%s.auth.pubsvs." GSI_DOMAIN_NAME "/AuthService/AuthService.asmx"
char wsAuthServiceURL[WS_LOGIN_MAX_URL_LEN] = "";

typedef struct WSIRequestData
{
	union 
	{
		WSLoginCallback mLoginCallback;
		WSLoginPs3CertCallback mLoginPs3CertCallback;
	} mUserCallback;
	//void * mUserCallback;
	void * mUserData;
} WSIRequestData;


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void wsiLoginEncryptPassword(const gsi_char * password, gsi_u8 ciphertext[GS_CRYPT_RSA_BYTE_SIZE]);
static gsi_bool wsiServiceAvailable();


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Checks to make sure the availability check has been performed prior
// to using any AuthService Login, and sets the service URL if it has
static gsi_bool wsiServiceAvailable()
{
	if (__GSIACResult == GSIACAvailable)
	{
		if (wsAuthServiceURL[0] == '\0')
			snprintf(wsAuthServiceURL, WS_LOGIN_MAX_URL_LEN, WS_LOGIN_SERVICE_URL_FORMAT, __GSIACGamename);
		return gsi_true;
	}
	else
		return gsi_false;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void wsiLoginProfileCallback(GHTTPResult theResult, 
								   GSXmlStreamWriter theRequestXml,
								   GSXmlStreamReader theResponseXml,
								   void * theRequestData)
{
	GHTTPResult translatedResult = GHTTPSuccess;
	WSIRequestData * requestData = (WSIRequestData*)theRequestData;

	WSLoginResponse response;
	GSLoginCertificate * cert = &response.mCertificate; // for convenience

	// initialize local variables
	cert->mIsValid = gsi_false;
	memset(&response, 0, sizeof(response));
	
	if (theResult == GHTTPSuccess)
	{
		// try to parse the soap
		if (gsi_is_false(gsXmlMoveToStart(theResponseXml)) ||
			gsi_is_false(gsXmlMoveToNext(theResponseXml, "LoginProfileResult")))
		{
			response.mLoginResult = WSLogin_ParseError;
		}
		else
		{
			// prepare response structure
			if (gsi_is_false(gsXmlReadChildAsInt(theResponseXml, "responseCode", (int*)&response.mResponseCode)))
			{
				// could not parse login response code
				response.mLoginResult = WSLogin_ParseError;
			}
			else if (response.mResponseCode != WSLogin_Success)
			{
				// server reported an error into reponseCode
				response.mLoginResult = WSLogin_ServerError;
			}
			else if (gsi_is_false(gsXmlMoveToChild(theResponseXml, "certificate")) ||
				gsi_is_false(wsLoginCertReadXML(cert, theResponseXml)) ||
				gsi_is_false(gsXmlMoveToParent(theResponseXml)) ||
				gsi_is_false(gsXmlReadChildAsLargeInt(theResponseXml, "peerkeyprivate", 
				                 &response.mPrivateData.mPeerPrivateKey.exponent))
				)
			{
				response.mLoginResult = WSLogin_ParseError;
			}
			else
			{
				MD5_CTX md5;

				// peer privatekey modulus is same as peer public key modulus
				memcpy(&response.mPrivateData.mPeerPrivateKey.modulus, &cert->mPeerPublicKey.modulus, sizeof(cert->mPeerPublicKey.modulus));

				// hash the private key
				MD5Init(&md5);
				//gsLargeIntAddToMD5(&response.mPrivateData.mPeerPrivateKey.modulus, &md5);
				gsLargeIntAddToMD5(&response.mPrivateData.mPeerPrivateKey.exponent, &md5);
				MD5Final((unsigned char*)response.mPrivateData.mKeyHash, &md5);

				// verify certificate
				cert->mIsValid = wsLoginCertIsValid(cert);
				if (gsi_is_false(cert->mIsValid))
				{
					response.mLoginResult = WSLogin_InvalidCertificate;
				}
			}
		}
	}
	else
	{
		response.mLoginResult = WSLogin_HttpError;
	}

	// trigger the user callback
	if (requestData->mUserCallback.mLoginCallback != NULL)
	{
		WSLoginCallback userCallback = (WSLoginCallback)(requestData->mUserCallback.mLoginCallback);
		(userCallback)(translatedResult, &response, requestData->mUserData);
	}
	gsifree(requestData);
	GSI_UNUSED(theRequestXml);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_u32 wsLoginProfile(int partnerCode,
					   int namespaceId,
					   const gsi_char * profileNick, 
					   const gsi_char * email, 
					   const gsi_char * password, 
					   const gsi_char * cdkey, 
					   WSLoginCallback callback, 
					   void * userData)
{
	GSXmlStreamWriter writer;
	WSIRequestData * requestData = NULL;
	gsi_u8 encryptedPassword[GS_CRYPT_RSA_BYTE_SIZE];

	if (!wsiServiceAvailable())
		return WSLogin_NoAvailabilityCheck;

	GS_ASSERT(partnerCode >= 0);
	GS_ASSERT(profileNick != NULL);
	GS_ASSERT(email       != NULL);
	GS_ASSERT(password    != NULL);

	if (_tcslen(profileNick) >= WS_LOGIN_NICK_LEN)
		return WSLogin_InvalidParameters;
	if (_tcslen(email) >= WS_LOGIN_EMAIL_LEN)
		return WSLogin_InvalidParameters;
	if (_tcslen(password) >= WS_LOGIN_PASSWORD_LEN)
		return WSLogin_InvalidParameters;
	if (cdkey != NULL && (_tcslen(cdkey) > WS_LOGIN_CDKEY_LEN))
		return WSLogin_InvalidParameters;

	// make a copy of the request callback and user param
	requestData = (WSIRequestData*)gsimalloc(sizeof(WSIRequestData));
	if (requestData == NULL)
		return WSLogin_OutOfMemory;
	requestData->mUserCallback.mLoginCallback = callback;
	requestData->mUserData     = userData;

	// encrypt the password (includes safety padding and hash)
	wsiLoginEncryptPassword(password, encryptedPassword);

	writer = gsXmlCreateStreamWriter(WS_AUTHSERVICE_NAMESPACES, WS_AUTHSERVICE_NAMESPACE_COUNT);
	if (writer != NULL)
	{
		GSSoapTask * aTask = NULL;

		if (gsi_is_false(gsXmlWriteOpenTag      (writer, WS_AUTHSERVICE_NAMESPACE, WS_AUTHSERVICE_LOGINPROFILE)) ||
			gsi_is_false(gsXmlWriteIntElement   (writer, WS_AUTHSERVICE_NAMESPACE, "version", WS_AUTHSERVICE_PROTOVERSION)) ||
			gsi_is_false(gsXmlWriteIntElement   (writer, WS_AUTHSERVICE_NAMESPACE, "partnercode", (gsi_u32)partnerCode)) ||
			gsi_is_false(gsXmlWriteIntElement   (writer, WS_AUTHSERVICE_NAMESPACE, "namespaceid", (gsi_u32)namespaceId)) ||
			gsi_is_false(gsXmlWriteAsciiStringElement(writer, WS_AUTHSERVICE_NAMESPACE, "email", email)) ||
			gsi_is_false(gsXmlWriteAsciiStringElement(writer, WS_AUTHSERVICE_NAMESPACE, "profilenick", profileNick)) ||
			gsi_is_false(gsXmlWriteOpenTag      (writer, WS_AUTHSERVICE_NAMESPACE, "password")) ||
			gsi_is_false(gsXmlWriteHexBinaryElement(writer, WS_AUTHSERVICE_NAMESPACE, "Value", encryptedPassword, GS_CRYPT_RSA_BYTE_SIZE)) ||
			gsi_is_false(gsXmlWriteCloseTag     (writer, WS_AUTHSERVICE_NAMESPACE, "password")) ||
			gsi_is_false(gsXmlWriteCloseTag     (writer, WS_AUTHSERVICE_NAMESPACE, WS_AUTHSERVICE_LOGINPROFILE)) ||
			gsi_is_false(gsXmlCloseWriter       (writer))
			)
		{
			gsXmlFreeWriter(writer);
			return WSLogin_OutOfMemory;
		}
		
		aTask = gsiExecuteSoap(wsAuthServiceURL, WS_AUTHSERVICE_LOGINPROFILE_SOAP,
			        writer, wsiLoginProfileCallback, (void*)requestData);
		if (aTask == NULL)
		{
			gsXmlFreeWriter(writer);
			gsifree(requestData);
			return WSLogin_OutOfMemory;
		}
	}
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void wsLoginUniqueCallback(GHTTPResult theResult, 
								  GSXmlStreamWriter theRequestXml,
								  GSXmlStreamReader theResponseXml,
								  void * theRequestData)
{
	GHTTPResult translatedResult = theResult;
	WSIRequestData * requestData = (WSIRequestData*)theRequestData;
	
	WSLoginResponse response;
	GSLoginCertificate * cert = &response.mCertificate; // for convenience
	cert->mIsValid = gsi_false;

	memset(&response, 0, sizeof(response));

	if (theResult == GHTTPSuccess)
	{
		// try to parse the soap
		if (gsi_is_false(gsXmlMoveToStart(theResponseXml)) ||
			gsi_is_false(gsXmlMoveToNext(theResponseXml, "LoginUniqueNickResult")))
		{
			response.mLoginResult = WSLogin_ParseError;
		}
		else
		{
			// prepare response structure
			if (gsi_is_false(gsXmlReadChildAsInt(theResponseXml, "responseCode", (int*)&response.mResponseCode)))
			{
				// could not parse login response code
				response.mLoginResult = WSLogin_ParseError;
			}
			else if (response.mResponseCode != WSLogin_Success)
			{
				// server reported an error into reponseCode
				response.mLoginResult = WSLogin_ServerError;
			}
			else if (gsi_is_false(gsXmlMoveToChild(theResponseXml, "certificate")) ||
				gsi_is_false(wsLoginCertReadXML(cert, theResponseXml)) ||
				gsi_is_false(gsXmlMoveToParent(theResponseXml)) ||
				gsi_is_false(gsXmlReadChildAsLargeInt(theResponseXml, "peerkeyprivate", 
				                 &response.mPrivateData.mPeerPrivateKey.exponent))
				)
			{
				response.mLoginResult = WSLogin_ParseError;
			}
			else
			{
				MD5_CTX md5;

				// peer privatekey modulus is same as peer public key modulus
				memcpy(&response.mPrivateData.mPeerPrivateKey.modulus, &cert->mPeerPublicKey.modulus, sizeof(cert->mPeerPublicKey.modulus));

				// hash the private key
				//   -- we use the has like a password for simple authentication
				MD5Init(&md5);
				//gsLargeIntAddToMD5(&response.mPrivateData.mPeerPrivateKey.modulus, &md5);
				gsLargeIntAddToMD5(&response.mPrivateData.mPeerPrivateKey.exponent, &md5);
				MD5Final((unsigned char*)response.mPrivateData.mKeyHash, &md5);

				// verify certificate
				cert->mIsValid = wsLoginCertIsValid(cert);
				if (gsi_is_false(cert->mIsValid))
				{
					response.mLoginResult = WSLogin_InvalidCertificate;
				}
			}
		}
	}
	else if (theResult == GHTTPRequestCancelled)
	{
		response.mLoginResult = WSLogin_Cancelled;
	}
	else
	{
		response.mLoginResult = WSLogin_HttpError;
	}

	// trigger the user callback
	if (requestData->mUserCallback.mLoginCallback != NULL)
	{
		WSLoginCallback userCallback = (WSLoginCallback)(requestData->mUserCallback.mLoginCallback);
		(userCallback)(translatedResult, &response, requestData->mUserData);
	}
	gsifree(requestData);
	GSI_UNUSED(theRequestXml);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//gsi_u32 wsLoginUnique(WSLoginUniqueRequest * request, WSLoginCallback callback)
gsi_u32 wsLoginUnique(int partnerCode,
					  int namespaceId,
					  const gsi_char * uniqueNick, 
					  const gsi_char * password, 
					  const gsi_char * cdkey, 
					  WSLoginCallback userCallback, 
					  void * userData)
{
	GSXmlStreamWriter writer;
	WSIRequestData * requestData = NULL;
	gsi_u8 encryptedPassword[GS_CRYPT_RSA_BYTE_SIZE];

	if (!wsiServiceAvailable())
		return WSLogin_NoAvailabilityCheck;

	GS_ASSERT(partnerCode >= 0);
	GS_ASSERT(uniqueNick != NULL);
	GS_ASSERT(password != NULL);

	if (_tcslen(uniqueNick) >= WS_LOGIN_UNIQUENICK_LEN)
		return WSLogin_InvalidParameters;
	if (_tcslen(password) >= WS_LOGIN_PASSWORD_LEN)
		return WSLogin_InvalidParameters;
	if (cdkey != NULL && (_tcslen(cdkey) >= WS_LOGIN_CDKEY_LEN))
		return WSLogin_InvalidParameters;

	// allocate the request values
	requestData = (WSIRequestData*)gsimalloc(sizeof(WSIRequestData));
	if (requestData == NULL)
		return WSLogin_OutOfMemory;
	requestData->mUserCallback.mLoginCallback = userCallback;
	requestData->mUserData     = userData;
	
	// encrypt the password (includes safety padding and hash)
	wsiLoginEncryptPassword(password, encryptedPassword);

	// create the xml request
	writer = gsXmlCreateStreamWriter(WS_AUTHSERVICE_NAMESPACES, WS_AUTHSERVICE_NAMESPACE_COUNT);
	if (writer != NULL)
	{
		GSSoapTask * aTask = NULL;

		if (gsi_is_false(gsXmlWriteOpenTag      (writer, WS_AUTHSERVICE_NAMESPACE, WS_AUTHSERVICE_LOGINUNIQUE)) ||
			gsi_is_false(gsXmlWriteIntElement   (writer, WS_AUTHSERVICE_NAMESPACE, "version", WS_AUTHSERVICE_PROTOVERSION)) ||
			gsi_is_false(gsXmlWriteIntElement   (writer, WS_AUTHSERVICE_NAMESPACE, "partnercode", (gsi_u32)partnerCode)) ||
			gsi_is_false(gsXmlWriteIntElement   (writer, WS_AUTHSERVICE_NAMESPACE, "namespaceid", (gsi_u32)namespaceId)) ||
			gsi_is_false(gsXmlWriteAsciiStringElement(writer, WS_AUTHSERVICE_NAMESPACE, "uniquenick", uniqueNick)) ||
			gsi_is_false(gsXmlWriteOpenTag      (writer, WS_AUTHSERVICE_NAMESPACE, "password")) ||
			gsi_is_false(gsXmlWriteHexBinaryElement(writer, WS_AUTHSERVICE_NAMESPACE, "Value", encryptedPassword, GS_CRYPT_RSA_BYTE_SIZE)) ||
			gsi_is_false(gsXmlWriteCloseTag     (writer, WS_AUTHSERVICE_NAMESPACE, "password")) ||
			gsi_is_false(gsXmlWriteCloseTag     (writer, WS_AUTHSERVICE_NAMESPACE, WS_AUTHSERVICE_LOGINUNIQUE)) ||
			gsi_is_false(gsXmlCloseWriter       (writer))
			)
		{
			gsXmlFreeWriter(writer);
			return WSLogin_OutOfMemory;
		}
		
		aTask = gsiExecuteSoap(wsAuthServiceURL, WS_AUTHSERVICE_LOGINUNIQUE_SOAP,
			        writer, wsLoginUniqueCallback, (void*)requestData);
		if (aTask == NULL)
		{
			gsXmlFreeWriter(writer);
			gsifree(requestData);
			return WSLogin_OutOfMemory;
		}
	}
	return 0;
}



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void wsLoginRemoteAuthCallback(GHTTPResult theResult, 
									  GSXmlStreamWriter theRequestXml,
									  GSXmlStreamReader theResponseXml,
									  void * theRequestData)
{
	GHTTPResult translatedResult = theResult;
	WSIRequestData * requestData = (WSIRequestData*)theRequestData;
	
	WSLoginResponse response;
	GSLoginCertificate * cert = &response.mCertificate; // for convenience
	cert->mIsValid = gsi_false;

	memset(&response, 0, sizeof(response));

	if (theResult == GHTTPSuccess)
	{
		// try to parse the soap
		if (gsi_is_false(gsXmlMoveToStart(theResponseXml)) ||
			gsi_is_false(gsXmlMoveToNext(theResponseXml, "LoginRemoteAuthResult")))
		{
			response.mLoginResult = WSLogin_ParseError;
		}
		else
		{
			// prepare response structure
			if (gsi_is_false(gsXmlReadChildAsInt(theResponseXml, "responseCode", (int*)&response.mResponseCode)))
			{
				// could not parse login response code
				response.mLoginResult = WSLogin_ParseError;
			}
			else if (response.mResponseCode != WSLogin_Success)
			{
				// server reported an error into reponseCode
				response.mLoginResult = WSLogin_ServerError;
			}
			else if (gsi_is_false(gsXmlMoveToChild(theResponseXml, "certificate")) ||
				gsi_is_false(wsLoginCertReadXML(cert, theResponseXml)) ||
				gsi_is_false(gsXmlMoveToParent(theResponseXml)) ||
				gsi_is_false(gsXmlReadChildAsLargeInt(theResponseXml, "peerkeyprivate", 
				                 &response.mPrivateData.mPeerPrivateKey.exponent))
				)
			{
				response.mLoginResult = WSLogin_ParseError;
			}
			else
			{
				MD5_CTX md5;

				// peer privatekey modulus is same as peer public key modulus
				memcpy(&response.mPrivateData.mPeerPrivateKey.modulus, &cert->mPeerPublicKey.modulus, sizeof(cert->mPeerPublicKey.modulus));

				// hash the private key
				//   -- we use the has like a password for simple authentication
				MD5Init(&md5);
				//gsLargeIntAddToMD5(&response.mPrivateData.mPeerPrivateKey.modulus, &md5);
				gsLargeIntAddToMD5(&response.mPrivateData.mPeerPrivateKey.exponent, &md5);
				MD5Final((unsigned char*)response.mPrivateData.mKeyHash, &md5);

				// verify certificate
				cert->mIsValid = wsLoginCertIsValid(cert);
				if (gsi_is_false(cert->mIsValid))
				{
					response.mLoginResult = WSLogin_InvalidCertificate;
				}
			}
		}
	}
	else if (theResult == GHTTPRequestCancelled)
	{
		response.mLoginResult = WSLogin_Cancelled;
	}
	else
	{
		response.mLoginResult = WSLogin_HttpError;
	}

	// trigger the user callback
	if (requestData->mUserCallback.mLoginCallback != NULL)
	{
		WSLoginCallback userCallback = (WSLoginCallback)(requestData->mUserCallback.mLoginCallback);
		(userCallback)(translatedResult, &response, requestData->mUserData);
	}
	gsifree(requestData);
	GSI_UNUSED(theRequestXml);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_u32 wsLoginRemoteAuth(int partnerCode, 
						  int namespaceId, 
						  const gsi_char authtoken[WS_LOGIN_AUTHTOKEN_LEN], 
						  const gsi_char partnerChallenge[WS_LOGIN_PARTNERCHALLENGE_LEN], 
						  WSLoginCallback userCallback, 
						  void * userData)
{
	GSXmlStreamWriter writer;
	WSIRequestData * requestData = NULL;
	//gsi_u8 encryptedChallenge[GS_CRYPT_RSA_BYTE_SIZE];

	if (!wsiServiceAvailable())
		return WSLogin_NoAvailabilityCheck;

	GS_ASSERT(partnerCode >= 0);

	// allocate the request values
	requestData = (WSIRequestData*)gsimalloc(sizeof(WSIRequestData));
	if (requestData == NULL)
		return WSLogin_OutOfMemory;
	requestData->mUserCallback.mLoginCallback = userCallback;
	requestData->mUserData     = userData;
	
	// encrypt the password (includes safety padding and hash)
	//wsiLoginEncryptPassword(partnerChallenge, encryptedChallenge);

	// create the xml request
	writer = gsXmlCreateStreamWriter(WS_AUTHSERVICE_NAMESPACES, WS_AUTHSERVICE_NAMESPACE_COUNT);
	if (writer != NULL)
	{
		GSSoapTask * aTask = NULL;

		if (gsi_is_false(gsXmlWriteOpenTag      (writer, WS_AUTHSERVICE_NAMESPACE, WS_AUTHSERVICE_LOGINREMOTEAUTH)) ||
			gsi_is_false(gsXmlWriteIntElement   (writer, WS_AUTHSERVICE_NAMESPACE, "version", WS_AUTHSERVICE_PROTOVERSION)) ||
			gsi_is_false(gsXmlWriteIntElement   (writer, WS_AUTHSERVICE_NAMESPACE, "partnercode", (gsi_u32)partnerCode)) ||
			gsi_is_false(gsXmlWriteIntElement   (writer, WS_AUTHSERVICE_NAMESPACE, "namespaceid", (gsi_u32)namespaceId)) ||
			gsi_is_false(gsXmlWriteTStringElement(writer, WS_AUTHSERVICE_NAMESPACE, "authtoken", authtoken)) ||
			gsi_is_false(gsXmlWriteTStringElement(writer, WS_AUTHSERVICE_NAMESPACE, "challenge", partnerChallenge)) ||
			//gsi_is_false(gsXmlWriteOpenTag      (writer, WS_AUTHSERVICE_NAMESPACE, "challenge")) ||
			//gsi_is_false(gsXmlWriteHexBinaryElement(writer, WS_AUTHSERVICE_NAMESPACE, "Value", encryptedChallenge, GS_CRYPT_RSA_BYTE_SIZE)) ||
			//gsi_is_false(gsXmlWriteCloseTag     (writer, WS_AUTHSERVICE_NAMESPACE, "challenge")) ||
			gsi_is_false(gsXmlWriteCloseTag     (writer, WS_AUTHSERVICE_NAMESPACE, WS_AUTHSERVICE_LOGINREMOTEAUTH)) ||
			gsi_is_false(gsXmlCloseWriter       (writer))
			)
		{
			gsXmlFreeWriter(writer);
			return WSLogin_OutOfMemory;
		}
		
		aTask = gsiExecuteSoap(wsAuthServiceURL, WS_AUTHSERVICE_LOGINREMOTEAUTH_SOAP,
			        writer, wsLoginRemoteAuthCallback, (void*)requestData);
		if (aTask == NULL)
		{
			gsXmlFreeWriter(writer);
			gsifree(requestData);
			return WSLogin_OutOfMemory;
		}
	}
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void wsLoginPs3CertCallback(GHTTPResult theResult, 
								  GSXmlStreamWriter theRequestXml,
								  GSXmlStreamReader theResponseXml,
								  void * theRequestData)
{

	GHTTPResult translatedResult = theResult;
	WSIRequestData * requestData = (WSIRequestData*)theRequestData;
	
	WSLoginPs3CertResponse response;
	memset(&response, 0, sizeof(response));

	if (theResult == GHTTPSuccess)
	{
		// try to parse the soap
		if (gsi_is_false(gsXmlMoveToStart(theResponseXml)) ||
			gsi_is_false(gsXmlMoveToNext(theResponseXml, "LoginPs3CertResult")))
		{
			response.mLoginResult = WSLogin_ParseError;
		}
		else
		{
			// prepare response structure
			if (gsi_is_false(gsXmlReadChildAsInt(theResponseXml, "responseCode", (int*)&response.mResponseCode)))
			{
				// could not parse login response code
				response.mLoginResult = WSLogin_ParseError;
			}
			else if (response.mResponseCode != WSLogin_Success)
			{
				// server reported an error into reponseCode
				response.mLoginResult = WSLogin_ServerError;
			}
			else 
			{
				const char * tokenStr = NULL;
				const char * challengeStr = NULL;
				int tokenLen = 0;
				int challengeLen = 0;

				// Check length of token+challenge, then read into memory
				if ( //gsi_is_false(gsXmlReadChildAsBase64Binary(theResponseXml, "authToken", NULL, &tokenLen)) ||
				     //gsi_is_false(gsXmlReadChildAsBase64Binary(theResponseXml, "partnerChallenge", NULL, &challengeLen)) ||
					 //(tokenLen > WS_LOGIN_AUTHTOKEN_LEN || challengeLen > WS_LOGIN_PARTNERCHALLENGE_LEN) ||
					gsi_is_false(gsXmlReadChildAsString(theResponseXml, "authToken", &tokenStr, &tokenLen)) ||
					gsi_is_false(gsXmlReadChildAsString(theResponseXml, "partnerChallenge", &challengeStr, &challengeLen)) ||
					(tokenLen >= WS_LOGIN_AUTHTOKEN_LEN || challengeLen >= WS_LOGIN_PARTNERCHALLENGE_LEN) )
				{
					response.mLoginResult = WSLogin_ParseError;
				}
				else
				{
					memcpy(response.mRemoteAuthToken, tokenStr, (gsi_u32)tokenLen);
					memcpy(response.mPartnerChallenge, challengeStr, (gsi_u32)challengeLen);
					response.mRemoteAuthToken[tokenLen] = '\0';
					response.mPartnerChallenge[challengeLen] = '\0';
				}
			}
		}
	}
	else if (theResult == GHTTPRequestCancelled)
	{
		response.mLoginResult = WSLogin_Cancelled;
	}
	else
	{
		response.mLoginResult = WSLogin_HttpError;
	}

	// trigger the user callback
	if (requestData->mUserCallback.mLoginPs3CertCallback != NULL)
	{
		WSLoginPs3CertCallback userCallback = (WSLoginPs3CertCallback)(requestData->mUserCallback.mLoginPs3CertCallback);
		(userCallback)(translatedResult, &response, requestData->mUserData);
	}
	gsifree(requestData);
	GSI_UNUSED(theRequestXml);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//gsi_u32 wsLoginUnique(WSLoginUniqueRequest * request, WSLoginCallback callback)
gsi_u32 wsLoginPs3Cert(int gameId,
					   int partnerCode,
					   int namespaceId,
					   const gsi_u8 * ps3cert,
					   int certLen,
					   WSLoginPs3CertCallback userCallback, 
					   void * userData)
{
	GSXmlStreamWriter writer;
	WSIRequestData * requestData = NULL;

	if (!wsiServiceAvailable())
		return WSLogin_NoAvailabilityCheck;

	GS_ASSERT(partnerCode >= 0);
	GS_ASSERT(ps3cert != NULL);
	
	
	// Version check, todo: use something better than length
	//if (certLen != 248)
	//	return WSLogin_InvalidParameters;

	// allocate the request values
	requestData = (WSIRequestData*)gsimalloc(sizeof(WSIRequestData));
	if (requestData == NULL)
		return WSLogin_OutOfMemory;

	requestData->mUserCallback.mLoginPs3CertCallback = userCallback;
	requestData->mUserData     = userData;
	
	// create the xml request
	writer = gsXmlCreateStreamWriter(WS_AUTHSERVICE_NAMESPACES, WS_AUTHSERVICE_NAMESPACE_COUNT);
	if (writer != NULL)
	{
		GSSoapTask * aTask = NULL;

		if (gsi_is_false(gsXmlWriteOpenTag      (writer, WS_AUTHSERVICE_NAMESPACE, WS_AUTHSERVICE_LOGINPS3CERT)) ||
			gsi_is_false(gsXmlWriteIntElement   (writer, WS_AUTHSERVICE_NAMESPACE, "version", WS_AUTHSERVICE_PROTOVERSION)) ||
			gsi_is_false(gsXmlWriteIntElement   (writer, WS_AUTHSERVICE_NAMESPACE, "gameid", (gsi_u32)gameId)) ||
			gsi_is_false(gsXmlWriteIntElement   (writer, WS_AUTHSERVICE_NAMESPACE, "partnercode", (gsi_u32)partnerCode)) ||
			gsi_is_false(gsXmlWriteIntElement   (writer, WS_AUTHSERVICE_NAMESPACE, "namespaceid", (gsi_u32)namespaceId)) ||
			gsi_is_false(gsXmlWriteOpenTag      (writer, WS_AUTHSERVICE_NAMESPACE, "npticket")) ||
			gsi_is_false(gsXmlWriteBase64BinaryElement(writer, WS_AUTHSERVICE_NAMESPACE, "Value", ps3cert, certLen)) ||
			gsi_is_false(gsXmlWriteCloseTag     (writer, WS_AUTHSERVICE_NAMESPACE, "npticket")) ||
			gsi_is_false(gsXmlWriteCloseTag     (writer, WS_AUTHSERVICE_NAMESPACE, WS_AUTHSERVICE_LOGINPS3CERT)) ||
			gsi_is_false(gsXmlCloseWriter       (writer))
			)
		{
			gsXmlFreeWriter(writer);
			return WSLogin_OutOfMemory;
		}
		
		aTask = gsiExecuteSoap(wsAuthServiceURL, WS_AUTHSERVICE_LOGINPS3CERT_SOAP,
			        writer, wsLoginPs3CertCallback, (void*)requestData);
		if (aTask == NULL)
		{
			gsXmlFreeWriter(writer);
			gsifree(requestData);
			return WSLogin_OutOfMemory;
		}
	}
	return 0;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static void wsiLoginEncryptPassword(const gsi_char * password, gsi_u8 ciphertext[GS_CRYPT_RSA_BYTE_SIZE])
{
	gsCryptRSAKey sigkeypub;


#ifdef GSI_UNICODE
	char password_A[WS_LOGIN_PASSWORD_LEN];
		// strip password into ascii to encrypt
	UCS2ToAsciiString(password, password_A);
#endif

	gsLargeIntSetFromHexString(&sigkeypub.modulus, WS_AUTHSERVICE_SIGNATURE_KEY);
	gsLargeIntSetFromHexString(&sigkeypub.exponent, WS_AUTHSERVICE_SIGNATURE_EXP);

#ifdef GSI_UNICODE
	gsCryptRSAEncryptBuffer(&sigkeypub, (const gsi_u8*)password_A, _tcslen(password), ciphertext);
#else
	gsCryptRSAEncryptBuffer(&sigkeypub, (const gsi_u8*)password, strlen(password), ciphertext);
#endif
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
static gsi_u32 wsiMakeLittleEndian32(gsi_u32 num)
{
#if defined(GSI_BIG_ENDIAN)
	num = gsiByteOrderSwap32(num);
#endif
	return num;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool wsLoginCertIsValid(const GSLoginCertificate * cert)
{
	// Verify the signature
	gsCryptRSAKey sigkeypub;
	MD5_CTX md5;
	gsi_u8 hash[16];
	gsi_i32 cryptResult = 0;
	gsi_u32 temp;

	// hash certificate data
	MD5Init(&md5);
	temp = wsiMakeLittleEndian32(cert->mLength);
	MD5Update(&md5, (unsigned char*)&temp, 4);
	temp = wsiMakeLittleEndian32(cert->mVersion);
	MD5Update(&md5, (unsigned char*)&temp, 4);
	temp = wsiMakeLittleEndian32(cert->mPartnerCode);
	MD5Update(&md5, (unsigned char*)&temp, 4);
	temp = wsiMakeLittleEndian32(cert->mNamespaceId);
	MD5Update(&md5, (unsigned char*)&temp, 4);
	temp = wsiMakeLittleEndian32(cert->mUserId);
	MD5Update(&md5, (unsigned char*)&temp, 4);
	temp = wsiMakeLittleEndian32(cert->mProfileId);
	MD5Update(&md5, (unsigned char*)&temp, 4);
	temp = wsiMakeLittleEndian32(cert->mExpireTime);
	MD5Update(&md5, (unsigned char*)&temp, 4);

#if defined(GSI_UNICODE)
	{
		char profile_A[WS_LOGIN_NICK_LEN];
		char uniquenick_A[WS_LOGIN_UNIQUENICK_LEN];
		char keyhash_A[WS_LOGIN_KEYHASH_LEN];

		UCS2ToAsciiString(cert->mProfileNick, profile_A);
		UCS2ToAsciiString(cert->mUniqueNick, uniquenick_A);
		UCS2ToAsciiString(cert->mCdKeyHash, keyhash_A);

		MD5Update(&md5, (unsigned char*)profile_A, strlen(profile_A));        //FIX for unicode
		MD5Update(&md5, (unsigned char*)uniquenick_A, strlen(uniquenick_A));  //FIX for unicode
		MD5Update(&md5, (unsigned char*)keyhash_A, strlen(keyhash_A));        //FIX for unicode
	}
#else
	MD5Update(&md5, (unsigned char*)&cert->mProfileNick, strlen(cert->mProfileNick)); 
	MD5Update(&md5, (unsigned char*)&cert->mUniqueNick, strlen(cert->mUniqueNick));   
	MD5Update(&md5, (unsigned char*)&cert->mCdKeyHash, strlen(cert->mCdKeyHash));     
#endif

	// must be hashed in big endian byte order
	//    skip leading zeroes
	gsLargeIntAddToMD5(&cert->mPeerPublicKey.modulus, &md5);
	gsLargeIntAddToMD5(&cert->mPeerPublicKey.exponent, &md5);

	MD5Update(&md5, (unsigned char*)&cert->mServerData, WS_LOGIN_SERVERDATA_LEN);
	MD5Final(hash, &md5);
				
	gsLargeIntSetFromHexString(&sigkeypub.modulus, WS_AUTHSERVICE_SIGNATURE_KEY);
	gsLargeIntSetFromHexString(&sigkeypub.exponent, WS_AUTHSERVICE_SIGNATURE_EXP);
	cryptResult = gsCryptRSAVerifySignedHash(&sigkeypub, hash, 16, cert->mSignature, WS_LOGIN_SIGNATURE_LEN);
	if (cryptResult == 0)
		return gsi_true;
	else
		return gsi_false;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Utility to write a certificate in binary form, does it really belong in this file?
#define WRITE_NETWORK_INT(a)  { \
                                  intNB = (gsi_i32)htonl(a); \
                                  if(lenoutSoFar + sizeof(intNB) > maxlen) \
                                      return gsi_false; \
                                  memcpy(bufout+lenoutSoFar, &intNB, sizeof(intNB)); \
								  lenoutSoFar += sizeof(intNB); }
#define WRITE_NTS(a)  { \
                                  if(lenoutSoFar + _tcslen(a) > maxlen) \
                                      return gsi_false; \
								  strcpy(bufout+lenoutSoFar, a); \
								  lenoutSoFar += _tcslen(a) + 1; }

#define WRITE_BINARY(a,l)  { \
                                  if(lenoutSoFar + l > maxlen) \
                                      return gsi_false; \
								  memcpy(bufout+lenoutSoFar, a, l); \
								  lenoutSoFar += l; }

#define WRITE_REV_BINARY(a,l)  { \
	                              int i=(gsi_i32)l; \
                                  const char * readPos = ((char*)a)+i-1; \
								  if (lenoutSoFar + l > maxlen) \
								      return gsi_false; \
                                  while(i > 0) \
                                  { \
								      *(bufout+lenoutSoFar) = *readPos; \
									  readPos--; lenoutSoFar++; i--; \
								  }; \
                               }

gsi_bool wsLoginCertWriteBinary(const GSLoginCertificate * cert, char * bufout, unsigned int maxlen, unsigned int * lenout)
{
	gsi_i32 intNB; // network byte order int
	gsi_i32 lenoutSoFar = 0; // tracks bytes written to bufout
	gsi_i32 lenTemp = 0; // for temp length of large ints


	WRITE_NETWORK_INT(cert->mLength);
	WRITE_NETWORK_INT(cert->mVersion);
	WRITE_NETWORK_INT(cert->mPartnerCode);
	WRITE_NETWORK_INT(cert->mNamespaceId);
	WRITE_NETWORK_INT(cert->mUserId);
	WRITE_NETWORK_INT(cert->mProfileId);
	WRITE_NETWORK_INT(cert->mExpireTime);

#ifndef GSI_UNICODE
	WRITE_NTS(cert->mProfileNick);
	WRITE_NTS(cert->mUniqueNick);
	WRITE_NTS(cert->mCdKeyHash);
#else
	if((lenoutSoFar + _tcslen(cert->mProfileNick) + _tcslen(cert->mUniqueNick) + _tcslen(cert->mCdKeyHash)) > maxlen)
		return gsi_false;
	// strip unicode to Ascii before writing this to the buffer
	lenoutSoFar += UCS2ToAsciiString(cert->mProfileNick, bufout+lenoutSoFar)+1;
	lenoutSoFar += UCS2ToAsciiString(cert->mUniqueNick, bufout+lenoutSoFar)+1;
	lenoutSoFar += UCS2ToAsciiString(cert->mCdKeyHash, bufout+lenoutSoFar)+1;
#endif

	lenTemp = (gsi_i32)gsLargeIntGetByteLength(&cert->mPeerPublicKey.modulus); // size in bytes, no leading zeroes
	WRITE_NETWORK_INT(lenTemp); 
	WRITE_REV_BINARY(cert->mPeerPublicKey.modulus.mData, (unsigned int)lenTemp);

	lenTemp = (gsi_i32)gsLargeIntGetByteLength(&cert->mPeerPublicKey.exponent); // size in bytes, no leading zeroes
	WRITE_NETWORK_INT(lenTemp);
	WRITE_REV_BINARY(cert->mPeerPublicKey.exponent.mData, (unsigned int)lenTemp);

	WRITE_NETWORK_INT(WS_LOGIN_SERVERDATA_LEN); 
	WRITE_BINARY(cert->mServerData, (unsigned int)WS_LOGIN_SERVERDATA_LEN); 

	WRITE_NETWORK_INT(GS_CRYPT_RSA_BYTE_SIZE); 
	WRITE_BINARY(cert->mSignature, (unsigned int)WS_LOGIN_SERVERDATA_LEN); 

	*lenout = (gsi_u32)lenoutSoFar;
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Utility to read the binary certificate

// each read macro first clears the buffer (a) before writing
#define READ_NTOH_INT(a)  { \
								  gsi_i32 temp; \
								  memset(&a, 0, sizeof(a)); \
								  memcpy(&temp, bufin, sizeof(gsi_i32)); \
								  intHB = (gsi_u32)ntohl(temp); \
                                  memcpy(&a, &intHB, sizeof(intHB)); \
								  if(lenoutSoFar + sizeof(intHB) > maxlen) \
                                      return gsi_false; \
								  bufin += sizeof(gsi_u32); \
								  lenoutSoFar += sizeof(intHB); }
#define READ_NTS(a,l)  { \
								  memset(&a, 0, sizeof(a)); \
								  _tcsncpy(a, bufin, l); \
								  if(lenoutSoFar + _tcslen(a)+1 > maxlen) \
                                      return gsi_false; \
								  bufin += _tcslen(a)+1; \
								  lenoutSoFar += _tcslen(a)+1; }

#define READ_ASCII(a,l)  { \
								  char temp[l]; \
								  memset(&a, 0, sizeof(a)); \
								  strncpy(temp, bufin, l); \
								  AsciiToUCS2String(temp, a); \
								  if(lenoutSoFar + _tcslen(a)+1 > maxlen) \
                                      return gsi_false; \
								  bufin += _tcslen(a)+1; \
								  lenoutSoFar += _tcslen(a)+1; }


#define READ_BINARY(a,l)  { \
								  int index = 0; \
								  memset(&a, 0, sizeof(a)); \
								  if(lenoutSoFar + l > maxlen) \
                                      return gsi_false; \
								  memcpy(a+index, bufin, l); \
								  lenoutSoFar += l; \
							 	  bufin += l; }

#define READ_REV_BINARY_TO_INT(a,l) { \
									  int i=(gsi_i32)l; \
									  int index = 0; \
									  char temp[4]; \
									  const char * readPos = bufin+i-1; \
									  memset(&a, 0, sizeof(a)); \
									  if (lenoutSoFar + l > maxlen) \
										  return gsi_false; \
									  while(i > 0) \
									  { \
										  temp[index%4] = *readPos; \
										  if (index%4 == 3) \
											 memcpy(a+(index/4), temp, 4); \
										  else if (index == (gsi_i32)l-1) \
										     memcpy(a+(index/4), temp, l); \
										  readPos--; index++; lenoutSoFar++; i--; \
									  }; \
									  bufin += l; }




gsi_bool wsLoginCertReadBinary(GSLoginCertificate * certOut, char * bufin, unsigned int maxlen)
{
	gsi_u32 intHB; // host byte order int
	gsi_u32 lenoutSoFar = 0; // tracks bufin index to make sure we dont exceed the maxlen
	gsi_u32 lenTemp = 0; // for temp length of large ints

	
	READ_NTOH_INT(certOut->mLength);
	READ_NTOH_INT(certOut->mVersion);
	READ_NTOH_INT(certOut->mPartnerCode);
	READ_NTOH_INT(certOut->mNamespaceId);
	READ_NTOH_INT(certOut->mUserId);
	READ_NTOH_INT(certOut->mProfileId);
	READ_NTOH_INT(certOut->mExpireTime);

#ifndef GSI_UNICODE

	READ_NTS(certOut->mProfileNick, WS_LOGIN_NICK_LEN);
	READ_NTS(certOut->mUniqueNick, WS_LOGIN_UNIQUENICK_LEN);
	READ_NTS(certOut->mCdKeyHash, WS_LOGIN_CDKEY_LEN);
#else

	// parses ascii to unicode before writing into the buffer
	READ_ASCII(certOut->mProfileNick, WS_LOGIN_NICK_LEN);
	READ_ASCII(certOut->mUniqueNick, WS_LOGIN_UNIQUENICK_LEN);
	READ_ASCII(certOut->mCdKeyHash, WS_LOGIN_CDKEY_LEN);
#endif

	READ_NTOH_INT(lenTemp); //size of the modulus data in bytes	
	// now calculate the length in int's
	certOut->mPeerPublicKey.modulus.mLength = (lenTemp / GS_LARGEINT_DIGIT_SIZE_BYTES);
	if (lenTemp % GS_LARGEINT_DIGIT_SIZE_BYTES != 0)
		certOut->mPeerPublicKey.modulus.mLength++;

	READ_REV_BINARY_TO_INT(certOut->mPeerPublicKey.modulus.mData, lenTemp);

	READ_NTOH_INT(lenTemp); //size of the exponent data in bytes
	// now calculate the length in int's
	certOut->mPeerPublicKey.exponent.mLength = (lenTemp / GS_LARGEINT_DIGIT_SIZE_BYTES);
	if (lenTemp % GS_LARGEINT_DIGIT_SIZE_BYTES != 0)
		certOut->mPeerPublicKey.exponent.mLength++;

	READ_REV_BINARY_TO_INT(certOut->mPeerPublicKey.exponent.mData, lenTemp);

	bufin += sizeof(gsi_u32); //skip the prepended length
	READ_BINARY(certOut->mServerData, (unsigned int)WS_LOGIN_SERVERDATA_LEN); 

	bufin += sizeof(gsi_u32); //skip the prepended length
	READ_BINARY(certOut->mSignature, (unsigned int)WS_LOGIN_SERVERDATA_LEN); 

	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool wsLoginCertWriteXML(const GSLoginCertificate * cert, const char * aNamespace, GSXmlStreamWriter writer)
{
	GS_ASSERT(cert != NULL);
	GS_ASSERT(writer != NULL);

	if (gsi_is_false(gsXmlWriteIntElement(writer, aNamespace, "length",      cert->mLength))       ||
		gsi_is_false(gsXmlWriteIntElement(writer, aNamespace, "version",     cert->mVersion))      ||
		gsi_is_false(gsXmlWriteIntElement(writer, aNamespace, "partnercode", cert->mPartnerCode))  ||
		gsi_is_false(gsXmlWriteIntElement(writer, aNamespace, "namespaceid", cert->mNamespaceId))  ||
		gsi_is_false(gsXmlWriteIntElement(writer, aNamespace, "userid",      cert->mUserId))       ||
		gsi_is_false(gsXmlWriteIntElement(writer, aNamespace, "profileid",   cert->mProfileId))    ||
		gsi_is_false(gsXmlWriteIntElement(writer, aNamespace, "expiretime",  cert->mExpireTime))   ||
		gsi_is_false(gsXmlWriteAsciiStringElement(writer, aNamespace, "profilenick", cert->mProfileNick))||
		gsi_is_false(gsXmlWriteAsciiStringElement(writer, aNamespace, "uniquenick",  cert->mUniqueNick)) ||
		gsi_is_false(gsXmlWriteAsciiStringElement(writer, aNamespace, "cdkeyhash",   cert->mCdKeyHash))  ||
		gsi_is_false(gsXmlWriteLargeIntElement(writer, aNamespace, "peerkeymodulus", &cert->mPeerPublicKey.modulus)) ||
 		gsi_is_false(gsXmlWriteLargeIntElement(writer, aNamespace, "peerkeyexponent", &cert->mPeerPublicKey.exponent)) ||	
		gsi_is_false(gsXmlWriteHexBinaryElement(writer, aNamespace, "serverdata", cert->mServerData, WS_LOGIN_SERVERDATA_LEN)) ||
		gsi_is_false(gsXmlWriteHexBinaryElement(writer, aNamespace, "signature", cert->mSignature, WS_LOGIN_SIGNATURE_LEN))
		)
	{
		//gsLargeIntReverseBytes(&cert->mPeerPublicKey.modulus);
		//gsLargeIntReverseBytes(&cert->mPeerPublicKey.exponent);
		return gsi_false;
	}
		
	//gsLargeIntReverseBytes(&cert->mPeerPublicKey.modulus);
	//gsLargeIntReverseBytes(&cert->mPeerPublicKey.exponent);
	return gsi_true;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
gsi_bool wsLoginCertReadXML(GSLoginCertificate * cert, GSXmlStreamReader reader)
{
	char hexstr[GS_CRYPT_RSA_BYTE_SIZE*2 +1]; // temp storage for key hex strings
	int hexlen;

	GS_ASSERT(cert != NULL);
	GS_ASSERT(reader != NULL);

	if (gsi_is_false(gsXmlReadChildAsInt      (reader, "length",     (int*)&cert->mLength))      ||
		gsi_is_false(gsXmlReadChildAsInt      (reader, "version",    (int*)&cert->mVersion))     ||
		gsi_is_false(gsXmlReadChildAsInt      (reader, "partnercode",(int*)&cert->mPartnerCode)) ||
		gsi_is_false(gsXmlReadChildAsInt      (reader, "namespaceid",(int*)&cert->mNamespaceId)) ||
		gsi_is_false(gsXmlReadChildAsInt      (reader, "userid",     (int*)&cert->mUserId))      ||
		gsi_is_false(gsXmlReadChildAsInt      (reader, "profileid",  (int*)&cert->mProfileId))   ||
		gsi_is_false(gsXmlReadChildAsInt      (reader, "expiretime", (int*)&cert->mExpireTime))  ||
		gsi_is_false(gsXmlReadChildAsTStringNT (reader, "profilenick", cert->mProfileNick, WS_LOGIN_NICK_LEN))       ||
		gsi_is_false(gsXmlReadChildAsTStringNT (reader, "uniquenick",  cert->mUniqueNick,  WS_LOGIN_UNIQUENICK_LEN)) ||
		gsi_is_false(gsXmlReadChildAsTStringNT (reader, "cdkeyhash",   cert->mCdKeyHash,   WS_LOGIN_KEYHASH_LEN))    ||

		gsi_is_false(gsXmlReadChildAsStringNT (reader, "peerkeymodulus", hexstr, GS_CRYPT_RSA_BYTE_SIZE*2 +1)) ||
		gsi_is_false(gsLargeIntSetFromHexString(&cert->mPeerPublicKey.modulus, hexstr)) ||

		gsi_is_false(gsXmlReadChildAsStringNT (reader, "peerkeyexponent", hexstr, GS_CRYPT_RSA_BYTE_SIZE*2 +1)) ||
		gsi_is_false(gsLargeIntSetFromHexString(&cert->mPeerPublicKey.exponent, hexstr)) ||

		gsi_is_false(gsXmlReadChildAsHexBinary(reader, "serverdata", cert->mServerData, WS_LOGIN_SERVERDATA_LEN, &hexlen)) ||

		gsi_is_false(gsXmlReadChildAsHexBinary(reader, "signature", cert->mSignature, WS_LOGIN_SIGNATURE_LEN, &hexlen))
		)
	{
		return gsi_false;
	}
	return gsi_true;
}

#pragma warning(default: 4267)