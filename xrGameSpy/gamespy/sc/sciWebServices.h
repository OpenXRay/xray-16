///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifndef __SCWEBSERVICES_H__
#define __SCWEBSERVICES_H__


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "../common/gsSoap.h"
#include "../common/gsXML.h"
#include "../ghttp/ghttpPost.h"

#include "sci.h"
#include "sciReport.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Web service result codes (must match definitions in StatsReceiver)
typedef enum
{
	SCWsResult_NO_ERROR = 0,
	SCWsResult_REPORT_INVALID,
	SCWsResult_SINGLE_ATTACHMENT_EXPECTED
} SCWsResult;

	
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
typedef struct
{
	SCInterfacePtr          mInterface;
	SCCreateSessionCallback mCreateSessionCallback;
	SCSetReportIntentionCallback   mSetReportIntentionCallback;
	SCSubmitReportCallback  mSubmitReportDataCallback;
	gsi_bool                mSetReportIntentionPending;
	gsi_bool                mCreateSessionPending;
	gsi_bool                mSubmitReportPending;
	void *                  mSetReportIntentionUserData;
	void *                  mCreateSessionUserData;
	void *                  mSubmitReportUserData;
	gsi_u8*                 mSubmitReportData;
	gsi_u32                 mSubmitReportLength;
	gsi_bool                mInit;
} SCWebServices;


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SCResult sciWsInit   (SCWebServices* theWebServices,
                      SCInterfacePtr theInterface);
void     sciWsDestroy(SCWebServices* theWebServices);
void     sciWsThink  (SCWebServices* theWebServices);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SCResult sciWsCreateSession       (SCWebServices *    theWebServices,
								   gsi_u32            theGameId,
								   const GSLoginCertificate * theCertificate,
								   const GSLoginPrivateData * thePrivateData,
                                   SCCreateSessionCallback theCallback,
                                   gsi_time           theTimeoutMs,
								   void *       theUserData);

SCResult sciWsCreateMatchlessSession(SCWebServices *    theWebServices,
								   gsi_u32              theGameId,
								   const GSLoginCertificate * theCertificate,
								   const GSLoginPrivateData * thePrivateData,
								   SCCreateSessionCallback theCallback,
								   gsi_time     theTimeoutMs,
								   void *       theUserData);

void     sciWsCreateSessionCallback(GHTTPResult       theHttpResult,
                                   GSXmlStreamWriter  theRequestData,
                                   GSXmlStreamReader  theResponseData,
                                   void*        theUserData);

SCResult sciWsSetReportIntention  (SCWebServices*     theWebServices,
								   gsi_u32            theGameId,
								   const char *       theSessionId,
								   const char *       theConnectionId,
								   gsi_bool           isAuthoritative,
								   const GSLoginCertificate * theCertificate,
								   const GSLoginPrivateData * thePrivateData,
                                   SCSetReportIntentionCallback theCallback,
                                   gsi_time           theTimeoutMs,
								   void *       theUserData);

void     sciWsSetReportIntentionCallback(GHTTPResult  theHttpResult,
                                   GSXmlStreamWriter  theRequestData,
                                   GSXmlStreamReader  theResponseData,
                                   void*        theUserData);

SCResult sciWsSubmitReport        (SCWebServices*     theWebServices,
								   gsi_u32            theGameId,
								   const char *       theSessionId,
								   const char *       theConnectionId,
								   const SCIReport*   theReport,
								   gsi_bool           isAuthoritative,
                                   const GSLoginCertificate * theCertificate,
								   const GSLoginPrivateData * thePrivateData,
                                   SCSubmitReportCallback theCallback,
                                   gsi_time           theTimeoutMs,
								   void *       theUserData);

void     sciWsSubmitReportCallback(GHTTPResult        theHttpResult,
                                   GSXmlStreamWriter  theRequestData,
                                   GSXmlStreamReader  theResponseData,
                                   void*        theUserData);


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#endif // __SCWEBSERVICES_H__
