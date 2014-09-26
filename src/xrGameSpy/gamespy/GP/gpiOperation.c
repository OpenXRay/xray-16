/*
gpiOperation.c
GameSpy Presence SDK 
Dan "Mr. Pants" Schoenblum

Copyright 1999-2007 GameSpy Industries, Inc

devsupport@gamespy.com

***********************************************************************
Please see the GameSpy Presence SDK documentation for more information
**********************************************************************/

//INCLUDES
//////////
#include <stdlib.h>
#include "gpi.h"

//FUNCTIONS
///////////
GPResult
gpiFailedOpCallback(
  GPConnection * connection,
  const GPIOperation * operation
)
{
	GPICallback callback;
	GPIConnection * iconnection = (GPIConnection*)*connection;

	assert(connection != NULL);
	assert(*connection != NULL);
	assert(operation != NULL);

	callback = operation->callback;
	if(callback.callback != NULL)
	{
		// Handle based on operation type.
		//////////////////////////////////
		switch(operation->type)
		{
		case GPI_CONNECT:
		{
			GPConnectResponseArg * arg;
			arg = (GPConnectResponseArg *)gsimalloc(sizeof(GPConnectResponseArg));
			if(arg == NULL)
				Error(connection, GP_MEMORY_ERROR, "Out of memory.");
			memset(arg, 0, sizeof(GPConnectResponseArg));
			arg->result = operation->result;
			if(iconnection->errorCode == GP_NEWUSER_BAD_NICK)
			{
				arg->profile = (GPProfile)iconnection->profileid;
				iconnection->profileid = 0;
			}
			CHECK_RESULT(gpiAddCallback(connection, callback, arg, operation, 0));
			break;
		}
		case GPI_NEW_PROFILE:
		{
			GPNewProfileResponseArg * arg;
			arg = (GPNewProfileResponseArg *)gsimalloc(sizeof(GPNewProfileResponseArg));
			if(arg == NULL)
				Error(connection, GP_MEMORY_ERROR, "Out of memory.");
			memset(arg, 0, sizeof(GPNewProfileResponseArg));
			arg->result = operation->result;
			CHECK_RESULT(gpiAddCallback(connection, callback, arg, operation, 0));
			break;
		}
		case GPI_DELETE_PROFILE:
		{
			GPDeleteProfileResponseArg * arg;
			arg = (GPDeleteProfileResponseArg *)gsimalloc(sizeof(GPDeleteProfileResponseArg));
			if (arg == NULL)
				Error(connection, GP_MEMORY_ERROR, "Out of memory.");
			memset(arg, 0, sizeof(GPDeleteProfileResponseArg));
			arg->result = operation->result;
			CHECK_RESULT(gpiAddCallback(connection, callback, arg, operation, 0));
			break;

		}
		case GPI_GET_INFO:
		{
			GPGetInfoResponseArg * arg;
			arg = (GPGetInfoResponseArg *)gsimalloc(sizeof(GPGetInfoResponseArg));
			if(arg == NULL)
				Error(connection, GP_MEMORY_ERROR, "Out of memory.");
			memset(arg, 0, sizeof(GPGetInfoResponseArg));
			arg->result = operation->result;
			CHECK_RESULT(gpiAddCallback(connection, callback, arg, operation, 0));
			break;
		}
		case GPI_PROFILE_SEARCH:
		{
			GPProfileSearchResponseArg * arg;
			arg = (GPProfileSearchResponseArg *)gsimalloc(sizeof(GPProfileSearchResponseArg));
			if(arg == NULL)
				Error(connection, GP_MEMORY_ERROR, "Out of memory.");
			memset(arg, 0, sizeof(GPProfileSearchResponseArg));
			arg->result = operation->result;
			((GPProfileSearchResponseArg *)arg)->matches = NULL;
			CHECK_RESULT(gpiAddCallback(connection, callback, arg, operation, 0));
			break;
		}
		case GPI_REGISTER_UNIQUENICK:
		{
			GPRegisterUniqueNickResponseArg * arg;
			arg = (GPRegisterUniqueNickResponseArg *)gsimalloc(sizeof(GPRegisterUniqueNickResponseArg));
			if(arg == NULL)
				Error(connection, GP_MEMORY_ERROR, "Out of memory.");
			memset(arg, 0, sizeof(GPRegisterUniqueNickResponseArg));
			arg->result = operation->result;
			CHECK_RESULT(gpiAddCallback(connection, callback, arg, operation, 0));
			break;
		}
		case GPI_REGISTER_CDKEY:
		{
			GPRegisterCdKeyResponseArg * arg;
			arg = (GPRegisterCdKeyResponseArg *)gsimalloc(sizeof(GPRegisterCdKeyResponseArg));
			if(arg == NULL)
				Error(connection, GP_MEMORY_ERROR, "Out of memory.");
			memset(arg, 0, sizeof(GPRegisterCdKeyResponseArg));
			arg->result = operation->result;
			CHECK_RESULT(gpiAddCallback(connection, callback, arg, operation, 0));
			break;
		}
		default:
			assert(0);
		}
	}

	return GP_NO_ERROR;
}

GPResult
gpiAddOperation(
  GPConnection * connection,
  int type,
  void * data,
  GPIOperation ** op,
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	GPIOperation * operation;
	GPIConnection * iconnection = (GPIConnection*)*connection;

	// Create a new operation struct.
	/////////////////////////////////
	operation = (GPIOperation *)gsimalloc(sizeof(GPIOperation));
	if(operation == NULL)
		Error(connection, GP_MEMORY_ERROR, "Out of memory.");

	// Set the data.
	////////////////
	operation->type = type;
	operation->data = data;
	operation->blocking = (GPIBool)blocking;
	operation->state = GPI_START;
	if(type == GPI_CONNECT)
	{
		// Connect is always ID 1.
		//////////////////////////
		operation->id = 1;
	}
	else
	{
		operation->id = iconnection->nextOperationID++;
		if(iconnection->nextOperationID < 2)
			iconnection->nextOperationID = 2;
	}
	operation->result = GP_NO_ERROR;
	operation->callback.callback = callback;
	operation->callback.param = param;

	// Add it to the list.
	//////////////////////
	operation->pnext = iconnection->operationList;
	iconnection->operationList = operation;

	*op = operation;
	return GP_NO_ERROR;
}

void
gpiDestroyOperation(
  GPConnection * connection,
  GPIOperation * operation
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	
	// Search?
	//////////
	if(operation->type == GPI_PROFILE_SEARCH)
	{
		GPISearchData * data = (GPISearchData *)operation->data;

		// One less.
		////////////
		iconnection->numSearches--;
		assert(iconnection->numSearches >= 0);

		// Close the socket.
		////////////////////
		shutdown(data->sock, 2);
		closesocket(data->sock);

		// freeclear the buffers.
		////////////////////
		freeclear(data->outputBuffer.buffer);
		freeclear(data->inputBuffer.buffer);
	}

	// freeclear the data.
	/////////////////
	freeclear(operation->data);

	// freeclear the operation struct.
	/////////////////////////////
	freeclear(operation);
}

void
gpiRemoveOperation(
  GPConnection * connection,
  GPIOperation * operation
)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	GPIOperation * pcurr = iconnection->operationList;
	GPIOperation * pprev = NULL;

	// Go through the list of operations.
	/////////////////////////////////////
	while(pcurr != NULL)
	{
		// Check for a match.
		/////////////////////
		if(pcurr == operation)
		{
			// Update the list.
			///////////////////
			if(pprev == NULL)
				iconnection->operationList = pcurr->pnext;
			else
				pprev->pnext = operation->pnext;

			gpiDestroyOperation(connection, operation);

			return;
		}

		pprev = pcurr;
		pcurr = pcurr->pnext;
	}
}

GPIBool
gpiFindOperationByID(
  const GPConnection * connection,
  GPIOperation ** operation,
  int id
)
{
	GPIOperation * op;
	GPIConnection * iconnection = (GPIConnection*)*connection;

	// Go through the list of operations.
	/////////////////////////////////////
	for(op = iconnection->operationList ; op != NULL ; op = op->pnext)
	{
		// Check the id.
		////////////////
		if(op->id == id)
		{
			// Found it.
			////////////
			if(operation != NULL)
				*operation = op;
			return GPITrue;
		}
	}

	// Didn't find it.
	//////////////////
	if(operation != NULL)
		*operation = NULL;
	return GPIFalse;
}

GPIBool
gpiOperationsAreBlocking(
  const GPConnection * connection
)
{
	GPIOperation * operation;
	GPIConnection * iconnection = (GPIConnection*)*connection;

	// Loop through the operations.
	///////////////////////////////
	for(operation = iconnection->operationList ; operation != NULL ; operation = operation->pnext)
	{
		// Check if it's blocking.
		//////////////////////////
		if((operation->blocking) && (operation->type != GPI_PROFILE_SEARCH))
			return GPITrue;
	}

	// Nothing was blocking.
	////////////////////////
	return GPIFalse;
}

GPResult
gpiProcessOperation(
  GPConnection * connection,
  GPIOperation * operation,
  const char * input
)
{
	GPResult result = GP_NO_ERROR;

	// Check the operation type.
	////////////////////////////
	switch(operation->type)
	{
	case GPI_CONNECT:
		result = gpiProcessConnect(connection, operation, input);
		break;

	case GPI_NEW_PROFILE:
		result = gpiProcessNewProfile(connection, operation, input);
		break;
	
	case GPI_DELETE_PROFILE:
		result = gpiProcessDeleteProfle(connection, operation, input);
		break;

	case GPI_GET_INFO:
		result = gpiProcessGetInfo(connection, operation, input);
		break;

	case GPI_REGISTER_UNIQUENICK:
		result = gpiProcessRegisterUniqueNick(connection, operation, input);
		break;

	case GPI_REGISTER_CDKEY:
		result = gpiProcessRegisterCdKey(connection, operation, input);
		break;

	default:
		gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Misc, GSIDebugLevel_HotError,
			"gpiProcessOperation was passed an operation with an invalid type (%d)\n", operation->type);
		assert(0);
		break;
	}

	if(result != GP_NO_ERROR)
		operation->result = result;
	
	return result;
}

