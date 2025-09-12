/*
gpiPS3.c
GameSpy Presence SDK 

devsupport@gamespy.com

***********************************************************************
Please see the GameSpy Presence SDK documentation for more information
**********************************************************************/

//INCLUDES
//////////
#include <stdlib.h>
#include <string.h>
#include "gpi.h"

#ifdef _PS3
//GLOBALS
//////////
uint8_t	gpi_np_pool[SCE_NP_MIN_POOL_SIZE];
SceNpCommunicationId gpi_communication_id = {
	{'N','P','X','S','0','0','0','0','5'},
	'\0',
	0,
	0
};

//FUNCTIONS
///////////
int gpiNpBasicCallback(
  int event, 
  int retCode, 
  uint32_t reqId, 
  void *arg
)
{
	// No-op - can ignore any events
	////////////////////////////////
	return 0;
}

GPResult gpiInitializeNpBasic(
  GPConnection * connection
)
{
	int ret = 0;
    GPIConnection * iconnection = (GPIConnection*)*connection;

    iconnection->npInitialized = gsi_true;

	// Initial NP init - after this we wait for status to get to online
	////////////////////////////////////////////////////////////////////
	ret = sceNpInit(SCE_NP_MIN_POOL_SIZE, gpi_np_pool);

    if (ret == SCE_NP_ERROR_ALREADY_INITIALIZED)
    {
        // If already initialized - DO NOT terminate after sync (game might need it)
        ////////////////////////////////////////////////////////////////////////////
        iconnection->npBasicGameInitialized = gsi_true;
    }
	else if (ret < 0) 
	{
		iconnection->npBasicGameInitialized = gsi_true;
        gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
            "gpiInitializeNpBasic: sceNpInit() failed, NP-functionality disabled. ret = 0x%x\n", ret);	
        return GP_MISC_ERROR;
	}
    else
        iconnection->npBasicGameInitialized = gsi_false; //GP initialized, so destroy after complete

	return GP_NO_ERROR;
}

// Freeing up transaction list darray
void gpiNpTransactionListFree(void *element)
{
    npIdLookupTrans *aTrans = (npIdLookupTrans *)element;
    freeclear(aTrans->npIdForAdd);
}


GPResult gpiCheckNpStatus(
  GPConnection * connection
)
{
	int ret = 0;
	int status = SCE_NP_MANAGER_STATUS_OFFLINE;
    SceNpId npId;
	GPIConnection * iconnection = (GPIConnection*)*connection;

	// Get NP status
	////////////////
	ret = sceNpManagerGetStatus(&status);
	if (ret < 0) 
	{
		gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
			"gpiCheckNpStatus: sceNpGetStatus() failed. ret = 0x%x\n", ret);	
	}
	gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_Comment,
		"gpiCheckNpStatus: sceNpGetStatus - status = %d\n", status);	


	// If NP status != online after the timeout period, stop syncing 
	////////////////////////////////////////////////////////////////
	if (status != SCE_NP_MANAGER_STATUS_ONLINE && (current_time() - iconnection->loginTime > GPI_NP_STATUS_TIMEOUT))
	{
		gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
			"gpiCheckNpStatus: NP Status not online - timed out\n");	
		
		// Flag to stop the sync process
		////////////////////////////////
		iconnection->npPerformBuddySync = gsi_false;
        iconnection->npPerformBlockSync = gsi_false;

		return GP_MISC_ERROR;
	}

	// Once status is online, finish NP init
	////////////////////////////////////////
	if (status == SCE_NP_MANAGER_STATUS_ONLINE)
	{
		iconnection->loginTime = current_time();

        // Note - we ignore error messages here - if something fails we really don't care
		/////////////////////////////////////////////////////////////////////////////////
        if (!iconnection->npBasicGameInitialized)
        {
		    ret = sceNpBasicInit(); //obsolete?
		    if (ret < 0) 
		    {
			    gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
				    "gpiCheckNpStatus: sceNpBasicInit() failed. ret = 0x%x\n", ret);	
		    }

		    ret = sceNpBasicRegisterHandler(&gpi_communication_id, gpiNpBasicCallback, NULL);
		    if (ret < 0) 
		    {
			    gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
				    "gpiCheckNpStatus: sceNpBasicRegisterHandler() failed. ret = 0x%x\n", ret);
		    }
        }

        ret = sceNpLookupInit();
        if (ret == SCE_NP_COMMUNITY_ERROR_ALREADY_INITIALIZED)
        {
            // If already initialized - DO NOT terminate after GP destroy (game might need it)
            //////////////////////////////////////////////////////////////////////////////////
            iconnection->npLookupGameInitialized = gsi_true;
        }
        else if (ret < 0) 
        {
            gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
                "gpiCheckNpStatus: sceNpLookupInit() failed. ret = 0x%x\n", ret);    
            iconnection->npLookupGameInitialized = gsi_true;
        }
        else
            iconnection->npLookupGameInitialized = gsi_false;

        // Regardless of game, create a title context id for GP to use for lookups
        ///////////////////////////////////////////////////////////////////////////
        ret = sceNpManagerGetNpId(&npId);
        if (ret < 0) 
        {
            gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
                "gpiCheckNpStatus: sceNpManagerGetNpId() failed. ret = 0x%x\n", ret);  
        }

        ret = sceNpLookupCreateTitleCtx(&gpi_communication_id, &npId);
        if (ret < 0) 
        {
            gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
                "gpiCheckNpStatus: sceNpLookupCreateTitleCtx() failed. ret = 0x%x\n", ret);  
        }

        iconnection->npLookupTitleCtxId = ret;

		// Mark status retrieval completed
		//////////////////////////////////
		iconnection->npStatusRetrieved = gsi_true;
        gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_Comment,
            "gpiCheckNpStatus: NP is now initialized with status.\n");	

        iconnection->npTransactionList = ArrayNew(sizeof(npIdLookupTrans), 1, gpiNpTransactionListFree);
        if (!iconnection->npTransactionList)
            Error(connection, GP_MEMORY_ERROR, "Out of memory.");
	}

	return GP_NO_ERROR;
}

GPResult gpiDestroyNpBasic(
  GPConnection * connection
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;	

    // Explicitly destroy title context we used for lookup
    //////////////////////////////////////////////////////
    if (iconnection->npLookupTitleCtxId >= 0)
        sceNpLookupDestroyTitleCtx(iconnection->npLookupTitleCtxId);

    // Do not destroy NpLookup or NpBasic if Game is using it
    /////////////////////////////////////////////////////////
    if (!iconnection->npLookupGameInitialized)
        sceNpLookupTerm();

    if (!iconnection->npBasicGameInitialized)
    {
	    sceNpBasicUnregisterHandler();

	    // Obsolete?
	    sceNpBasicTerm();

        sceNpTerm();
    }

    // Free up transaction list used for NP lookups
    ///////////////////////////////////////////////
    if (iconnection->npTransactionList)
        ArrayFree(iconnection->npTransactionList);

	iconnection->npInitialized = gsi_false;
    iconnection->npStatusRetrieved = gsi_false;

	return GP_NO_ERROR;
}

GPResult gpiSyncNpBuddies(
  GPConnection * connection
)
{
	int ret; 
	SceNpId npId;	//Buffer to store friend list entry's NP ID
	gsi_u32 i, count = 0;
	GPIConnection * iconnection = (GPIConnection*)*connection;


	// Flag sync as complete so we don't do it more than once per login
	////////////////////////////////////////////////////////////////////
	iconnection->npPerformBuddySync = gsi_false;

	// Get buddy count
	///////////////////
	ret = sceNpBasicGetFriendListEntryCount(&count);
	if ( ret < 0 ) 
	{
		gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
			"PS3BuddySync: Failed to get NP friend list count\n");
	}

	// Loop through each buddy, check for existence of GSID account
	///////////////////////////////////////////////////////////////
	for (i = 0; i < count; i++) 
	{
		memset(&npId, 0x00, sizeof(npId));
		ret = sceNpBasicGetFriendListEntry(i, &npId);
		if (ret < 0) 
		{
			gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
				"PS3BuddySync: Failed to get NP friend entry #%d\n", i);
			return GP_MISC_ERROR;
		}

		gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_Comment,
			"PS3BuddySync: NP friend entry #%d, npid = %s. Queueing Search.\n", i, npId.handle.data);

		gpiProfileSearchUniquenick(connection, npId.handle.data, &iconnection->namespaceID, 
			1, GP_NON_BLOCKING, (GPCallback)gpiSyncNpBuddiesCallback, NULL);
	}

	return GP_NO_ERROR;
}

void gpiSyncNpBuddiesCallback(
  GPConnection * pconnection, 
  GPProfileSearchResponseArg * arg, 
  void * param
)
{
	if(arg->result == GP_NO_ERROR)
	{
		if(arg->numMatches == 1)
		{
            // Check if already a buddy
            ////////////////////////////
		    if (!gpIsBuddy(pconnection, arg->matches[0].profile))
            {
				gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_Comment,
					"PS3BuddySync: NP Buddy \"%s\" found in namespace %d. Sending Request.\n", 
					arg->matches[0].uniquenick, arg->matches[0].namespaceID);

                // Send the add request
                ////////////////////////
				gpSendBuddyRequest(pconnection, arg->matches[0].profile, _T("PS3 Buddy Sync"));
			}
			else
				gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_Comment,
					"PS3BuddySync: \"%s\" is already a buddy\n", arg->matches[0].uniquenick);
		}
		else
			gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_Comment,
				"PS3BuddySync: No suitable match found\n");
	}
	else
		gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
			"PS3BuddySync: Buddy Search FAILED!\n");

	GSI_UNUSED(pconnection);
	GSI_UNUSED(param);
}

GPResult gpiSyncNpBlockList(
  GPConnection * connection
)
{
    int ret; 
    SceNpId npId;	//Buffer to store block list entry's NP ID
    gsi_u32 i, count = 0;
    GPIConnection * iconnection = (GPIConnection*)*connection;


    // Flag sync as complete so we don't do it more than once per login
    ////////////////////////////////////////////////////////////////////
    iconnection->npPerformBlockSync = gsi_false;

    // Get block list count
    ///////////////////////
    ret = sceNpBasicGetBlockListEntryCount(&count);
    if ( ret < 0 ) 
    {
        gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
            "PS3BlockSync: Failed to get NP block list count\n");
    }

    // Loop through each entry, check for existence of GSID account
    ///////////////////////////////////////////////////////////////
    for (i = 0; i < count; i++) 
    {
        memset(&npId, 0x00, sizeof(npId));
        ret = sceNpBasicGetBlockListEntry(i, &npId);
        if (ret < 0) 
        {
            gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
                "PS3BlockSync: Failed to get NP block entry #%d\n", i);
            return GP_MISC_ERROR;
        }

        gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_Comment,
            "PS3BlockSync: NP block entry #%d, npid = %s. Queueing Search.\n", i, npId.handle.data);

        gpiProfileSearchUniquenick(connection, npId.handle.data, &iconnection->namespaceID, 
            1, GP_NON_BLOCKING, (GPCallback)gpiSyncNpBlockListCallback, NULL);
    }

    return GP_NO_ERROR;
}

void gpiSyncNpBlockListCallback(
  GPConnection * pconnection, 
  GPProfileSearchResponseArg * arg, 
  void * param
)
{
	GPIProfile * pProfile;
    GPIConnection * iconnection = (GPIConnection*)*pconnection;

    if(arg->result == GP_NO_ERROR)
    {
        if(arg->numMatches == 1)
        {
            // Check if already blocked
            ////////////////////////////
            if(!gpiGetProfile(pconnection, arg->matches[0].profile, &pProfile) || !pProfile->blocked)
            {
                gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_Comment,
                    "PS3BlockSync: NP Block Entry \"%s\" found in namespace %d. Adding to BlockedList.\n", 
                    arg->matches[0].uniquenick, arg->matches[0].namespaceID);

                // Add to GP Blocked List - set lock to make sure we dont try to add to NP list
                ///////////////////////////////////////////////////////////////////////////////
                iconnection->npSyncLock = gsi_true;
                gpiAddToBlockedList(pconnection, arg->matches[0].profile);
                iconnection->npSyncLock = gsi_false;
            }
            else
                gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_Comment,
                    "PS3BlockSync: \"%s\" is already blocked\n", arg->matches[0].uniquenick);
        }
        else
            gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_Comment,
                "PS3BlockSync: No suitable match found\n");
    }
    else
        gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
        "PS3BlockSync: Block Entry Search FAILED!\n");

    GSI_UNUSED(param);
}

GPResult gpiAddToNpBlockList(
  GPConnection * connection, 
  int profileid
)
{
    GPIConnection * iconnection = (GPIConnection*)*connection;
    // TODO: consider developer method for cache input in order to check HDD cache?

    // If NP status not resolved, don't bother with lookup
    ///////////////////////////////////////////////////////
    if (!iconnection->npTransactionList || iconnection->npLookupTitleCtxId < 0)
    {
        gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
            "PS3AddToNpBlockList: Cancelling add - NP status not yet resolved.\n");        
        return GP_NO_ERROR;
    }

    // Do an info lookup to find out if this player has an NP account.
    /////////////////////////////////////////////////////////////////
    gpiGetInfo(connection, profileid, GP_CHECK_CACHE, GP_NON_BLOCKING, 
        (GPCallback)gpiAddToNpBlockListInfoCallback, NULL);

    return GP_NO_ERROR;
}

void gpiAddToNpBlockListInfoCallback(
  GPConnection * pconnection, 
  GPGetInfoResponseArg * arg, 
  void * param
)
{
    SceNpOnlineId onlineId;
    int ret;
    npIdLookupTrans transaction;
    GPIConnection * iconnection = (GPIConnection*)*pconnection;
#ifdef GSI_UNICODE
    char asciiUniquenick[GP_UNIQUENICK_LEN];
#endif

    if(arg->result == GP_NO_ERROR)
    {
        // Make sure its a PS3 uniquenick (e.g. we have the uniquenick)
        ///////////////////////////////////////////////////////////////
        if (_tcslen(arg->uniquenick) != 0)
        {
            memset(&onlineId, 0, sizeof(onlineId));

#ifdef GSI_UNICODE
            UCS2ToAsciiString(arg->uniquenick, (char*)asciiUniquenick);
            strncpy(onlineId.data, asciiUniquenick, SCE_NET_NP_ONLINEID_MAX_LENGTH);
#else
            strncpy(onlineId.data, arg->uniquenick, SCE_NET_NP_ONLINEID_MAX_LENGTH);
#endif

            if (ArrayLength(iconnection->npTransactionList) < GPI_NP_NUM_TRANSACTIONS)
            {
                ret = sceNpLookupCreateTransactionCtx(iconnection->npLookupTitleCtxId);
                if (ret < 0)
                {
                    gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
                        "PS3AddToNpBlockList: sceNpLookupCreateTransactionCtx() failed. ret = 0x%x\n", ret);  
                }
                else
                {
                    transaction.npIdForAdd = (SceNpId*)gsimalloc(sizeof(SceNpId));
                    if(transaction.npIdForAdd == NULL)
                    {
                        sceNpLookupDestroyTransactionCtx(ret);
                        gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
                            "PS3AddToNpBlockList: Out of memory.\n");  
                        return;
                    }
                    transaction.npTransId = ret;
                    transaction.npLookupDone = gsi_false;
                    ArrayAppend(iconnection->npTransactionList, &transaction);

                    // Perform NP lookup to get the NpId
                    /////////////////////////////////////
                    ret = sceNpLookupNpIdAsync(transaction.npTransId, &onlineId, 
                        transaction.npIdForAdd, 0, NULL);
                    if (ret < 0) 
                    {
                        gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
                            "PS3AddToNpBlockList: sceNpLookupNpIdAsync() failed. ret = 0x%x\n", ret);  
                    } 
                }
            }
            else
            {
                // Can only have a max of 32 simultaneous transactions (based on PS3 lib)
                /////////////////////////////////////////////////////////////////////////
                gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_WarmError,
                    "PS3AddToNpBlockList: Transactions limit reached for np lookups\n");  
            }
        }
        else
            gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
                "PS3AddToNpBlockList: Profile [%d] does not have a uniquenick in namespace %d!\n",
                arg->profile, iconnection->namespaceID);
    }
    else
        gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
            "PS3AddToNpBlockList: Player Info lookup FAILED!\n");

    GSI_UNUSED(pconnection);
    GSI_UNUSED(param);
}

GPResult gpiProcessNp(GPConnection * connection)
{
    int i, ret=0;
    GPIConnection * iconnection = (GPIConnection*)*connection;
    npIdLookupTrans * transaction;

    // Check for uninitialized transaction darray
    //////////////////////////////////////////////
    if (!iconnection->npTransactionList)
        return GP_NO_ERROR;

    // Need to process Sysutil for the Async lookups
    /////////////////////////////////////////////////
    if (ArrayLength(iconnection->npTransactionList) > 0)
        cellSysutilCheckCallback();

    // Loop through all current transactions, check if complete
    ///////////////////////////////////////////////////////////
    for (i=0; i < ArrayLength(iconnection->npTransactionList); i++)
    {
        // Grab next transaction in the list
        /////////////////////////////////////    
        transaction = (npIdLookupTrans *)ArrayNth(iconnection->npTransactionList, i);

        if (!transaction->npLookupDone)
        {
            if (sceNpLookupPollAsync(transaction->npTransId, &ret)==0)
                transaction->npLookupDone = gsi_true;
        }
        else
        {
            if (ret<0)
            {
                gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
                    "PS3AddToNpBlockList: sceNpLookupWaitAsync. ret = 0x%x\n", ret);
                if (ret == (int)SCE_NP_COMMUNITY_SERVER_ERROR_NO_SUCH_USER_NPID)
                {
                    gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
                        "PS3AddToNpBlockList: Player '%s' is not an NP user.\n", 
                        transaction->npIdForAdd->handle.data);
                }
            }
            else
            {
                // Found an NpId, try to add
                /////////////////////////////                 
                ret = sceNpBasicAddBlockListEntry(transaction->npIdForAdd);
                if (ret == (int)SCE_NP_BASIC_ERROR_BUSY)
                {
                    // Oh nice, NP is too busy to help us.... keep on trying
                    /////////////////////////////////////////////////////////                         
                    gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_Comment,
                        "PS3AddToNpBlockList: SCE_NP_BASIC_ERROR_BUSY. continue trying to add to NP\n"); 
                    return GP_NO_ERROR;
                }
                else if ( ret < 0 ) 
                {
                    gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
                        "PS3AddToNpBlockList: sceNpBasicAddBlockListEntry() failed. ret = 0x%x\n", ret); 
                }                
                else
                {
                    gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_Comment,
                        "PS3AddToNpBlockList: Player '%s' added to NP Block list.\n", 
                        transaction->npIdForAdd->handle.data); 
                }
            }

            ret = sceNpLookupDestroyTransactionCtx(transaction->npTransId);
            if (ret<0)
            {
                gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
                    "PS3AddToNpBlockList: sceNpLookupDestroyTransactionCtx() failed. ret = 0x%x\n", ret); 
            }

            // Delete Transaction when its complete
            ////////////////////////////////////////
            ArrayDeleteAt(iconnection->npTransactionList, i);
        }
    }

    return GP_NO_ERROR;
}

#endif
