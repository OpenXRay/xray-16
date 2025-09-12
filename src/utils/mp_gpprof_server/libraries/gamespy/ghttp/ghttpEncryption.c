///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "ghttpCommon.h"
#if defined(MATRIXSSL)
#include "../matrixssl/matrixssl.h"
#endif


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
GHTTPBool ghttpSetRequestEncryptionEngine(GHTTPRequest request, GHTTPEncryptionEngine engine)
{
	GHIConnection * connection = ghiRequestToConnection(request);
	if(!connection)
		return GHTTPFalse;
	
	// Translate default into the actual engine name
	// We don't want to set the engine value to "default" because
	//   we'd lose the ability to determine the engine name in other places
	if (engine == GHTTPEncryptionEngine_Default)
	{
		#if defined(MATRIXSSL)
			engine = GHTTPEncryptionEngine_MatrixSsl;
		#elif defined(REVOEXSSL)
			engine = GHTTPEncryptionEngine_RevoEx;
		#else
			engine = GHTTPEncryptionEngine_GameSpy;
		#endif
	}

	// If the same engine has previously been set then we're done
	if (connection->encryptor.mEngine == engine)
		return GHTTPTrue; 

	// If a different engine has previously been set then we're screwed
	if (connection->encryptor.mInterface != NULL &&
		connection->encryptor.mEngine != engine)
	{
		return GHTTPFalse; 
	}

	// If the URL is HTTPS but the engine is specific as NONE then we can't connect
	if((engine == GHTTPEncryptionEngine_None) && (strncmp(connection->URL, "https://", 8) == 0))
		return GHTTPFalse;

	// Initialize the engine
	connection->encryptor.mEngine = engine;

	if (engine == GHTTPEncryptionEngine_None)
	{
		connection->encryptor.mInterface = NULL;
		return GHTTPTrue; // this is the default, just return
	}
	else
	{
		// 02OCT07 BED: Design was changed to only allow one engine at a time
		//              Assert that the specified engine is the one supported
		if (engine != GHTTPEncryptionEngine_Default)
		{
			#if defined(MATRIXSSL)
				GS_ASSERT(engine==GHTTPEncryptionEngine_MatrixSsl);
			#elif defined(REVOEXSSL)
				GS_ASSERT(engine==GHTTPEncryptionEngine_RevoEx);
			#else
				GS_ASSERT(engine==GHTTPEncryptionEngine_GameSpy);
			#endif
		}
		
		connection->encryptor.mInterface   = NULL;
		connection->encryptor.mInitFunc    = ghiEncryptorSslInitFunc;
		connection->encryptor.mStartFunc   = ghiEncryptorSslStartFunc;
		connection->encryptor.mCleanupFunc = ghiEncryptorSslCleanupFunc;
		connection->encryptor.mEncryptFunc = ghiEncryptorSslEncryptFunc;
		connection->encryptor.mDecryptFunc = ghiEncryptorSslDecryptFunc;
		connection->encryptor.mInitialized = GHTTPFalse;
		connection->encryptor.mSessionStarted = GHTTPFalse;
		connection->encryptor.mSessionEstablished = GHTTPFalse;
		connection->encryptor.mEncryptOnBuffer = GHTTPTrue;
		connection->encryptor.mEncryptOnSend   = GHTTPFalse;
		connection->encryptor.mLibSendsHandshakeMessages = GHTTPFalse;
		return GHTTPTrue;
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// *********************  MATRIXSSL ENCRYPTION ENGINE  ********************* //
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef MATRIXSSL

// SSL requires a certificate validator
static int ghiSslCertValidator(struct sslCertInfo* theCertInfo, void* theUserData)
{
	// Taken from matrisSslExample
	sslCertInfo_t	*next;
/*
	Make sure we are checking the last cert in the chain
*/
	next = theCertInfo;
	while (next->next != NULL) {
		next = next->next;
	}
	return next->verified;
}

// Init the engine
GHIEncryptionResult ghiEncryptorSslInitFunc(struct GHIConnection * connection,
											  struct GHIEncryptor  * theEncryptor)
{
	sslKeys_t *keys = NULL;
	sslSessionId_t *id = NULL;

	int ecodeResult;

	if (matrixSslOpen() < 0)
		return GHIEncryptionResult_Error;

	if (matrixSslReadKeys(&keys, NULL, NULL, NULL, NULL) < 0)
		return GHIEncryptionResult_Error;

	if (matrixSslNewSession((ssl_t**)&theEncryptor->mInterface, keys, id, 0) < 0)
		return GHIEncryptionResult_Error;

	matrixSslSetCertValidator((ssl_t*)theEncryptor->mInterface, ghiSslCertValidator, NULL);

	theEncryptor->mInitialized = GHTTPTrue;
	return GHIEncryptionResult_Success;
}

// Start the handshake process
GHIEncryptionResult ghiEncryptorSslInitFunc(struct GHIConnection * connection,
											  struct GHIEncryptor  * theEncryptor)
{
	sslBuf_t helloWrapper;
	
	// Prepare the hello message
	helloWrapper.buf   = connection->sendBuffer.data;
	helloWrapper.size  = connection->sendBuffer.size;
	helloWrapper.start = connection->sendBuffer.data + connection->sendBuffer.pos;
	helloWrapper.end   = helloWrapper.start; // start writing here
	
	ecodeResult = matrixSslEncodeClientHello((ssl_t*)theEncryptor->mInterface, &helloWrapper, 0); // 0 = cipher
	if (ecodeResult != 0) 
		return GHIEncryptionResult_Error; // error!

	// Adjust the sendBuffer to account for the new data
	connection->sendBuffer.len += (int)(helloWrapper.end - helloWrapper.start);
	connection->sendBuffer.encrypted = GHTTPTrue;
	theEncryptor->mSessionStarted = GHTTPTrue;
}

// Destroy the engine
GHIEncryptionResult ghiEncryptorSslCleanupFunc(struct GHIConnection * connection,
												 struct GHIEncryptor  * theEncryptor)
{
	matrixSslClose();
	return GHIEncryptionResult_Success;
}

// Encrypt some data
//    -  theEncryptedLength is reduced by the length of data written to theEncryptedBuffer
GHIEncryptionResult ghiEncryptorSslEncryptFunc(struct GHIConnection * connection,
												 struct GHIEncryptor  * theEncryptor,
												 const char * thePlainTextBuffer,
												 int          thePlainTextLength,
												 char *       theEncryptedBuffer,
												 int *        theEncryptedLength)
{
	int encodeResult = 0;

	// SSL buffer wrapper
	// Append to theDecryptedBuffer
	sslBuf_t encryptedBuf;
	encryptedBuf.buf   = theEncryptedBuffer;  // buf starts here
	encryptedBuf.start = theEncryptedBuffer;  // readpos,  set to start
	encryptedBuf.end   = theEncryptedBuffer;  // writepos, set to start
	encryptedBuf.size  = *theEncryptedLength; // total size of buf
	
	// perform the encryption
	encodeResult = matrixSslEncode(connection->encryptor.mInterface, 
		(unsigned char*)thePlainTextBuffer, *thePlainTextLength, &encryptedBuf);

	if (encodeResult == SSL_ERROR)
		return GHIEncryptionResult_Error;
	else if (encodeResult == SSL_FULL)
		return GHIEncryptionResult_BufferTooSmall;
	else
	{
		//*thePlainTextLength = *thePlainTextLength; // we always use the entire buffer
		*theEncryptedLength -= (int)(encryptedBuf.end - encryptedBuf.start);
		return GHIEncryptionResult_Success;
	}
}

// Decrypt some data
//    -  During the handshaking process, this may result in data being appended to the send buffer
//    -  Data may be left in the encrypted buffer
//    -  theEncryptedLength becomes the length of data read from theEncryptedBuffer
//    -  theDecryptedLength becomes the length of data written to theDecryptedBuffer
GHIEncryptionResult ghiEncryptorSslDecryptFunc(struct GHIConnection * connection,
												 struct GHIEncryptor  * theEncryptor,
												 const char * theEncryptedBuffer,
												 int *        theEncryptedLength,
												 char *       theDecryptedBuffer,
												 int *        theDecryptedLength)
{
	GHTTPBool decryptMore = GHTTPTrue;
	int decodeResult = 0;

	// SSL buffer wrappers
	sslBuf_t inBuf;
	sslBuf_t decryptedBuf;
	int encryptedStartSize = *theEncryptedLength;

	// Read from theEncryptedBuffer - Have to cast away the "const"
	inBuf.buf   = (unsigned char*)theEncryptedBuffer;  
	inBuf.start = (unsigned char*)theEncryptedBuffer;
	inBuf.end   = (unsigned char*)theEncryptedBuffer + *theEncryptedLength;
	inBuf.size  = *theEncryptedLength;

	// Append to theDecryptedBuffer
	decryptedBuf.buf   = theDecryptedBuffer;  // buf starts here
	decryptedBuf.start = theDecryptedBuffer;  // readpos,  set to start
	decryptedBuf.end   = theDecryptedBuffer;  // writepos, set to start
	decryptedBuf.size  = *theDecryptedLength; // total size of buf
	
	// Perform the decode operation
	//     - may require multiple tries
	while(decryptMore != GHTTPFalse && ((inBuf.end-inBuf.start) > 0))
	{
		unsigned char error = 0;
		unsigned char alertlevel = 0;
		unsigned char alertdescription = 0;

		// perform the decode, this will decode a single SSL message at a time
		decodeResult = matrixSslDecode(theEncryptor->mInterface, &inBuf, &decryptedBuf, 
										&error, &alertlevel, &alertdescription);
		switch(decodeResult)
		{
		case SSL_SUCCESS:          
			// a message was handled internally by matrixssl
			// No data is appeneded to the decrypted buffer
			if (matrixSslHandshakeIsComplete(theEncryptor->mInterface))
				theEncryptor->mSessionEstablished = GHTTPTrue;
			break;

		case SSL_PROCESS_DATA:
			// We've received app data, continue on.  
			// App data was appended to the decrypted buffer
			break;

		case SSL_SEND_RESPONSE:
			{
			// we must send an SSL response which has been written to decryptedBuf
			// transfer this response to the connection's sendBuffer
			int responseSize = decryptedBuf.end - decryptedBuf.start;

			// force disable-encryption
			//   this may seem like a hack, but it's the best way to avoid
			//   unnecessary data copies without modifying matrixSSL
			theEncryptor->mSessionEstablished = GHTTPFalse;
			ghiTrySendThenBuffer(connection, decryptedBuf.start, responseSize);
			theEncryptor->mSessionEstablished = GHTTPTrue;

			// Remove the bytes from the decrypted buffer (we don't want to return them to the app)
			decryptedBuf.end = decryptedBuf.start; // bug?
			break;
			}

		case SSL_ERROR:            
			// error decoding the data
			decryptMore = GHTTPFalse;
			break;

		case SSL_ALERT:            
			// server sent an alert
			if (alertdescription == SSL_ALERT_CLOSE_NOTIFY)
			decryptMore = GHTTPFalse;
			break;

		case SSL_PARTIAL:          
			// need to read more data from the socket(inbuf incomplete)
			decryptMore = GHTTPFalse;
			break;

		case SSL_FULL:             
			{
				// decodeBuffer is too small, need to increase size and try again
				decryptMore = GHTTPFalse;
				break;
			}
		};
	}

	// Store off the lengths
	*theEncryptedLength = encryptedStartSize - (inBuf.end - inBuf.start);
	*theDecryptedLength = decryptedBuf.end - decryptedBuf.start;

	// Return status to app
	if (decodeResult == SSL_FULL)
		return GHIEncryptionResult_BufferTooSmall;
	else if (decodeResult == SSL_ERROR || decodeResult == SSL_ALERT)
		return GHIEncryptionResult_Error;

	//if ((int)(decryptedBuf.end - decryptedBuf.start) > 0)
	//	printf ("Decrypted: %d bytes\r\n", *theDecryptedLength);
	return GHIEncryptionResult_Success;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// *********************  REVOEX SSL ENCRYPTION ENGINE  ******************** //
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#elif defined(REVOEXSSL)
#include <revolution/ssl.h>

typedef struct gsRevoExInterface
{
	SSLId mId;
	SSLClientCertId mClientCertId;
	SSLRootCAId mRootCAId;
	GHTTPBool mConnected; // means "connected to socket", not "connected to remote machine"
} gsRevoExInterface;

// Init the engine
GHIEncryptionResult ghiEncryptorSslInitFunc(struct GHIConnection * connection,
											  struct GHIEncryptor  * theEncryptor)
{
	int i=0;
	gsRevoExInterface* sslInterface = NULL;

	// There is only one place where this function should be called,
	//  and it should check if the engine has been initialized
	GS_ASSERT(theEncryptor->mInitialized == GHTTPFalse);
	GS_ASSERT(theEncryptor->mInterface == NULL);

	// allocate the interface (need one per connection)
	theEncryptor->mInterface = gsimalloc(sizeof(gsRevoExInterface));
	if (theEncryptor->mInterface == NULL)
	{
		// memory allocation failed
		gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Memory, GSIDebugLevel_WarmError,
			"Failed to allocate SSL interface (out of memory: %d bytes)\r\n", sizeof(gsRevoExInterface));
		return GHIEncryptionResult_Error;
	}
	memset(theEncryptor->mInterface, 0, sizeof(gsRevoExInterface));
	sslInterface = (gsRevoExInterface*)theEncryptor->mInterface;

	{
		int verifyOption = 0;
		int rcode = 0;
		
		verifyOption = SSL_VERIFY_COMMON_NAME
			| SSL_VERIFY_ROOT_CA
			| SSL_VERIFY_DATE
			| SSL_VERIFY_CHAIN
			| SSL_VERIFY_SUBJECT_ALT_NAME;

		// todo serverAddress, is this used for certificate name?
		sslInterface->mId = SSLNew(verifyOption, connection->serverAddress);

		rcode = SSLSetBuiltinRootCA(sslInterface->mId, SSL_ROOTCA_NINTENDO_1);
		if(rcode != SSL_ENONE){
			SSLShutdown(sslInterface->mId);
			return GHIEncryptionResult_Error;
		}

		rcode = SSLSetBuiltinClientCert(sslInterface->mId, SSL_CLIENTCERT_NINTENDO_0);
		if(rcode != SSL_ENONE){
			SSLShutdown(sslInterface->mId);
			return GHIEncryptionResult_Error;
		}
	}

	theEncryptor->mInitialized = GHTTPTrue;
	theEncryptor->mSessionStarted = GHTTPFalse;
	theEncryptor->mSessionEstablished = GHTTPFalse;
	//theEncryptor->mUseSSLConnect = GHTTPTrue;   
	theEncryptor->mEncryptOnBuffer = GHTTPFalse;
	theEncryptor->mEncryptOnSend = GHTTPTrue;
	theEncryptor->mLibSendsHandshakeMessages = GHTTPTrue;

	gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
		"GameSpy SSL (RevoEx engine) initialized\r\n");

	return GHIEncryptionResult_Success;
}

											  
// Destroy the engine
GHIEncryptionResult ghiEncryptorSslCleanupFunc(struct GHIConnection * connection,
												 struct GHIEncryptor  * theEncryptor)
{
	if (theEncryptor != NULL)
	{
		gsRevoExInterface* sslInterface = (gsRevoExInterface*)theEncryptor->mInterface;
		if (sslInterface != NULL)
		{
			SSLShutdown(sslInterface->mId);
			gsifree(sslInterface);
			theEncryptor->mInterface = NULL;
		}
		theEncryptor->mInitialized = GHTTPFalse;
		theEncryptor->mSessionStarted = GHTTPFalse;
		theEncryptor->mSessionEstablished = GHTTPFalse;
	}

	gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
		"GameSpy SSL (RevoEx) engine cleanup called\r\n");

	GSI_UNUSED(connection);

	return GHIEncryptionResult_Success;
}


GHIEncryptionResult ghiEncryptorSslStartFunc(struct GHIConnection * connection,
											struct GHIEncryptor  * theEncryptor)
{
	gsRevoExInterface* sslInterface = (gsRevoExInterface*)theEncryptor->mInterface;
	int result = 0;
	
	GS_ASSERT(theEncryptor->mSessionStarted == GHTTPFalse);
	
	// Call this only AFTER the socket has been connected to the remote server
	if (!sslInterface->mConnected)
	{
		result = SSLConnect(sslInterface->mId, connection->socket);
		if (result != SSL_ENONE)
		{
			switch(result)
			{
				case SSL_EFAILED:
					gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
						"GameSpy SSL (RevoEx) SSLConnect failed (SSL_EFAILED)\r\n");
					break;
				case SSL_ESSLID:
					gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
						"GameSpy SSL (RevoEx) SSLConnect failed (SSL_ESSLID)\r\n");
					break;
				default:
					gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
						"GameSpy SSL (RevoEx) SSLConnect failed (Unhandled Error)\r\n");
					break;
			}
			return GHIEncryptionResult_Error;
		}
		sslInterface->mConnected = GHTTPTrue;
	}

	GS_ASSERT(sslInterface->mConnected == GHTTPTrue);
		
	// begin securing the session
	result = SSLDoHandshake(sslInterface->mId);
	if (result != SSL_ENONE)
	{
		// Check for EWOULDBLOCK conditions
		if (result == SSL_EWANT_READ || result == SSL_EWANT_WRITE)
			return GHIEncryptionResult_Success;

		switch(result)
		{
			case SSL_EFAILED:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLDoHandshake failed (SSL_EFAILED)\r\n");
				break;
			case SSL_ESSLID:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLDoHandshake failed (SSL_ESSLID)\r\n");
				break;
			case SSL_ESYSCALL:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLDoHandshake failed (SSL_ESYSCALL)\r\n");
				break;
			case SSL_EZERO_RETURN:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLDoHandshake failed (SSL_EZERO_RETURN)\r\n");
				break;
			case SSL_EWANT_CONNECT:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLDoHandshake failed (SSL_EWANT_CONNECT)\r\n");
				break;
			case SSL_EVERIFY_COMMON_NAME:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLDoHandshake failed (SSL_EVERIFY_COMMON_NAME)\r\n");
				break;
			case SSL_EVERIFY_CHAIN:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLDoHandshake failed (SSL_EVERIFY_CHAIN)\r\n");
				break;
			case SSL_EVERIFY_ROOT_CA:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLDoHandshake failed (SSL_EVERIFY_ROOT_CA)\r\n");
				break;
			case SSL_EVERIFY_DATE:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLDoHandshake failed (SSL_EVERIFY_DATE)\r\n");
				break;																																				
			default:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLDoHandshake failed (Unhandled Error)\r\n");
				break;
		}
		return GHIEncryptionResult_Error;
	}

	// Success	
	theEncryptor->mSessionStarted = GHTTPTrue;
	theEncryptor->mSessionEstablished = GHTTPTrue;
	return GHIEncryptionResult_Success;
}

// Encrypt and send some data
GHIEncryptionResult ghiEncryptorSslEncryptSend(struct GHIConnection * connection,
												 struct GHIEncryptor  * theEncryptor,
												 const char * thePlainTextBuffer,
												 int          thePlainTextLength,
												 int *        theBytesSentOut)
{
	gsRevoExInterface* sslInterface = (gsRevoExInterface*)theEncryptor->mInterface;
	int result = 0;
	
	result = SSLWrite(sslInterface->mId, thePlainTextBuffer, thePlainTextLength);
	if (result == SSL_EZERO_RETURN)
	{
		// send 0 is not fatal
		*theBytesSentOut = 0;
	}
	else if (result == SSL_EWANT_WRITE)
	{
		// signal socket error, GetLastError will return EWOULDBLOCK or EINPROGRESS
		*theBytesSentOut = -1; 
	}
	else if (result < 0)
	{
		switch(result)
		{
			case SSL_EFAILED:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLWrite failed (SSL_EFAILED)\r\n");
				break;
			case SSL_ESSLID:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLWrite failed (SSL_ESSLID)\r\n");
				break;
			case SSL_EWANT_READ:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLWrite failed (SSL_EWANT_READ)\r\n");
				break;
			case SSL_ESYSCALL:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLWrite failed (SSL_ESYSCALL)\r\n");
				break;
			case SSL_EWANT_CONNECT:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLWrite failed (SSL_EWANT_CONNECT)\r\n");
				break;
			default:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLWrite failed (Unhandled Error)\r\n");
				break;
		}
		return GHIEncryptionResult_Error;
	}
	else
	{
		GS_ASSERT(result > 0);
		*theBytesSentOut = result;	
	}
	GSI_UNUSED(connection);
	return GHIEncryptionResult_Success;
}

// Receive and decrypt some data
GHIEncryptionResult ghiEncryptorSslDecryptRecv(struct GHIConnection * connection,
												 struct GHIEncryptor  * theEncryptor,
												 char * theDecryptedBuffer,
												 int *  theDecryptedLength)
{
	gsRevoExInterface* sslInterface = (gsRevoExInterface*)theEncryptor->mInterface;
	int result = 0;
	
	result = SSLRead(sslInterface->mId, theDecryptedBuffer, *theDecryptedLength);
	if (result == SSL_EZERO_RETURN)
	{
		// receive 0 is not fatal
		*theDecryptedLength = 0;
	}
	else if (result == SSL_EWANT_READ)
	{
		// signal socket error, GetLastError will return EWOULDBLOCK or EINPROGRESS
		*theDecryptedLength = -1; 
	}
	else if (result < 0)
	{
		// Fatal errors
		switch(result)
		{
			case SSL_EFAILED:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLRead failed (SSL_EFAILED)\r\n");
				break;
			case SSL_ESSLID:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLRead failed (SSL_ESSLID)\r\n");
				break;
			case SSL_EWANT_WRITE:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLRead failed (SSL_EWANT_WRITE)\r\n");
				break;
			case SSL_ESYSCALL:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLRead failed (SSL_ESYSCALL)\r\n");
				break;
			case SSL_EWANT_CONNECT:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLRead failed (SSL_EWANT_CONNECT)\r\n");
				break;
			default:
				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"GameSpy SSL (RevoEx) SSLRead failed (Unhandled Error)\r\n");
				break;
		}
		return GHIEncryptionResult_Error;
	}
	else
	{
		GS_ASSERT(result > 0);
		*theDecryptedLength = result;
	}
	GSI_UNUSED(connection);
	return GHIEncryptionResult_Success;
}

GHIEncryptionResult ghiEncryptorSslEncryptFunc(struct GHIConnection * connection,
												 struct GHIEncryptor  * theEncryptor,
												 const char * thePlainTextBuffer,
												 int          thePlainTextLength,
												 char *       theEncryptedBuffer,
												 int *        theEncryptedLength)
{
	GS_FAIL(); // Should never call this for RevoEx SSL!  It uses encrypt on send

	GSI_UNUSED(connection);
	GSI_UNUSED(theEncryptor);
	GSI_UNUSED(thePlainTextBuffer);
	GSI_UNUSED(thePlainTextLength);
	GSI_UNUSED(theEncryptedBuffer);
	GSI_UNUSED(theEncryptedLength);
	
	return GHIEncryptionResult_Error;
}

GHIEncryptionResult ghiEncryptorSslDecryptFunc(struct GHIConnection * connection,
												 struct GHIEncryptor  * theEncryptor,
												 const char * theEncryptedBuffer,
												 int *        theEncryptedLength,
												 char *       theDecryptedBuffer,
												 int *        theDecryptedLength)
{
	GS_FAIL(); // Should never call this for RevoEx SSL!  It uses decrypt on recv

	GSI_UNUSED(connection);
	GSI_UNUSED(theEncryptor);
	GSI_UNUSED(theEncryptedBuffer);
	GSI_UNUSED(theEncryptedLength);
	GSI_UNUSED(theDecryptedBuffer);
	GSI_UNUSED(theDecryptedLength);

	return GHIEncryptionResult_Error;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// ***********************  GS SSL ENCRYPTION ENGINE  ********************** //
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#else

#include "../common/gsSSL.h"
#include "../common/gsSHA1.h"
#include "../common/gsRC4.h"
#include "../md5.h"


// Processor for SSL state messages (transparent to application)
static GHIEncryptionResult ghiEncryptorProcessSSLHandshake(struct GHIConnection * connection,
													  struct GHIEncryptor * encryptor,
													  GHIBuffer * data);

// SSL-ASN.1 lengths are variable length NBO integers
//    we use this utility to make data packing easier
//    example: 61 little-endian(intel) = 61 00 00 00
//                big-endian(network)  = 00 00 00 61
static void ghiEncryptorWriteNBOLength(unsigned char* buf, int value, int size)
{
	int NBO = (int)htonl(value);
	unsigned char* NBOData = (unsigned char*)&NBO;

	assert(size <= sizeof(NBO));
	if (size > sizeof(NBO))
		return; // can't write more than 4 bytes!

	// this won't work if NBO ever changes from big-endian
	memcpy(buf, NBOData+(sizeof(int)-size), (size_t)size);
}
static GHTTPBool ghiEncryptorReadNBOLength(GHIBuffer * data, int* value, int size)
{
	assert(size <= sizeof(*value));
	if (size > sizeof(*value))
		return GHTTPFalse;
	if (GHTTPFalse == ghiReadDataFromBufferFixed(data, ((char*)value)+(sizeof(int)-size), size))
		return GHTTPFalse;

	*value = (int)htonl(*value);
	return GHTTPTrue;
}

static GHTTPBool ghiEncryptorParseASN1Sequence(GHIBuffer * data, int* lenOut)
{
	char tempChar = '\0';

	if (GHTTPFalse == ghiReadDataFromBufferFixed(data, &tempChar, 1))
		return GHTTPFalse;
	if (tempChar != 0x30) // sequence start byte
		return GHTTPFalse;

	if (GHTTPFalse == ghiReadDataFromBufferFixed(data, &tempChar, 1))
		return GHTTPFalse;
	if ((tempChar & 0x80) == 0x80)
	{
		int tempInt = 0;

		// length is stored in next (tempChar^0x80) bytes
		tempChar ^= 0x80;
		if (GHTTPFalse == ghiEncryptorReadNBOLength(data, &tempInt, tempChar))
			return GHTTPFalse;
		if (tempInt > (data->len - data->pos))
			return GHTTPFalse;

		*lenOut = tempInt;
		return GHTTPTrue;
	}
	else
	{
		if ((int)tempChar > (data->len - data->pos))
			return GHTTPFalse;

		*lenOut = tempChar;
		return GHTTPTrue;
	}
}

static void ghiEncryptorGenerateEncryptionKeys(gsSSL* sslInterface)
{
	// Use the server random, client random and pre master secret
	// to compute the encryption key.  
	
	// SSLv3 style
	//  master_secret = {
	//		MD5(pre_master_secret + SHA1("A"+pre_master_secret+client_random+server_random)) +
	//		MD5(pre_master_secret + SHA1("BB"+pre_master_secret+client_random+server_random)) +
	//		MD5(pre_master_secret + SHA1("CCC"+pre_master_secret+client_random+server_random))
	//  }
	//  key_block = {
	//		MD5(master_secret + SHA1("A"+master_secret+server_random+client_random)) +
	//		MD5(master_secret + SHA1("BB"+master_secret+server_random+client_random)) +
	//		MD5(master_secret + SHA1("CCC"+master_secret+server_random+client_random))

	SHA1Context sha1;
	MD5_CTX md5;
	unsigned char temp[SHA1HashSize];

	unsigned int randomSize = 32;
	unsigned char keyblock[64]; // todo: support different key sizes
	
	// master_secret "A"
	SHA1Reset(&sha1);
	SHA1Input(&sha1, (const unsigned char*)"A", 1);
	SHA1Input(&sha1, (const unsigned char*)sslInterface->premastersecret, GS_SSL_MASTERSECRET_LEN);
	SHA1Input(&sha1, (const unsigned char*)sslInterface->clientRandom, randomSize);
	SHA1Input(&sha1, (const unsigned char*)sslInterface->serverRandom, randomSize);
	SHA1Result(&sha1, temp);
	MD5Init(&md5);
	MD5Update(&md5, (unsigned char*)sslInterface->premastersecret, GS_SSL_MASTERSECRET_LEN);
	MD5Update(&md5, temp, GS_CRYPT_SHA1_HASHSIZE);
	MD5Final((unsigned char*)&sslInterface->mastersecret[0*GS_CRYPT_MD5_HASHSIZE], &md5);

	// master_secret "BB"
	SHA1Reset(&sha1);
	SHA1Input(&sha1, (const unsigned char*)"BB", 2);
	SHA1Input(&sha1, (const unsigned char*)sslInterface->premastersecret, GS_SSL_MASTERSECRET_LEN);
	SHA1Input(&sha1, (const unsigned char*)sslInterface->clientRandom, randomSize);
	SHA1Input(&sha1, (const unsigned char*)sslInterface->serverRandom, randomSize);
	SHA1Result(&sha1, temp);
	MD5Init(&md5);
	MD5Update(&md5, (unsigned char*)sslInterface->premastersecret, GS_SSL_MASTERSECRET_LEN);
	MD5Update(&md5, temp, GS_CRYPT_SHA1_HASHSIZE);
	MD5Final((unsigned char*)&sslInterface->mastersecret[1*GS_CRYPT_MD5_HASHSIZE], &md5);

	// master_secret "CCC"
	SHA1Reset(&sha1);
	SHA1Input(&sha1, (const unsigned char*)"CCC", 3);
	SHA1Input(&sha1, (const unsigned char*)sslInterface->premastersecret, GS_SSL_MASTERSECRET_LEN);
	SHA1Input(&sha1, (const unsigned char*)sslInterface->clientRandom, randomSize);
	SHA1Input(&sha1, (const unsigned char*)sslInterface->serverRandom, randomSize);
	SHA1Result(&sha1, temp);
	MD5Init(&md5);
	MD5Update(&md5, (unsigned char*)sslInterface->premastersecret, GS_SSL_MASTERSECRET_LEN);
	MD5Update(&md5, temp, GS_CRYPT_SHA1_HASHSIZE);
	MD5Final((unsigned char*)&sslInterface->mastersecret[2*GS_CRYPT_MD5_HASHSIZE], &md5);

	// key_block "A"
	SHA1Reset(&sha1);
	SHA1Input(&sha1, (const unsigned char*)"A", 1);
	SHA1Input(&sha1, (const unsigned char*)sslInterface->mastersecret, GS_SSL_MASTERSECRET_LEN);
	SHA1Input(&sha1, (const unsigned char*)sslInterface->serverRandom, randomSize);
	SHA1Input(&sha1, (const unsigned char*)sslInterface->clientRandom, randomSize);
	SHA1Result(&sha1, temp);
	MD5Init(&md5);
	MD5Update(&md5, (unsigned char*)sslInterface->mastersecret, GS_SSL_MASTERSECRET_LEN);
	MD5Update(&md5, temp, GS_CRYPT_SHA1_HASHSIZE);
	MD5Final(&keyblock[0*GS_CRYPT_MD5_HASHSIZE], &md5);

	// key_block "BB"
	SHA1Reset(&sha1);
	SHA1Input(&sha1, (const unsigned char*)"BB", 2);
	SHA1Input(&sha1, (const unsigned char*)sslInterface->mastersecret, GS_SSL_MASTERSECRET_LEN);
	SHA1Input(&sha1, (const unsigned char*)sslInterface->serverRandom, randomSize);
	SHA1Input(&sha1, (const unsigned char*)sslInterface->clientRandom, randomSize);
	SHA1Result(&sha1, temp);
	MD5Init(&md5);
	MD5Update(&md5, (unsigned char*)sslInterface->mastersecret, GS_SSL_MASTERSECRET_LEN);
	MD5Update(&md5, temp, GS_CRYPT_SHA1_HASHSIZE);
	MD5Final(&keyblock[1*GS_CRYPT_MD5_HASHSIZE], &md5);
	
	// key_block "CCC"
	SHA1Reset(&sha1);
	SHA1Input(&sha1, (const unsigned char*)"CCC", 3);
	SHA1Input(&sha1, (const unsigned char*)sslInterface->mastersecret, GS_SSL_MASTERSECRET_LEN);
	SHA1Input(&sha1, (const unsigned char*)sslInterface->serverRandom, randomSize);
	SHA1Input(&sha1, (const unsigned char*)sslInterface->clientRandom, randomSize);
	SHA1Result(&sha1, temp);
	MD5Init(&md5);
	MD5Update(&md5, (unsigned char*)sslInterface->mastersecret, GS_SSL_MASTERSECRET_LEN);
	MD5Update(&md5, temp, GS_CRYPT_SHA1_HASHSIZE);
	MD5Final(&keyblock[2*GS_CRYPT_MD5_HASHSIZE], &md5);

	// key_block "DDDD"
	SHA1Reset(&sha1);
	SHA1Input(&sha1, (const unsigned char*)"DDDD", 4);
	SHA1Input(&sha1, (const unsigned char*)sslInterface->mastersecret, GS_SSL_MASTERSECRET_LEN);
	SHA1Input(&sha1, (const unsigned char*)sslInterface->serverRandom, randomSize);
	SHA1Input(&sha1, (const unsigned char*)sslInterface->clientRandom, randomSize);
	SHA1Result(&sha1, temp);
	MD5Init(&md5);
	MD5Update(&md5, (unsigned char*)sslInterface->mastersecret, GS_SSL_MASTERSECRET_LEN);
	MD5Update(&md5, temp, GS_CRYPT_SHA1_HASHSIZE);
	MD5Final(&keyblock[3*GS_CRYPT_MD5_HASHSIZE], &md5);

	// key_block "EEEEE"
	// key_block "FFFFFF"
	// ... continue if more key material is needed

	// todo: support different key sizes
	// KEYBLOCK
	//    writemac[16], readmac[16], writekey[16], readkey[16], writeIV[0], readIV[0]
	memcpy(sslInterface->clientWriteMACSecret, &keyblock[0],  16);
	memcpy(sslInterface->clientReadMACSecret,  &keyblock[16], 16);
	memcpy(sslInterface->clientWriteKey,       &keyblock[32], 16);
	memcpy(sslInterface->clientReadKey,        &keyblock[48], 16);

	sslInterface->clientWriteMACLen = 16;
	sslInterface->clientReadMACLen = 16;
	sslInterface->clientWriteKeyLen = 16;
	sslInterface->clientReadKeyLen = 16;

	// Init the stream cipher
	RC4Init(&sslInterface->sendRC4, (const unsigned char*)sslInterface->clientWriteKey, sslInterface->clientWriteKeyLen);
	RC4Init(&sslInterface->recvRC4, (const unsigned char*)sslInterface->clientReadKey, sslInterface->clientReadKeyLen);

	gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
		"Generated SSL session keys\r\n");
}


// Init the engine
GHIEncryptionResult ghiEncryptorSslInitFunc(struct GHIConnection * connection,
											  struct GHIEncryptor  * theEncryptor)
{
	gsSSL* sslInterface = NULL;

	// There is only one place where this function should be called,
	//  and it should check if the engine has been initialized
	assert(theEncryptor->mInitialized == GHTTPFalse);
	assert(theEncryptor->mInterface == NULL);

	// Make sure the send buffer is large enough for the SSL handshake (handshake is <1k)
	if (connection->sendBuffer.size - connection->sendBuffer.len < sizeof(gsSSLClientHelloMsg))
		return GHIEncryptionResult_BufferTooSmall;

	// allocate the interface (need one per connection)
	theEncryptor->mInterface = gsimalloc(sizeof(gsSSL));
	if (theEncryptor->mInterface == NULL)
	{
		// memory allocation failed
		gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Memory, GSIDebugLevel_WarmError,
			"Failed to allocate SSL interface (out of memory: %d bytes)\r\n", sizeof(gsSSL));
		return GHIEncryptionResult_Error;
	}
	memset(theEncryptor->mInterface, 0, sizeof(gsSSL));
	sslInterface = (gsSSL*)theEncryptor->mInterface;

	theEncryptor->mInitialized = GHTTPTrue;
	theEncryptor->mSessionStarted = GHTTPFalse;
	theEncryptor->mSessionEstablished = GHTTPFalse;
	theEncryptor->mEncryptOnBuffer = GHTTPTrue;
	theEncryptor->mEncryptOnSend = GHTTPFalse;
	theEncryptor->mLibSendsHandshakeMessages = GHTTPFalse;
	MD5Init(&sslInterface->finishHashMD5);
	SHA1Reset(&sslInterface->finishHashSHA1);

	gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
		"GameSpy SSL engine initialized\r\n");

	return GHIEncryptionResult_Success;
}

// Destroy the engine
GHIEncryptionResult ghiEncryptorSslCleanupFunc(struct GHIConnection * connection,
												 struct GHIEncryptor  * theEncryptor)
{
	if (theEncryptor != NULL)
	{
		gsSSL* sslInterface = (gsSSL*)theEncryptor->mInterface;
		if (sslInterface != NULL)
		{
			gsifree(sslInterface);
			theEncryptor->mInterface = NULL;
		}
		theEncryptor->mInitialized = GHTTPFalse;
		theEncryptor->mSessionStarted = GHTTPFalse;
		theEncryptor->mSessionEstablished = GHTTPFalse;
	}

	gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
		"GameSpy SSL engine cleanup called\r\n");

	GSI_UNUSED(connection);

	return GHIEncryptionResult_Success;
}

// Init the engine
GHIEncryptionResult ghiEncryptorSslStartFunc(struct GHIConnection * connection,
											  struct GHIEncryptor  * theEncryptor)
{
	gsSSL* sslInterface = (gsSSL*)theEncryptor->mInterface;
	gsSSLClientHelloMsg helloMsg;
	int i=0;
	
	// prepare the client hello
	//    1) 
	helloMsg.header.contentType  = GS_SSL_CONTENT_HANDSHAKE;
	helloMsg.header.versionMajor = GS_SSL_VERSION_MAJOR;
	helloMsg.header.versionMinor = GS_SSL_VERSION_MINOR;

	// Set the length of the client hello message (2-byte NBO int, not including record header)
	ghiEncryptorWriteNBOLength(helloMsg.header.lengthNBO, sizeof(gsSSLClientHelloMsg)-sizeof(gsSSLRecordHeaderMsg), 2);

	helloMsg.handshakeType = GS_SSL_HANDSHAKE_CLIENTHELLO;
	helloMsg.versionMajor  = GS_SSL_VERSION_MAJOR;
	helloMsg.versionMinor  = GS_SSL_VERSION_MINOR;

	// Set the length of the client hello data (3 byte NBO int)
	//    This is the total message length MINUS the SSL record header MINUS four additional header bytes 
	ghiEncryptorWriteNBOLength(helloMsg.lengthNBO, sizeof(gsSSLClientHelloMsg)-sizeof(gsSSLRecordHeaderMsg)-4, 3);
	//ghttpEncryptorSetNBOBytesFromHBOInt(helloMsg.time, (gsi_u32)current_time(), 4); // 4 byte time value (for randomness)
	ghiEncryptorWriteNBOLength(helloMsg.time, 0, 4); // test code: no randomness
	
	// fill in the [rest of the] random
	//   Security Note: If a hacker is able to discern the current_time() they may be able to
	//   recreate the random bytes and recover the session key.
	Util_RandSeed(current_time());
	for (i=0; i<28; i++)
	{
		#if defined(GS_CRYPT_NO_RANDOM)
			#pragma message ("!!!WARNING: SSL Random disable for debug purposes.  SSL not secured!!!")
			helloMsg.random[i] = 0; // test code: no randomness
		#else
			helloMsg.random[i] = (unsigned char)Util_RandInt(0, 0xff);
		#endif
	}

	// store a copy of the random (used later for key generation)
	memcpy(&sslInterface->clientRandom[0], helloMsg.time, 4);
	memcpy(&sslInterface->clientRandom[4], helloMsg.random, 28);

	// todo: session resumption
	helloMsg.sessionIdLen = 0;

	// fill in cipher suite IDs
	helloMsg.cipherSuitesLength = htons(sizeof(gsi_u16)*GS_SSL_NUM_CIPHER_SUITES);
	for (i=0; i < GS_SSL_NUM_CIPHER_SUITES; i++)
		helloMsg.cipherSuites[i] = htons((unsigned short)gsSSLCipherSuites[i].mSuiteID);

	// there are no standard SSL compression methods
	helloMsg.compressionMethodLen = 1;
	helloMsg.compressionMethodList = 0;

	// We need to compute a hash of all the handshake messages
	//    Add this message to the hash (both MD5 hash and SHA1 hash)
	MD5Update(&sslInterface->finishHashMD5, (unsigned char*)&helloMsg+sizeof(gsSSLRecordHeaderMsg), sizeof(gsSSLClientHelloMsg)-sizeof(gsSSLRecordHeaderMsg));
	SHA1Input(&sslInterface->finishHashSHA1, (unsigned char*)&helloMsg+sizeof(gsSSLRecordHeaderMsg), sizeof(gsSSLClientHelloMsg)-sizeof(gsSSLRecordHeaderMsg));

	// Now send it (we already verified the length, so this should not fail)
	if (GHTTPFalse == ghiAppendDataToBuffer(&connection->sendBuffer, (const char*)&helloMsg, sizeof(gsSSLClientHelloMsg)))
	{
		// assert or just return?
		return GHIEncryptionResult_BufferTooSmall;
	}
	
	gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
		"GameSpy SSL engine handshake started\r\n");

	theEncryptor->mSessionStarted = GHTTPTrue;
	return GHIEncryptionResult_Success;

}

// Encrypt some data
//    -  theEncryptedLength is reduced by the length of data written to theEncryptedBuffer
// So if the encrypted buffer is 255 bytes long and we write 50 additional bytes, we'll return 205.
GHIEncryptionResult ghiEncryptorSslEncryptFunc(struct GHIConnection * connection,
												 struct GHIEncryptor  * theEncryptor,
												 const char * thePlainTextBuffer,
												 int          thePlainTextLength,
												 char *       theEncryptedBuffer,
												 int *        theEncryptedLength)
{
	if (theEncryptor != NULL)
	{
		gsSSL* sslInterface = (gsSSL*)theEncryptor->mInterface;
		if (sslInterface == NULL || theEncryptor->mSessionEstablished == GHTTPFalse)
		{
			// not secured yet, send as plain text
			if (thePlainTextLength > *theEncryptedLength)
				return GHIEncryptionResult_BufferTooSmall;
			memcpy(theEncryptedBuffer, thePlainTextBuffer, (size_t)thePlainTextLength);
			*theEncryptedLength += thePlainTextLength; // number of bytes written
		}
		else
		{
			// Create an SSL encrypted record
			//    The order of operations below is very important.
			//    The MAC must be computed before ciphering the plain text because
			//    theEncryptedBuffer may be the same memory location as thePlainTextBuffer

			gsSSL* sslInterface = (gsSSL*)theEncryptor->mInterface;
			gsSSLRecordHeaderMsg* header = NULL;
			MD5_CTX md5;
			int pos = 0;
			unsigned short lengthNBO = htons((unsigned short)thePlainTextLength);
			unsigned char MAC[GS_CRYPT_MD5_HASHSIZE];

			// The SSL record adds a little overhead
			if (*theEncryptedLength < (thePlainTextLength+(int)sizeof(gsSSLRecordHeaderMsg)))
				return GHIEncryptionResult_BufferTooSmall;

			// write the SSL header
			header = (gsSSLRecordHeaderMsg*)theEncryptedBuffer;
			header->contentType = GS_SSL_CONTENT_APPLICATIONDATA;
			header->versionMajor = GS_SSL_VERSION_MAJOR;
			header->versionMinor = GS_SSL_VERSION_MINOR;
			pos += sizeof(gsSSLRecordHeaderMsg);

			// calculate the MAC
			MD5Init(&md5);
			MD5Update(&md5, sslInterface->clientWriteMACSecret, (unsigned int)sslInterface->clientWriteMACLen);
			MD5Update(&md5, (unsigned char*)GS_SSL_PAD_ONE, GS_SSL_MD5_PAD_LEN);
			MD5Update(&md5, sslInterface->sendSeqNBO, sizeof(sslInterface->sendSeqNBO));
			MD5Update(&md5, (unsigned char*)"\x17", 1); // content type application data
			MD5Update(&md5,(unsigned char*)&lengthNBO, sizeof(lengthNBO));
			MD5Update(&md5, (unsigned char*)thePlainTextBuffer, (unsigned int)thePlainTextLength); // **cast-away const**
			MD5Final(MAC, &md5); // first half of MAC

			MD5Init(&md5);
			MD5Update(&md5, sslInterface->clientWriteMACSecret, (unsigned int)sslInterface->clientWriteMACLen);
			MD5Update(&md5, (unsigned char*)GS_SSL_PAD_TWO, GS_SSL_MD5_PAD_LEN);
			MD5Update(&md5, MAC, GS_CRYPT_MD5_HASHSIZE);
			MD5Final(MAC, &md5); // complete MAC

			// apply stream cipher to data + MAC
			RC4Encrypt(&sslInterface->sendRC4, (const unsigned char*)thePlainTextBuffer, (unsigned char*)&theEncryptedBuffer[pos], thePlainTextLength);
			pos += thePlainTextLength;
			RC4Encrypt(&sslInterface->sendRC4, MAC, (unsigned char*)&theEncryptedBuffer[pos], GS_CRYPT_MD5_HASHSIZE);
			pos += GS_CRYPT_MD5_HASHSIZE;

			// Now that we know the final length (data+mac+pad), write it into the header
			ghiEncryptorWriteNBOLength(header->lengthNBO, (int)(pos - sizeof(gsSSLRecordHeaderMsg)), 2); 

			// adjust encrypted length
			*theEncryptedLength -= pos;

			// Update the sequence number for the next message (8-byte, NBO)
			pos = 7; // **changing the semantic of variable "pos"
			do 
			{
				//int carry = 0;
				if (sslInterface->sendSeqNBO[pos] == 0xFF) // wraparound means carry
				{
					//carry = 1;
					sslInterface->sendSeqNBO[pos] = 0;
					pos -= 1;
				}
				else
				{
					sslInterface->sendSeqNBO[pos] += 1;
					pos = 0; // end addition
				}
			} while(pos >= 0);
		}
	}

	GSI_UNUSED(connection);
	return GHIEncryptionResult_Success;
}

// Decrypt some data
//    -  During the handshaking process, this may result in data being appended to the send buffer
//    -  Data may be left in the encrypted buffer
//    -  theEncryptedLength becomes the length of data read from theEncryptedBuffer
//    -  theDecryptedLength becomes the length of data written to theDecryptedBuffer
GHIEncryptionResult ghiEncryptorSslDecryptFunc(struct GHIConnection * connection,
												 struct GHIEncryptor  * theEncryptor,
												 const char * theEncryptedBuffer,
												 int *        theEncryptedLength,
												 char *       theDecryptedBuffer,
												 int *        theDecryptedLength)
{
	gsSSL* sslInterface = NULL;
	int readPos = 0;
	int writePos = 0;

	// Make sure we have a valid encryptor
	assert(theEncryptor != NULL);
	assert(theEncryptor->mInterface != NULL);
	if (theEncryptor == NULL || theEncryptor->mInterface == NULL)
	{
		// no encryption set? copy as plain text
		memcpy(theDecryptedBuffer, theEncryptedBuffer, (size_t)(*theEncryptedLength));
		*theDecryptedLength = *theEncryptedLength;
		*theEncryptedLength = 0; // no bytes remaining
		return GHIEncryptionResult_Success;
	}

	sslInterface = (gsSSL*)theEncryptor->mInterface;
	if (sslInterface == NULL)
		return GHIEncryptionResult_Error;

	// Read each SSL message from the stream (leave partial messages)
	while(readPos < *theEncryptedLength)
	{
		gsSSLRecordHeaderMsg* header = NULL;
		unsigned short length = 0;
		GHIEncryptionResult result;

		// make sure we have the complete record header
		if ((*theEncryptedLength-readPos) < sizeof(gsSSLRecordHeaderMsg))
			break;
		header = (gsSSLRecordHeaderMsg*)&theEncryptedBuffer[readPos];

		// make sure we have the complete record data
		// Warning:  Convert the length in two steps to avoid issues with byte order
		//    BAD!! -> length = ntohs((header->lengthNBO[0] | (header->lengthNBO[1] << 8)));
		//length = (unsigned short)(header->lengthNBO[0] | (header->lengthNBO[1] << 8));
		memcpy(&length, &header->lengthNBO[0], sizeof(length));
		length = ntohs(length);
		if ( *theEncryptedLength < (readPos + length +(int)sizeof(gsSSLRecordHeaderMsg)))
			break; // wait for more data

		// if we have to decrypt, make sure there is room in the decrypt buffer
		if (connection->encryptor.mSessionEstablished)
		{
			if ((*theDecryptedLength-writePos) < length)
			{
				*theEncryptedLength = readPos; // bytes read *NOT* bytes remaining
				*theDecryptedLength = writePos; // bytes written

				if (*theDecryptedLength>0)
					return GHIEncryptionResult_Success;
				else
					return GHIEncryptionResult_BufferTooSmall;
			}
		}

		//readPos += sizeof(gsSSLRecordHeaderMsg);

		// process the record data
		switch(header->contentType)
		{
		case GS_SSL_CONTENT_HANDSHAKE:
			{
				GHIBuffer data;

				// Apply stream cipher if the session has been established
				readPos += sizeof(gsSSLRecordHeaderMsg);
				if (connection->encryptor.mSessionEstablished)
					RC4Encrypt(&sslInterface->recvRC4, (const unsigned char*)&theEncryptedBuffer[readPos], (unsigned char*)&theEncryptedBuffer[readPos], length);

				ghiInitReadOnlyBuffer(connection, &data, &theEncryptedBuffer[readPos], length);

				gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
					"SSL handshake message received\r\n");

				result = ghiEncryptorProcessSSLHandshake(connection, theEncryptor, &data);
				if (result != GHIEncryptionResult_Success)
					return result; // error!

				break;
			}
		case GS_SSL_CONTENT_APPLICATIONDATA:
			{
				// make sure there is enough room to receive this data
				if ( (*theDecryptedLength-writePos) < length)
				{
				}

				// Apply stream cipher if the session has been established
				readPos += sizeof(gsSSLRecordHeaderMsg);
				if (connection->encryptor.mSessionEstablished)
					RC4Encrypt(&sslInterface->recvRC4, (const unsigned char*)&theEncryptedBuffer[readPos], (unsigned char*)&theEncryptedBuffer[readPos], length);

				// verify MAC and pad
				// verifyMAC();

				// copy to decrypted buffer so HTTP layer can process
				memcpy(theDecryptedBuffer+writePos, &theEncryptedBuffer[readPos], (size_t)(length - GS_CRYPT_MD5_HASHSIZE));
				writePos += length - GS_CRYPT_MD5_HASHSIZE;
				break;
			}
			
		case GS_SSL_CONTENT_CHANGECIPHERSPEC:
			readPos += sizeof(gsSSLRecordHeaderMsg);
			//if(readPos > *theEncryptedLength)
			//	_asm int 3;
			gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Misc, GSIDebugLevel_Debug,
				"SSL change cipher spec message received\r\n");

			gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Network, GSIDebugLevel_Notice,
				"SSL: Incoming traffic now encrypted\r\n");
			connection->encryptor.mSessionEstablished = GHTTPTrue;
			break;

		case GS_SSL_CONTENT_ALERT:
			readPos += sizeof(gsSSLRecordHeaderMsg);
			//if(readPos > *theEncryptedLength)
			//	_asm int 3;

			gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Network, GSIDebugLevel_WarmError,
				"SSL received unhandled ALERT\r\n");
			// server alert
			break;

		default:
			readPos += sizeof(gsSSLRecordHeaderMsg);
			return GHIEncryptionResult_Error; // unhandled content type
		};

		readPos += length;
	};

	// remove read bytes from the stream
	*theEncryptedLength = readPos; // bytes read *NOT* bytes remaining
	*theDecryptedLength = writePos; // bytes written

	if (*theEncryptedLength < 0)
		return GHIEncryptionResult_Error;
	else
		return GHIEncryptionResult_Success;
}

static GHTTPBool ghiCertificateChainIsValid(gsSSL* sslInterface)
{
	GSI_UNUSED(sslInterface);

	return GHTTPTrue;
}

#define CHECK(a) { if (GHTTPFalse == a) return GHIEncryptionResult_Error; }


// Programmer note: 
//    The structure of these SSL handshake messages may seem a bit cryptic, 
//    due to their variable length data items.  Refer to the ASN1/DER encoding guide
//    for tag specifics.
GHIEncryptionResult ghiEncryptorProcessSSLHandshake(struct GHIConnection * connection,
													  struct GHIEncryptor * encryptor,
													  GHIBuffer * data)
{
	// There may be multiple messages within the handshake message
	//  length must be completely used, otherwise it's a protocol error
	gsSSL* sslInterface = (gsSSL*)encryptor->mInterface;

	while(data->pos < data->len)
	{
		// Parse each SSL handshake message (there may be multiple)
		int  messageStart = data->pos;
		char messageType = 0;
		CHECK(ghiReadDataFromBufferFixed(data, &messageType, 1));

		if (messageType == GS_SSL_HANDSHAKE_SERVERHELLO)
		{
			int totalMsgLen = 0; // length of header + data
			int msgDataLen = 0;  // length of data
			int tempInt = 0;
			char tempChar = '\0';
			
			// make sure we don't have a session already (e.g. dupe hello message)
			if (sslInterface->sessionLen != 0)
				return GHIEncryptionResult_Error; // abort connection

			CHECK(ghiEncryptorReadNBOLength(data, &msgDataLen, 3));

			// check reported size against the actual bytes remaining
			if (msgDataLen > (data->len - data->pos))
				return GHIEncryptionResult_Error; // abort connection

			// skip SSL version
			//    (length check not required because we did that above)
			data->pos += 2;

			// store server random (used for key generation)
			CHECK(ghiReadDataFromBufferFixed(data, (char*)&sslInterface->serverRandom[0], 32));

			// store session information (length followed by data)
			CHECK(ghiReadDataFromBufferFixed(data, &tempChar, 1));
			CHECK(ghiReadDataFromBufferFixed(data, (char*)sslInterface->sessionData, tempChar));
			sslInterface->sessionLen = (int)tempChar;

			// store cipher suite
			CHECK(ghiEncryptorReadNBOLength(data, &tempInt, 2));
			sslInterface->cipherSuite = (unsigned short)tempInt;

			// skip compression algorithms (should always be 0x00 since we don't support any!)
			CHECK(ghiReadDataFromBufferFixed(data, &tempChar, 1));
			if (tempChar != 0x00)
				return GHIEncryptionResult_Error;
						
			// add it to the running handshake hash
			totalMsgLen = data->pos - messageStart;
			MD5Update(&sslInterface->finishHashMD5, (unsigned char*)&data->data[messageStart], (unsigned int)totalMsgLen);
			SHA1Input(&sslInterface->finishHashSHA1, (unsigned char*)&data->data[messageStart], (unsigned int)totalMsgLen);
		}
		else if (messageType == GS_SSL_HANDSHAKE_CERTIFICATE)
		{
			int msgLength = 0;    // combined length of the message (size in SSL message header)
			int certListLen = 0;  // combined length of all certificates
			int totalMsgLen = 0;  // our calculated msg length (for handshake hashing)

			int certCount = 0;
			int certListEndPos = 0;

			CHECK(ghiEncryptorReadNBOLength(data, &msgLength, 3));
			CHECK(ghiEncryptorReadNBOLength(data, &certListLen, 3));
			if (msgLength != certListLen + 3)
				return GHIEncryptionResult_Error;

			// make sure we don't have the certificates already (e.g. dupe message)
			//if (sslInterface->certificateArray != NULL)
			//	return GHIEncryptionResult_Error; // abort connection
			
			// make sure we have enough data to cover the certificate list 
			certListEndPos = data->pos + certListLen;
			if (certListLen > (data->len - data->pos))
				return GHIEncryptionResult_Error;
						
			// read the certificates
			while(data->pos < certListEndPos)
			{
				int certLength = 0;
				int certStartPos = 0;

				int temp = 0;
				int version = 0;

				// Must start with a 3 byte length
				CHECK(ghiEncryptorReadNBOLength(data, &certLength, 3));

				// Make sure we have enough data to cover this certificate
				if (certLength > (data->len - data->pos))
					return GHIEncryptionResult_Error; // certificate too big

				// 0xFFFF is max message size in SSL v3.0, we don't currently support
				// split messages
				if (certLength > 0xFFFF) 
					return GHIEncryptionResult_Error;
	
				certStartPos = data->pos; // remember this for a shortcut later
				certCount++;
								
				// make a copy of the certificate data
				//certCopy = gsimalloc(certLength);
				//if (certCopy == NULL)
				//{
				//	gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Memory, GSIDebugLevel_WarmError,
				//		"SSL failed to allocate certificate #%d (out of memory)\r\n", certCount);
				//	return GHIEncryptionResult_Error;
				//}
				//memcpy(certCopy, &data[browsePos], certLength);
				//ArrayAppend(sslInterface->certificateArray, &certCopy);

				// The first certificate holds the server's public key
				if (certCount == 1)
				{
					// X.509 format is rather convoluted.  Since we only support
					// one variation anyways, I'm hardcoding the specific values
					// we require.  Anything else is a protocol error.
					//    0x30 marks the start of a sequence.  next byte is a length field size
					//    0x82 is a length tag, meaning the next two bytes contain the length
					//    0x81 is the same thing, only the next one byte contains the length
					//    The other values usually denote required types

					// Certificate SEQUENCE
					int seqLen = 0;
					CHECK(ghiEncryptorParseASN1Sequence(data, &seqLen));
					// todo: verify reported length of this sequence

					// TBSCertificate SEQUENCE
					CHECK(ghiEncryptorParseASN1Sequence(data, &seqLen));
					// todo: verify reported length of this sequence
					
					// EXPLICIT Version (must be one of: 0x03,0x02,0x01)
					if (5 > (data->len - data->pos)) return GHIEncryptionResult_Error;
					if ((unsigned char)data->data[data->pos++] != 0xa0) return GHIEncryptionResult_Error;
					if ((unsigned char)data->data[data->pos++] != 0x03) return GHIEncryptionResult_Error;
					if ((unsigned char)data->data[data->pos++] != 0x02) return GHIEncryptionResult_Error;
					if ((unsigned char)data->data[data->pos++] != 0x01) return GHIEncryptionResult_Error;
					version = (unsigned char)data->data[data->pos++];

					// Serial Number (variable length, with 2-byte length field)
					if ((unsigned char)data->data[data->pos++] != 0x02) return GHIEncryptionResult_Error;
					temp = (unsigned char)data->data[data->pos++]; // len of serial number
					if (data->pos + temp > certListEndPos) return GHIEncryptionResult_Error;
					data->pos += temp; // skip the serial number

					// Signature algorithm identifier SEQUENCE 
					CHECK(ghiEncryptorParseASN1Sequence(data, &seqLen));
					data->pos += seqLen; // skip algorithm ID (todo: verify signatures)

					// Issuer Name
					CHECK(ghiEncryptorParseASN1Sequence(data, &seqLen));
					data->pos += seqLen; // skip the issuer name sequence

					// Validity
					CHECK(ghiEncryptorParseASN1Sequence(data, &seqLen));
					data->pos += seqLen; // skip the validity sequence

					// Subject Name
					CHECK(ghiEncryptorParseASN1Sequence(data, &seqLen));
					data->pos += seqLen; // skip the subject name sequence

					// Subject Public Key Info
					CHECK(ghiEncryptorParseASN1Sequence(data, &seqLen));
					//     AlgorithmIdentifier
					CHECK(ghiEncryptorParseASN1Sequence(data, &seqLen));
					if (seqLen != 0x0d) return GHIEncryptionResult_Error;
					if ((unsigned char)data->data[data->pos++] != 0x06) return GHIEncryptionResult_Error;
					if ((unsigned char)data->data[data->pos++] != 0x09) return GHIEncryptionResult_Error;
					if (0 != memcmp(&data->data[data->pos], gsSslRsaOid, sizeof(gsSslRsaOid)))
						return GHIEncryptionResult_Error; // only RSA certs are supported
					data->pos += sizeof(gsSslRsaOid);
					if ((unsigned char)data->data[data->pos++] != 0x05) return GHIEncryptionResult_Error;
					if ((unsigned char)data->data[data->pos++] != 0x00) return GHIEncryptionResult_Error;

					//     Bitstring (subject public key)
					if (2 > (certListEndPos - data->pos)) return GHIEncryptionResult_Error;
					if ((unsigned char)data->data[data->pos++] != 0x03) return GHIEncryptionResult_Error; // bitstring
					if ((unsigned char)data->data[data->pos++] != 0x81) return GHIEncryptionResult_Error; // 1 byte len field
					if (temp > (certListEndPos - data->pos)) return GHIEncryptionResult_Error;
					temp = (unsigned char)data->data[data->pos++]; // remaining data size (check or ignore)

					if ((unsigned char)data->data[data->pos++] != 0x00) return GHIEncryptionResult_Error;

					//     Start of the public key modulus
					CHECK(ghiEncryptorParseASN1Sequence(data, &seqLen));

					// Read out the public key modulus
					if (data->data[data->pos++] != 0x02) return GHIEncryptionResult_Error; // integer tag
					if ((data->data[data->pos]&0x80)==0x80) // ASN1 variable length field
					{
						int lensize = data->data[data->pos++]&0x7f;
						if (lensize > 4)
							return GHIEncryptionResult_Error;
						temp = 0;
						while(lensize-- > 0)
							temp = (temp << 8) | (unsigned char)data->data[data->pos++];
					}
					else
					{
						temp = (unsigned char)data->data[data->pos++];
					}
					if (data->pos + temp > certListEndPos) return GHIEncryptionResult_Error;
					if (data->data[data->pos++] != 0x00) return GHIEncryptionResult_Error; // ignore bits must be 0x00
					if (temp-1 > GS_LARGEINT_BINARY_SIZE/sizeof(char)) return GHIEncryptionResult_Error;
					sslInterface->serverpub.modulus.mLength = (unsigned int)((temp-1)/GS_LARGEINT_DIGIT_SIZE_BYTES); //-1 to subtract the previous 0x00 byte
					gsLargeIntSetFromMemoryStream(&sslInterface->serverpub.modulus, (const gsi_u8*)&data->data[data->pos], (gsi_u32)temp-1);
					data->pos += temp-1;

					// Read out the public key exponent
					if (data->data[data->pos++] != 0x02) return GHIEncryptionResult_Error; // integer
					if ((data->data[data->pos]&0x80)==0x80)
					{
						int lensize = data->data[data->pos++]&0x7f;
						if (lensize > 4)
							return GHIEncryptionResult_Error;
						temp = 0;
						while(lensize-- > 0)
							temp = (temp << 8) | (unsigned char)data->data[data->pos++];
					}
					else
					{
						temp = (unsigned char)data->data[data->pos++];
					}
					if (data->pos + temp > certListEndPos) return GHIEncryptionResult_Error;
					if (temp == 0) return GHIEncryptionResult_Error; // no exponent?
					if (temp > GS_LARGEINT_BINARY_SIZE/sizeof(char)) return GHIEncryptionResult_Error;
					sslInterface->serverpub.exponent.mLength = (unsigned int)(((temp-1)/GS_LARGEINT_DIGIT_SIZE_BYTES)+1); // ceiling of temp/4
					gsLargeIntSetFromMemoryStream(&sslInterface->serverpub.exponent, (const gsi_u8*)&data->data[data->pos], (gsi_u32)temp);
					data->pos += temp;
				}
					
				// update the position
				data->pos = certStartPos + certLength;

				GSI_UNUSED(version); 
			}
			if (data->pos != certListEndPos)
				return GHIEncryptionResult_Error; // bytes hanging off the end!

			// todo: verify certificate chain
			//       first certificate is the server's, the rest likely belong to CA
			if  (GHTTPFalse == ghiCertificateChainIsValid(sslInterface))
				return GHIEncryptionResult_Error;

			// add it to the running handshake hash
			totalMsgLen = data->pos - messageStart;
			MD5Update(&sslInterface->finishHashMD5, (unsigned char*)&data->data[messageStart], (unsigned int)totalMsgLen);
			SHA1Input(&sslInterface->finishHashSHA1, (unsigned char*)&data->data[messageStart], (unsigned int)totalMsgLen);
		}
		else if (messageType == GS_SSL_HANDSHAKE_SERVERHELLODONE)
		{
			// Process the hello done
			// Respond with 3 messages
			//    ClientKeyExchange
			//    ChangeCipherSpec
			//    Finished (final handshake)
			int i=0;

			gsSSLClientKeyExchangeMsg* clientKeyExchange = NULL;
			gsSSLRecordHeaderMsg* changeCipherSpec = NULL;
			gsSSLRecordHeaderMsg* finalHandshake = NULL;

			unsigned char temp[7];
			unsigned char hashTempMD5[GS_CRYPT_MD5_HASHSIZE];
			unsigned char hashTempSHA1[GS_CRYPT_SHA1_HASHSIZE];
			int tempInt = 0;

			// ServerHelloDone has a zero length data field
			CHECK(ghiEncryptorReadNBOLength(data, &tempInt, 3));
			if (tempInt != 0x00) return GHIEncryptionResult_Error;

			// add it to the running handshake hash
			MD5Update(&sslInterface->finishHashMD5, (unsigned char*)&data->data[messageStart], (unsigned int)(data->pos - messageStart));
			SHA1Input(&sslInterface->finishHashSHA1, (unsigned char*)&data->data[messageStart], (unsigned int)(data->pos - messageStart));

			// Make sure there is room in the send buffer for the response messages
			tempInt = (int)(sizeof(gsSSLClientKeyExchangeMsg) + sslInterface->serverpub.modulus.mLength*GS_LARGEINT_DIGIT_SIZE_BYTES);
			while (connection->sendBuffer.size - connection->sendBuffer.len < tempInt)
			{
				// not enough room in send buffer, try to grow it
				if (GHTTPFalse == ghiResizeBuffer(&connection->sendBuffer, connection->sendBuffer.sizeIncrement))
					return GHIEncryptionResult_Error; 
			}
						
			// 1) Client key exchange,
			//    create the pre-master-secret
			sslInterface->premastersecret[0] = GS_SSL_VERSION_MAJOR;
			sslInterface->premastersecret[1] = GS_SSL_VERSION_MINOR;
			for (i=2; i<GS_SSL_MASTERSECRET_LEN; i++)
			{
				#if defined(GS_CRYPT_NO_RANDOM)
					#pragma message ("!!!WARNING: SSL Random disable for debug purposes.  SSL not secured!!!")
					// Use zero as the random so we can packet sniff for debug
					//   warning! : this compromises the SSL security
					sslInterface->premastersecret[i] = 0; // rand()
				#else
					Util_RandSeed(current_time());
					sslInterface->premastersecret[i] = (unsigned char)(Util_RandInt(0, 0x0100)); // range = [0...FF]
				#endif
			}

			clientKeyExchange = (gsSSLClientKeyExchangeMsg*)&connection->sendBuffer.data[connection->sendBuffer.len];
			connection->sendBuffer.len += sizeof(gsSSLClientKeyExchangeMsg);
			clientKeyExchange->header.contentType = GS_SSL_CONTENT_HANDSHAKE;
			clientKeyExchange->header.versionMajor = GS_SSL_VERSION_MAJOR;
			clientKeyExchange->header.versionMinor = GS_SSL_VERSION_MINOR;
			ghiEncryptorWriteNBOLength(clientKeyExchange->header.lengthNBO, (int)(sslInterface->serverpub.modulus.mLength*GS_LARGEINT_DIGIT_SIZE_BYTES+4), 2);
			clientKeyExchange->handshakeType = GS_SSL_HANDSHAKE_CLIENTKEYEXCHANGE;
			ghiEncryptorWriteNBOLength(clientKeyExchange->lengthNBO, (int)(sslInterface->serverpub.modulus.mLength*GS_LARGEINT_DIGIT_SIZE_BYTES), 3);
			//    encrypt the preMasterSecret using the server's public key (store result in sendbuffer)
			gsCryptRSAEncryptBuffer(&sslInterface->serverpub, sslInterface->premastersecret, 
				GS_SSL_MASTERSECRET_LEN, (unsigned char*)&connection->sendBuffer.data[connection->sendBuffer.len]);
			connection->sendBuffer.len += sslInterface->serverpub.modulus.mLength*GS_LARGEINT_DIGIT_SIZE_BYTES;
	
			// add it to the running handshake hash
			MD5Update(&sslInterface->finishHashMD5, (unsigned char*)clientKeyExchange+sizeof(gsSSLRecordHeaderMsg), 
				sizeof(gsSSLClientKeyExchangeMsg) - sizeof(gsSSLRecordHeaderMsg) + 
				sslInterface->serverpub.modulus.mLength*GS_LARGEINT_DIGIT_SIZE_BYTES);
			SHA1Input(&sslInterface->finishHashSHA1, (unsigned char*)clientKeyExchange+sizeof(gsSSLRecordHeaderMsg), 
				sizeof(gsSSLClientKeyExchangeMsg) - sizeof(gsSSLRecordHeaderMsg) +
				sslInterface->serverpub.modulus.mLength*GS_LARGEINT_DIGIT_SIZE_BYTES);


			// 2) change cipher spec
			changeCipherSpec = (gsSSLRecordHeaderMsg*)&connection->sendBuffer.data[connection->sendBuffer.len];
			changeCipherSpec->contentType = GS_SSL_CONTENT_CHANGECIPHERSPEC;
			changeCipherSpec->versionMajor = GS_SSL_VERSION_MAJOR;
			changeCipherSpec->versionMinor = GS_SSL_VERSION_MINOR;
			changeCipherSpec->lengthNBO[0] = 0;
			changeCipherSpec->lengthNBO[1] = 1; // always one byte length 
			connection->sendBuffer.len += sizeof(gsSSLRecordHeaderMsg);
			connection->sendBuffer.data[connection->sendBuffer.len++] = 0x01; // always set to 0x01
			// DO NOT add it to the running handshake hash (its content is not GS_SSL_CONTENT_HANDSHAKE)

			// Calculate the encryption keys
			ghiEncryptorGenerateEncryptionKeys(sslInterface);

			// 3) final handshake message (encrypted)
			finalHandshake = (gsSSLRecordHeaderMsg*)&connection->sendBuffer.data[connection->sendBuffer.len];
			finalHandshake->contentType = GS_SSL_CONTENT_HANDSHAKE;
			finalHandshake->versionMajor = GS_SSL_VERSION_MAJOR;
			finalHandshake->versionMinor = GS_SSL_VERSION_MINOR;
			finalHandshake->lengthNBO[0] = 0;
			finalHandshake->lengthNBO[1] = 56; // handshake type(1)+handshake lenNBO(3)+SHA1(20)+MD5(16)+MAC(16)
			connection->sendBuffer.len += sizeof(gsSSLRecordHeaderMsg);
			connection->sendBuffer.data[connection->sendBuffer.len++] = GS_SSL_HANDSHAKE_FINISHED;
			ghiEncryptorWriteNBOLength((unsigned char*)&connection->sendBuffer.data[connection->sendBuffer.len], 36, 3);
			connection->sendBuffer.len += 3;


			// MD5(master_secret + pad2 + MD5(handshake_messages+"CLNT"+master_secret+pad1))
			// SHA1(master_secret + pad2 + SHA1(handshake_messages+"CLNT"+master_secret+pad1))
			// prepare the final hashes (inner hashes)
			MD5Update(&sslInterface->finishHashMD5, (unsigned char*)GS_SSL_CLIENT_FINISH_VALUE, 4);
			MD5Update(&sslInterface->finishHashMD5, (unsigned char*)sslInterface->mastersecret, GS_SSL_MASTERSECRET_LEN);
			MD5Update(&sslInterface->finishHashMD5, (unsigned char*)GS_SSL_PAD_ONE, GS_SSL_MD5_PAD_LEN);
			MD5Final(hashTempMD5, &sslInterface->finishHashMD5);

			SHA1Input(&sslInterface->finishHashSHA1, (unsigned char*)GS_SSL_CLIENT_FINISH_VALUE, 4);
			SHA1Input(&sslInterface->finishHashSHA1, (unsigned char*)sslInterface->mastersecret, GS_SSL_MASTERSECRET_LEN);
			SHA1Input(&sslInterface->finishHashSHA1, (unsigned char*)GS_SSL_PAD_ONE, GS_SSL_SHA1_PAD_LEN);
			SHA1Result(&sslInterface->finishHashSHA1, hashTempSHA1);

			// prepare the final hashes (outer hashes)
			MD5Init(&sslInterface->finishHashMD5);
			MD5Update(&sslInterface->finishHashMD5, (unsigned char*)sslInterface->mastersecret, GS_SSL_MASTERSECRET_LEN);
			MD5Update(&sslInterface->finishHashMD5, (unsigned char*)GS_SSL_PAD_TWO, GS_SSL_MD5_PAD_LEN);
			MD5Update(&sslInterface->finishHashMD5, hashTempMD5, GS_CRYPT_MD5_HASHSIZE);
			MD5Final(hashTempMD5, &sslInterface->finishHashMD5);

			SHA1Reset(&sslInterface->finishHashSHA1);
			SHA1Input(&sslInterface->finishHashSHA1, (unsigned char*)sslInterface->mastersecret, GS_SSL_MASTERSECRET_LEN);
			SHA1Input(&sslInterface->finishHashSHA1, (unsigned char*)GS_SSL_PAD_TWO, GS_SSL_SHA1_PAD_LEN);
			SHA1Input(&sslInterface->finishHashSHA1, hashTempSHA1, GS_CRYPT_SHA1_HASHSIZE);
			SHA1Result(&sslInterface->finishHashSHA1, hashTempSHA1);

			// copy results into the sendbuffer
			memcpy(&connection->sendBuffer.data[connection->sendBuffer.len], hashTempMD5, GS_CRYPT_MD5_HASHSIZE);
			connection->sendBuffer.len += GS_CRYPT_MD5_HASHSIZE;
			memcpy(&connection->sendBuffer.data[connection->sendBuffer.len], hashTempSHA1, GS_CRYPT_SHA1_HASHSIZE);
			connection->sendBuffer.len += GS_CRYPT_SHA1_HASHSIZE;
			
			// output the message MAC  (hash(MAC_write_secret+pad_2+ hash(MAC_write_secret+pad_1+seq_num+length+content)));
			// Re-using the finishHashMD5 since it has already been allocated
			MD5Init(&sslInterface->finishHashMD5);
			MD5Update(&sslInterface->finishHashMD5, (unsigned char*)sslInterface->clientWriteMACSecret, GS_CRYPT_MD5_HASHSIZE);
			MD5Update(&sslInterface->finishHashMD5, (unsigned char*)GS_SSL_PAD_ONE, GS_SSL_MD5_PAD_LEN);
			MD5Update(&sslInterface->finishHashMD5, (unsigned char*)sslInterface->sendSeqNBO, 8);
			temp[0] = 0x16;
			temp[1] = (unsigned char)((GS_CRYPT_MD5_HASHSIZE+GS_CRYPT_SHA1_HASHSIZE+4)>>8);
			temp[2] = (unsigned char)((GS_CRYPT_MD5_HASHSIZE+GS_CRYPT_SHA1_HASHSIZE+4));
			//temp[1] = (unsigned char)(htons(GS_CRYPT_MD5_HASHSIZE+GS_CRYPT_SHA1_HASHSIZE));
			//temp[2] = (unsigned char)(htons(GS_CRYPT_MD5_HASHSIZE+GS_CRYPT_SHA1_HASHSIZE+4)>>8);
			temp[3] = 0x14; // 20-bytes of data (MD5+SHA1)
			temp[4] = 0x00; // 3-byte length NBO
			temp[5] = 0x00; // ..
			temp[6] = 0x24; // ..
			MD5Update(&sslInterface->finishHashMD5, (unsigned char*)&temp, 7);
			MD5Update(&sslInterface->finishHashMD5, hashTempMD5, GS_CRYPT_MD5_HASHSIZE);   // content part 1
			MD5Update(&sslInterface->finishHashMD5, hashTempSHA1, GS_CRYPT_SHA1_HASHSIZE); // content part 2
			MD5Final(hashTempMD5, &sslInterface->finishHashMD5);
			MD5Init(&sslInterface->finishHashMD5); // reset for outer hash
			MD5Update(&sslInterface->finishHashMD5, (unsigned char*)sslInterface->clientWriteMACSecret, GS_CRYPT_MD5_HASHSIZE);
			MD5Update(&sslInterface->finishHashMD5, (unsigned char*)GS_SSL_PAD_TWO, GS_SSL_MD5_PAD_LEN);
			MD5Update(&sslInterface->finishHashMD5, hashTempMD5, GS_CRYPT_MD5_HASHSIZE);
			MD5Final(hashTempMD5, &sslInterface->finishHashMD5);
			
			memcpy(&connection->sendBuffer.data[connection->sendBuffer.len], hashTempMD5, GS_CRYPT_MD5_HASHSIZE);
			connection->sendBuffer.len += GS_CRYPT_MD5_HASHSIZE;

			// increment sequence each time we send a message
			//   ...assume NBO is bigendian for simplicity
			memset(sslInterface->sendSeqNBO, 0, sizeof(sslInterface->sendSeqNBO));
			ghiEncryptorWriteNBOLength(&sslInterface->sendSeqNBO[4], 1, 4);
			
			// now encrypt the message (not including record header)
			RC4Encrypt(&sslInterface->sendRC4, 
				((unsigned char*)finalHandshake)+sizeof(gsSSLRecordHeaderMsg), 
				((unsigned char*)finalHandshake)+sizeof(gsSSLRecordHeaderMsg),
				56);
		}
		else if (messageType == GS_SSL_HANDSHAKE_FINISHED)
		{
			// process server finished and verify hashes
			gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Network, GSIDebugLevel_Notice,
				"SSL: todo - verify server finished hash\r\n");
			data->pos = data->len; 
		}
		else
		{
			gsDebugFormat(GSIDebugCat_HTTP, GSIDebugType_Network, GSIDebugLevel_WarmError,
				"SSL received unexpected handshake message type: %d\r\n", messageType);
			return GHIEncryptionResult_Error; // abort connection
		}
	}

	GSI_UNUSED(connection);

	if (data->pos == data->len)
		return GHIEncryptionResult_Success;
	else
		return GHIEncryptionResult_Error; // too many or too few bytes, protocol error!
}

GHIEncryptionResult ghiEncryptorSslEncryptSend(struct GHIConnection * connection,
												 struct GHIEncryptor  * theEncryptor,
												 const char * thePlainTextBuffer,
												 int          thePlainTextLength,
												 int *        theBytesSentOut)
{
	GS_FAIL(); // Should never call this for GameSpy SSL!  It uses encrypt on buffer

	GSI_UNUSED(connection);
	GSI_UNUSED(theEncryptor);
	GSI_UNUSED(thePlainTextBuffer);
	GSI_UNUSED(thePlainTextLength);
	GSI_UNUSED(theBytesSentOut);
	
	return GHIEncryptionResult_Error;
}

GHIEncryptionResult ghiEncryptorSslDecryptRecv(struct GHIConnection * connection,
												 struct GHIEncryptor  * theEncryptor,
												 char * theDecryptedBuffer,
												 int *  theDecryptedLength)
{
	GS_FAIL(); // Should never call this for GameSpy SSL!  It uses decrypt on buffer

	GSI_UNUSED(connection);
	GSI_UNUSED(theEncryptor);
	GSI_UNUSED(theDecryptedBuffer);
	GSI_UNUSED(theDecryptedLength);

	return GHIEncryptionResult_Error;	
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#endif // encryption engine switch


