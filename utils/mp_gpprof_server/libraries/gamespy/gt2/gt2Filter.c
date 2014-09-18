/*
GameSpy GT2 SDK
Dan "Mr. Pants" Schoenblum
dan@gamespy.com

Copyright 2002 GameSpy Industries, Inc

devsupport@gamespy.com
*/

#include "gt2Filter.h"
#include "gt2Callback.h"
#include "gt2Message.h"
#include "gt2Utility.h"

static int GS_STATIC_CALLBACK gti2SendFiltersCompare
(
	const void * elem1,
	const void * elem2
)
{
	gt2SendFilterCallback * callback1 = (gt2SendFilterCallback *)elem1;
	gt2SendFilterCallback * callback2 = (gt2SendFilterCallback *)elem2;

	if(*callback1 == *callback2)
		return 0;

	return 1;
}

static int GS_STATIC_CALLBACK gti2ReceiveFiltersCompare
(
	const void * elem1,
	const void * elem2
)
{
	gt2ReceiveFilterCallback * callback1 = (gt2ReceiveFilterCallback *)elem1;
	gt2ReceiveFilterCallback * callback2 = (gt2ReceiveFilterCallback *)elem2;

	if(*callback1 == *callback2)
		return 0;

	return 1;
}

GT2Bool gti2AddSendFilter(GT2Connection connection, gt2SendFilterCallback callback)
{
	// Check if we have a send filters list.
	if(!connection->sendFilters)
		return GT2False;

	// Add this callback to the list.
	ArrayAppend(connection->sendFilters, &callback);

	// Return GT2True if it was added.
	return (ArraySearch(connection->sendFilters, &callback, gti2SendFiltersCompare, 0, 0) != NOT_FOUND);
}

GT2Bool gti2AddReceiveFilter(GT2Connection connection, gt2ReceiveFilterCallback callback)
{
	// Check if we have a receive filters list.
	if(!connection->receiveFilters)
		return GT2False;

	// Add this callback to the list.
	ArrayAppend(connection->receiveFilters, &callback);

	// Return GT2True if it was added.
	return (ArraySearch(connection->receiveFilters, &callback, gti2ReceiveFiltersCompare, 0, 0) != NOT_FOUND);
}

void gti2RemoveSendFilter(GT2Connection connection, gt2SendFilterCallback callback)
{
	int index;

	// Check for no filters.
	if(!connection->sendFilters)
		return;

	// check for removing all
	if(!callback)
	{
		// Remove all the filters.
		ArrayClear(connection->sendFilters);
		return;
	}

	// Find it.
	index = ArraySearch(connection->sendFilters, &callback, gti2SendFiltersCompare, 0, 0);
	if(index == NOT_FOUND)
		return;

	// Remove it.
	ArrayRemoveAt(connection->sendFilters, index);
}

void gti2RemoveReceiveFilter(GT2Connection connection, gt2ReceiveFilterCallback callback)
{
	int index;

	// Check for no filters.
	if(!connection->receiveFilters)
		return;

	// check for removing all
	if(!callback)
	{
		// Remove all the filters.
		ArrayClear(connection->receiveFilters);
		return;
	}

	// Find it.
	index = ArraySearch(connection->receiveFilters, &callback, gti2ReceiveFiltersCompare, 0, 0);
	if(index == NOT_FOUND)
		return;

	// Remove it.
	ArrayRemoveAt(connection->receiveFilters, index);
}

GT2Bool gti2FilteredSend(GT2Connection connection, int filterID, const GT2Byte * message, int len, GT2Bool reliable)
{
	int num;

	// Make sure we're connected.
	if(connection->state != GTI2Connected)
		return GT2True;

	// check the message and len
	gti2MessageCheck(&message, &len);

	// Get the number of filters.
	num = ArrayLength(connection->sendFilters);

	// Check if its a valid ID.
	if(filterID < 0)
		return GT2True;
	if(filterID >= num)
		return GT2True;

	// Is it the last one?
	if(filterID == (num - 1))
	{
		// Do the actual send.
		if(!gti2Send(connection, message, len, reliable))
			return GT2False;
	}
	else
	{
		// Filter it.
		if(!gti2SendFilterCallback(connection, ++filterID, message, len, reliable))
			return GT2False;
	}

	return GT2True;
}

GT2Bool gti2FilteredReceive(GT2Connection connection, int filterID, GT2Byte * message, int len, GT2Bool reliable)
{
	int num;

	// Make sure we're connected.
	if(connection->state != GTI2Connected)
		return GT2True;

	// Get the number of filters.
	num = ArrayLength(connection->receiveFilters);

	// Check if its a valid ID.
	if(filterID < 0)
		return GT2True;
	if(filterID >= num)
		return GT2True;

	// Is it the last one?
	if(filterID == (num - 1))
	{
		// call the callback
		if(!gti2ReceivedCallback(connection, message, len, reliable))
			return GT2False;
	}
	else
	{
		// Filter it.
		if(!gti2ReceiveFilterCallback(connection, ++filterID, message, len, reliable))
			return GT2False;
	}

	return GT2True;
}
