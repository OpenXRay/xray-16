///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "../common/gsCore.h"

#include "sci.h"
#include "sciInterface.h"
#include "sciWebServices.h"
#include "sciReport.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#define SC_CREATEMATCHLESSSESSION_SOAPACTION "SOAPAction: \"http://gamespy.net/competition/CreateMatchlessSession\""
#define SC_CREATESESSION_SOAPACTION "SOAPAction: \"http://gamespy.net/competition/CreateSession\""
#define SC_SUBMITREPORT_SOAPACTION  "SOAPAction: \"http://gamespy.net/competition/SubmitReport\""
#define SC_SETINTENTION_SOAPACTION  "SOAPAction: \"http://gamespy.net/competition/SetReportIntention\""

#define SC_SERVICE_NAMESPACE_COUNT     1
const char * SC_SERVICE_NAMESPACES[SC_SERVICE_NAMESPACE_COUNT] =
{
	"gsc=\"http://gamespy.net/competition/\""
};


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SCResult sciWsInit(SCWebServices* theWebServices,
                   SCInterfacePtr theInterface)
{

	
	GS_ASSERT(theWebServices != NULL);
	GS_ASSERT(theInterface != NULL);
	GS_ASSERT(!theWebServices->mInit);

	// Check gsCore
	if (gsCoreIsShutdown())
	{
		return SCResult_CORE_NOT_INITIALIZED;
	}

	// Initialize SCWebServices struct
	theWebServices->mInterface                  = theInterface;
	theWebServices->mCreateSessionCallback      = NULL;
	theWebServices->mSetReportIntentionCallback = NULL;
	theWebServices->mSubmitReportDataCallback   = NULL;
	theWebServices->mCreateSessionUserData      = NULL;
	theWebServices->mSetReportIntentionUserData = NULL;
	theWebServices->mSubmitReportUserData       = NULL;
	theWebServices->mCreateSessionPending       = gsi_false;
	theWebServices->mSetReportIntentionPending  = gsi_false;
	theWebServices->mSubmitReportPending        = gsi_false;

	// Now initialized
	theWebServices->mInit = gsi_true;

	return SCResult_NO_ERROR;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void sciWsDestroy(SCWebServices* theWebServices)
{
	GS_ASSERT(theWebServices != NULL);
	GS_ASSERT(theWebServices->mInit);

	// No longer initialized
	theWebServices->mInit = gsi_false;

	// Destroy SCWebServices struct
	theWebServices->mCreateSessionCallback      = NULL;
	theWebServices->mSetReportIntentionCallback = NULL;
	theWebServices->mSubmitReportDataCallback   = NULL;
	theWebServices->mCreateSessionUserData      = NULL;
	theWebServices->mSetReportIntentionUserData = NULL;
	theWebServices->mSubmitReportUserData       = NULL;
	theWebServices->mCreateSessionPending       = gsi_false;
	theWebServices->mSetReportIntentionPending  = gsi_false;
	theWebServices->mSubmitReportPending        = gsi_false;
	theWebServices->mInterface = NULL;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void sciWsThink(SCWebServices* theWebServices)
{
	GS_ASSERT(theWebServices != NULL);
	GS_ASSERT(theWebServices->mInit);

	gsCoreThink(0);

	GSI_UNUSED(theWebServices);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SCResult sciWsCreateSession (SCWebServices * theWebServices,
							 gsi_u32         theGameId,
							 const GSLoginCertificate * theCertificate,
							 const GSLoginPrivateData * thePrivateData,
							 SCCreateSessionCallback theCallback,
							 gsi_time        theTimeoutMs,
							 void *          theUserData)
{
	GSXmlStreamWriter aRequest = NULL;

	// Check parameters
	GS_ASSERT(theWebServices != NULL);
	GS_ASSERT(theWebServices->mInit);

	// Check for pending request
	if (theWebServices->mCreateSessionPending)
		return SCResult_CALLBACK_PENDING;

	// Create the XML message writer
	aRequest = gsXmlCreateStreamWriter(SC_SERVICE_NAMESPACES, SC_SERVICE_NAMESPACE_COUNT);
	if (aRequest == NULL)
		return SCResult_OUT_OF_MEMORY;

	// Fill in the request data
	if (gsi_is_false(gsXmlWriteOpenTag(aRequest, "gsc", "CreateSession")) ||
		gsi_is_false(gsXmlWriteOpenTag(aRequest, "gsc", "certificate")) ||
		gsi_is_false(wsLoginCertWriteXML(theCertificate, "gsc", aRequest)) ||
		gsi_is_false(gsXmlWriteCloseTag(aRequest, "gsc", "certificate")) ||
		gsi_is_false(gsXmlWriteHexBinaryElement(aRequest, "gsc", "proof", (const gsi_u8*)thePrivateData->mKeyHash, GS_CRYPT_MD5_HASHSIZE)) ||
		gsi_is_false(gsXmlWriteIntElement(aRequest, "gsc", "gameid", (gsi_u32)theGameId)) ||
		gsi_is_false(gsXmlWriteCloseTag(aRequest, "gsc", "CreateSession")) ||
		gsi_is_false(gsXmlCloseWriter(aRequest))
		)
	{
		gsXmlFreeWriter(aRequest);
		return SCResult_HTTP_ERROR;
	}

	// Set callback
	theWebServices->mCreateSessionCallback = theCallback;
	theWebServices->mCreateSessionUserData = theUserData;
	theWebServices->mCreateSessionPending  = gsi_true;

	// Execute soap call
	gsiExecuteSoap(scServiceURL, SC_CREATESESSION_SOAPACTION, 
		aRequest, sciWsCreateSessionCallback, theWebServices);
	GSI_UNUSED(theTimeoutMs);
	return SCResult_NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SCResult sciWsCreateMatchlessSession (SCWebServices * theWebServices,
							          gsi_u32         theGameId,
							          const GSLoginCertificate * theCertificate,
							          const GSLoginPrivateData * thePrivateData,
							          SCCreateSessionCallback theCallback,
							          gsi_time        theTimeoutMs,
							          void *          theUserData)
{
	GSXmlStreamWriter aRequest = NULL;

	// Check parameters
	GS_ASSERT(theWebServices != NULL);
	GS_ASSERT(theWebServices->mInit);

	// Check for pending request
	if (theWebServices->mCreateSessionPending)
		return SCResult_CALLBACK_PENDING;

	// Create the XML message writer
	aRequest = gsXmlCreateStreamWriter(SC_SERVICE_NAMESPACES, SC_SERVICE_NAMESPACE_COUNT);
	if (aRequest == NULL)
		return SCResult_OUT_OF_MEMORY;

	// Fill in the request data
	if (gsi_is_false(gsXmlWriteOpenTag(aRequest, "gsc", "CreateMatchlessSession")) ||
		gsi_is_false(gsXmlWriteOpenTag(aRequest, "gsc", "certificate")) ||
		gsi_is_false(wsLoginCertWriteXML(theCertificate, "gsc", aRequest)) ||
		gsi_is_false(gsXmlWriteCloseTag(aRequest, "gsc", "certificate")) ||
		gsi_is_false(gsXmlWriteHexBinaryElement(aRequest, "gsc", "proof", (const gsi_u8*)thePrivateData->mKeyHash, GS_CRYPT_MD5_HASHSIZE)) ||
		gsi_is_false(gsXmlWriteIntElement(aRequest, "gsc", "gameid", (gsi_u32)theGameId)) ||
		gsi_is_false(gsXmlWriteCloseTag(aRequest, "gsc", "CreateMatchlessSession")) ||
		gsi_is_false(gsXmlCloseWriter(aRequest))
		)
	{
		gsXmlFreeWriter(aRequest);
		return SCResult_HTTP_ERROR;
	}

	// Set callback
	theWebServices->mCreateSessionCallback = theCallback;
	theWebServices->mCreateSessionUserData = theUserData;
	theWebServices->mCreateSessionPending  = gsi_true;

	// Execute soap call
	gsiExecuteSoap(scServiceURL, SC_CREATEMATCHLESSSESSION_SOAPACTION, 
		aRequest, sciWsCreateSessionCallback, theWebServices);
	GSI_UNUSED(theTimeoutMs);
	return SCResult_NO_ERROR;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void sciWsCreateSessionCallback(GHTTPResult theHttpResult,
								GSXmlStreamWriter  theRequestData,
								GSXmlStreamReader  theResponseData,
								void*        theUserData)
{
	SCResult aTranslatedResult     = SCResult_HTTP_ERROR;
	SCWebServices* aWebServices    = (SCWebServices*)theUserData;
	
	char csid[255];
	char ccid[255];
	int csidLen = 255;
	int ccidLen = 255;

	GS_ASSERT(aWebServices != NULL);
	GS_ASSERT(aWebServices->mCreateSessionPending);

	// Check for shutdown
	if (!aWebServices->mInit)
		return;

	if (theHttpResult == GHTTPSuccess)
	{
		int createResult = 0;

		// Parse through in a way that will work for either type of CreateSession response.
		if (gsi_is_false(gsXmlMoveToStart(theResponseData)))
		{
			aTranslatedResult = SCResult_RESPONSE_INVALID;
		}
		else if(gsi_is_false(gsXmlMoveToNext(theResponseData, "CreateSessionResponse")))
		{
			if(gsi_is_false(gsXmlMoveToNext(theResponseData, "CreateMatchlessSessionResponse")))
			{
				aTranslatedResult = SCResult_RESPONSE_INVALID;
			}
		}
		
		if(gsi_is_false(gsXmlMoveToNext(theResponseData, "CreateSessionResult")))
		{
			if(gsi_is_false(gsXmlMoveToNext(theResponseData, "CreateMatchlessSessionResult")))
			{
				aTranslatedResult = SCResult_RESPONSE_INVALID;
			}
		}

		if(gsi_is_false(gsXmlReadChildAsInt(theResponseData, "result", &createResult)))
		{
			aTranslatedResult = SCResult_RESPONSE_INVALID;
		}
		else if(aTranslatedResult != SCResult_RESPONSE_INVALID)
		{
			// Parse server reported result
			if (createResult == SCWsResult_NO_ERROR)
			{
				// Read session and connection ID
				if(gsi_is_false(gsXmlReadChildAsStringNT(theResponseData, "csid", csid, csidLen)) ||
				   gsi_is_false(gsXmlReadChildAsStringNT(theResponseData, "ccid", ccid, ccidLen))
				   )
				{
					aTranslatedResult = SCResult_RESPONSE_INVALID;
				}
				else
				{
					sciInterfaceSetSessionId((SCInterface*)aWebServices->mInterface, csid);
					sciInterfaceSetConnectionId((SCInterface*)aWebServices->mInterface, ccid);
					aTranslatedResult = SCResult_NO_ERROR;
				}
			}
			else
			{
				// Server reported an error, handle it?

				// TODO:
				//   translate result into developer useable form
				//   report result string as gsDebugFormat message for easier debugging
				aTranslatedResult = SCResult_RESPONSE_INVALID;
			}
		}
	}
	else
	{
		aTranslatedResult = SCResult_HTTP_ERROR;
	}

	// Client callback
	aWebServices->mCreateSessionPending = gsi_false;
	if (aWebServices->mCreateSessionCallback != NULL)
	{
		aWebServices->mCreateSessionCallback(aWebServices->mInterface, theHttpResult, aTranslatedResult, aWebServices->mCreateSessionUserData);
		aWebServices->mCreateSessionUserData = NULL;
		aWebServices->mCreateSessionCallback = NULL;
	}
	GSI_UNUSED(theRequestData);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SCResult sciWsSetReportIntention(SCWebServices* theWebServices,
								 gsi_u32        theGameId,
								 const char *   theSessionId,
								 const char *   theConnectionId,
								 gsi_bool       isAuthoritative,
								 const GSLoginCertificate * theCertificate,
								 const GSLoginPrivateData * thePrivateData,
                                 SCSetReportIntentionCallback theCallback,
                                 gsi_time       theTimeoutMs,
								 void *         theUserData)
{
	GSXmlStreamWriter aRequest = NULL;

	// Check parameters
	GS_ASSERT(theWebServices != NULL);
	GS_ASSERT(theWebServices->mInit);

	// Check for pending request
	if (theWebServices->mSetReportIntentionPending)
		return SCResult_CALLBACK_PENDING;

	// Create the XML message writer
	aRequest = gsXmlCreateStreamWriter(SC_SERVICE_NAMESPACES, SC_SERVICE_NAMESPACE_COUNT);
	if (aRequest == NULL)
		return SCResult_OUT_OF_MEMORY;

	// Fill in the request data
	if (gsi_is_false(gsXmlWriteOpenTag(aRequest, "gsc", "SetReportIntention")) ||
		gsi_is_false(gsXmlWriteOpenTag(aRequest, "gsc", "certificate")) ||
		gsi_is_false(wsLoginCertWriteXML(theCertificate, "gsc", aRequest)) ||
		gsi_is_false(gsXmlWriteCloseTag(aRequest, "gsc", "certificate")) ||
		gsi_is_false(gsXmlWriteHexBinaryElement(aRequest, "gsc", "proof", (const gsi_u8*)thePrivateData->mKeyHash, GS_CRYPT_MD5_HASHSIZE)) ||
		gsi_is_false(gsXmlWriteStringElement(aRequest, "gsc", "csid", theSessionId)) ||
		gsi_is_false(gsXmlWriteStringElement(aRequest, "gsc", "ccid", theConnectionId)) ||
		gsi_is_false(gsXmlWriteIntElement(aRequest, "gsc", "gameid", (gsi_u32)theGameId)) ||
		gsi_is_false(gsXmlWriteIntElement(aRequest, "gsc", "authoritative", (gsi_u32)(gsi_is_true(isAuthoritative) ? 1:0))) ||
		gsi_is_false(gsXmlWriteCloseTag(aRequest, "gsc", "SetReportIntention")) ||
		gsi_is_false(gsXmlCloseWriter(aRequest))
		)
	{
		gsXmlFreeWriter(aRequest);
		return SCResult_HTTP_ERROR;
	}

	// Set callback
	theWebServices->mSetReportIntentionCallback = theCallback;
	theWebServices->mSetReportIntentionUserData = theUserData;
	theWebServices->mSetReportIntentionPending = gsi_true;

	// Execute soap call
	gsiExecuteSoap(scServiceURL, SC_SETINTENTION_SOAPACTION, 
		aRequest, sciWsSetReportIntentionCallback, theWebServices);
	
	GSI_UNUSED(theTimeoutMs);
	return SCResult_NO_ERROR;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void sciWsSetReportIntentionCallback(GHTTPResult theHttpResult,
                                     GSXmlStreamWriter theRequestData,
                                     GSXmlStreamReader theResponseData,
                                     void*       theUserData)
{
	SCResult aTranslatedResult     = SCResult_HTTP_ERROR;
	SCWebServices* aWebServices    = (SCWebServices*)theUserData;

	char ccid[255];
	int ccidLen = 255;


	GS_ASSERT(aWebServices != NULL);
	GS_ASSERT(aWebServices->mSetReportIntentionPending);

	// Check for shutdown
	if (!aWebServices->mInit)
		return;

	if (theHttpResult == GHTTPSuccess)
	{
		int intentionResult = 0;

		if (gsi_is_false(gsXmlMoveToStart(theResponseData)) ||
			gsi_is_false(gsXmlMoveToNext(theResponseData, "SetReportIntentionResponse")) ||
			gsi_is_false(gsXmlMoveToNext(theResponseData, "SetReportIntentionResult")) ||
			gsi_is_false(gsXmlReadChildAsInt(theResponseData, "result", &intentionResult)) ||
  		    gsi_is_false(gsXmlReadChildAsStringNT(theResponseData, "ccid", ccid, ccidLen))
			)
		{
			aTranslatedResult = SCResult_RESPONSE_INVALID;
		}
		else
		{
			if (intentionResult == SCWsResult_NO_ERROR)
			{
				aTranslatedResult = SCResult_NO_ERROR;
				sciInterfaceSetConnectionId((SCInterface*)aWebServices->mInterface, ccid);
			}
			else
				aTranslatedResult = SCResult_UNKNOWN_RESPONSE;
		}
	}
	else
	{
		aTranslatedResult = SCResult_HTTP_ERROR;
	}

	// Client callback
	aWebServices->mSetReportIntentionPending = gsi_false;
	if (aWebServices->mSetReportIntentionCallback != NULL)
	{
		aWebServices->mSetReportIntentionCallback(aWebServices->mInterface,
			                                      theHttpResult,
		                                          aTranslatedResult,
												  aWebServices->mSetReportIntentionUserData);
		aWebServices->mSetReportIntentionUserData = NULL;
		aWebServices->mSetReportIntentionCallback = NULL;
	}
	GSI_UNUSED(theRequestData);
}


///////////////////////////////////////////////////////////////////////////////
// declared here to allow function to get around Unicode calls
extern GHTTPBool ghiPostAddFileFromMemory(GHTTPPost post,const char * name,const char * buffer,
	int bufferLen,const char * reportFilename,const char * contentType);
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Private GSSoapCustomFunc used by sciWsSubmitReport
static void sciWsSubmitReportCustom(GHTTPPost thePost, void* theUserData)
{
	SCWebServices* aWebServices = (SCWebServices*)theUserData;

	//Use internal method to get around unicode calls
	ghiPostAddFileFromMemory(thePost, "report", (char *)aWebServices->mSubmitReportData,
		(gsi_i32)aWebServices->mSubmitReportLength, "report", "application/bin");
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SCResult sciWsSubmitReport(SCWebServices* theWebServices,
						   gsi_u32        theGameId,
						   const char *   theSessionId,
						   const char *   theConnectionId,
						   const SCIReport *    theReport,
						   gsi_bool       isAuthoritative,
						   const GSLoginCertificate * theCertificate,
						   const GSLoginPrivateData * thePrivateData,
                           SCSubmitReportCallback theCallback,
                           gsi_time       theTimeoutMs,
						   void *         theUserData)
{
	GSXmlStreamWriter aRequest = NULL;
	//SCIReportHeader * aReportHeader = NULL;
	//gsi_u32 aTotalSize = 0;

	//SCReportStatus*   aStatus  = NULL;

	// Check parameters
	GS_ASSERT(theWebServices != NULL);
	//GS_ASSERT(theReportData != NULL);

	// Check for pending request
	if (theWebServices->mSubmitReportPending)
	{
		return SCResult_CALLBACK_PENDING;
	}

	// Get a pointer to the header
	//aReportHeader = (SCIReportHeader*)theReport->mBuffer.mData;

	// Check for complete report
	if (theReport->mBuffer.mPos < sizeof(SCIReportHeader))
		return SCResult_REPORT_INVALID;

	// Check size (early check for easier debugging)
	//aTotalSize = sizeof(SCIReportHeader);
	//aTotalSize += htonl(aReportHeader->mPlayerDataLength);
	//aTotalSize += htonl(aReportHeader->mTeamDataLength);
	//aTotalSize += htonl(aReportHeader->mSessionDataLength);
	// aTotalSize += auth info...
	//if (theReport->mBuffer.mPos != aTotalSize)
	//	return SCResult_REPORT_INVALID;
	
	// Create the XML message writer
	aRequest = gsXmlCreateStreamWriter(SC_SERVICE_NAMESPACES, SC_SERVICE_NAMESPACE_COUNT);
	if (aRequest == NULL)
		return SCResult_OUT_OF_MEMORY;

	// Fill in the request data
	if (gsi_is_false(gsXmlWriteOpenTag(aRequest, "gsc", "SubmitReport")) ||
		gsi_is_false(gsXmlWriteOpenTag(aRequest, "gsc", "certificate")) ||
		gsi_is_false(wsLoginCertWriteXML(theCertificate, "gsc", aRequest)) ||
		gsi_is_false(gsXmlWriteCloseTag(aRequest, "gsc", "certificate")) ||
		gsi_is_false(gsXmlWriteHexBinaryElement(aRequest, "gsc", "proof", (const gsi_u8*)thePrivateData->mKeyHash, GS_CRYPT_MD5_HASHSIZE)) ||
		gsi_is_false(gsXmlWriteStringElement(aRequest, "gsc", "csid", theSessionId)) ||
		gsi_is_false(gsXmlWriteStringElement(aRequest, "gsc", "ccid", theConnectionId)) ||
		gsi_is_false(gsXmlWriteIntElement(aRequest, "gsc", "gameid", (gsi_u32)theGameId)) ||
		gsi_is_false(gsXmlWriteIntElement(aRequest, "gsc", "authoritative", (gsi_u32)(gsi_is_true(isAuthoritative) ? 1:0))) ||
		gsi_is_false(gsXmlWriteCloseTag(aRequest, "gsc", "SubmitReport")) ||
		gsi_is_false(gsXmlCloseWriter(aRequest))
		)
	{
		gsXmlFreeWriter(aRequest);
		return SCResult_OUT_OF_MEMORY;
	}
	
	// Get submission size
	theWebServices->mSubmitReportData  = (gsi_u8*)theReport->mBuffer.mData;
	theWebServices->mSubmitReportLength = theReport->mBuffer.mPos;

	// Set callback
	theWebServices->mSubmitReportDataCallback = theCallback;
	theWebServices->mSubmitReportUserData     = theUserData;
	theWebServices->mSubmitReportPending      = gsi_true;

	// Execute soap call
	gsiExecuteSoapCustom(scServiceURL, SC_SUBMITREPORT_SOAPACTION, 
		aRequest, sciWsSubmitReportCallback,sciWsSubmitReportCustom, theWebServices);
	
	GSI_UNUSED(theTimeoutMs);
	return SCResult_NO_ERROR;
}



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void sciWsSubmitReportCallback(GHTTPResult       theHttpResult,
                               GSXmlStreamWriter theRequestData,
                               GSXmlStreamReader theResponseData,
                               void*             theUserData)
{
	SCResult aTranslatedResult     = SCResult_HTTP_ERROR;
	SCWebServices* aWebServices    = (SCWebServices*)theUserData;

	GS_ASSERT(aWebServices != NULL);

	// Check for shutdown
	if (!aWebServices->mInit)
		return;

	GS_ASSERT(aWebServices->mSubmitReportPending);

	if (theHttpResult == GHTTPSuccess)
	{
		int submitResult = 0;

		if (gsi_is_false(gsXmlMoveToStart(theResponseData)) ||
			gsi_is_false(gsXmlMoveToNext(theResponseData, "SubmitReportResponse")) ||
			gsi_is_false(gsXmlMoveToNext(theResponseData, "SubmitReportResult")) ||
			gsi_is_false(gsXmlReadChildAsInt(theResponseData, "result", &submitResult))
			)
		{
			aTranslatedResult = SCResult_RESPONSE_INVALID;
		}
		else
		{
			switch (submitResult)
			{
			case SCWsResult_NO_ERROR:       
				aTranslatedResult = SCResult_NO_ERROR;          
				break;
			case SCWsResult_REPORT_INVALID: 
				aTranslatedResult = SCResult_REPORT_INVALID;    
				break;
			case SCWsResult_SINGLE_ATTACHMENT_EXPECTED: 
				aTranslatedResult = SCResult_SUBMISSION_FAILED; 
				break;
			default:        
				aTranslatedResult = SCResult_UNKNOWN_RESPONSE; 
				break;
			};
		}
	}
	else
	{
		aTranslatedResult = SCResult_HTTP_ERROR;
	}

	// Client callback
	aWebServices->mSubmitReportPending = gsi_false;
	if (aWebServices->mSubmitReportDataCallback != NULL)
	{
		aWebServices->mSubmitReportDataCallback(aWebServices->mInterface,
			                                theHttpResult,
		                                    aTranslatedResult,
											aWebServices->mSubmitReportUserData);
		aWebServices->mSubmitReportUserData = NULL;
		aWebServices->mSubmitReportDataCallback = NULL;
	}
	GSI_UNUSED(theRequestData);
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
