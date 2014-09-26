///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
#include "sciInterface.h"


/* PRIVATE INTERFACE FUNCTIONS -- SCMAIN.C CONTAINS PUBLIC INTERFACE */


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// This is declared as an extern so it can be overriden when testing
#define SC_SERVICE_URL_FORMAT   "http://%s.comp.pubsvs." GSI_DOMAIN_NAME "/CompetitionService/CompetitionService.asmx"
char scServiceURL[SC_SERVICE_MAX_URL_LEN] = "";


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SCResult sciInterfaceCreate(SCInterface** theInterfaceOut)
{
#ifdef GSI_SC_STATIC_MEM
	static SCInterface gStaticInterface;
#endif

	GS_ASSERT(theInterfaceOut != NULL);

	// Check to see if the availability check has been performed and if it has
	// set the service URL prepended with the gamename
	if (__GSIACResult == GSIACAvailable)
	{
		if (scServiceURL[0] == '\0')
			snprintf(scServiceURL, SC_SERVICE_MAX_URL_LEN, SC_SERVICE_URL_FORMAT, __GSIACGamename);
	}
	else
		return SCResult_NO_AVAILABILITY_CHECK;	

#ifdef GSI_SC_STATIC_MEM
	*theInterfaceOut = &gStaticInterface;
#else
	*theInterfaceOut = (SCInterface*)gsimalloc(sizeof(SCInterface));
	if (*theInterfaceOut == NULL)
	{
		return SCResult_OUT_OF_MEMORY;
	}
#endif

	GS_ASSERT(*theInterfaceOut != NULL);
	memset(*theInterfaceOut, 0, sizeof(SCInterface));
	return SCResult_NO_ERROR;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
SCResult sciInterfaceInit(SCInterface* theInterface)
{
	SCResult anInitResult = SCResult_NO_ERROR;

	GS_ASSERT(theInterface != NULL);

	anInitResult = sciWsInit(&theInterface->mWebServices, theInterface);
	if (anInitResult != SCResult_NO_ERROR)
	{
		return anInitResult;
	}

	memset(theInterface->mSessionId, 0, sizeof(theInterface->mSessionId));
	memset(theInterface->mConnectionId, 0, sizeof(theInterface->mConnectionId));

	theInterface->mInit = gsi_true;
	return SCResult_NO_ERROR;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void sciInterfaceDestroy(SCInterface* theInterface)
{
	GS_ASSERT(theInterface != NULL);
	GS_ASSERT(theInterface->mInit);

	sciWsDestroy(&theInterface->mWebServices);

	memset(theInterface->mSessionId, 0, sizeof(theInterface->mSessionId));
	memset(theInterface->mConnectionId, 0, sizeof(theInterface->mConnectionId));
	theInterface->mSessionId[0] = 0xde;
	theInterface->mSessionId[1] = 0xad;
	theInterface->mConnectionId[0] = 0xde;
	theInterface->mConnectionId[1] = 0xad;

	theInterface->mInit = gsi_false;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void sciInterfaceSetSessionId(SCInterface * theInterface, const char * theSessionId)
{
	GS_ASSERT(theInterface != NULL);

	if (theSessionId == NULL)
		theInterface->mSessionId[0] = '\0';
	else
	{
		GS_ASSERT(strlen(theSessionId) < sizeof(theInterface->mSessionId));
		strcpy((char *)theInterface->mSessionId, theSessionId);
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void sciInterfaceSetConnectionId(SCInterface * theInterface, const char * theConnectionId)
{
	GS_ASSERT(theInterface != NULL);

	if (theConnectionId == NULL)
		theInterface->mConnectionId[0] = '\0';
	else
	{
		GS_ASSERT(strlen(theConnectionId) < sizeof(theInterface->mConnectionId));
		strcpy((char *)theInterface->mConnectionId, theConnectionId);
	}
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
