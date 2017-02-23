#include "gpi.h"

void gpiStatusInfoKeyFree(void *element)
{
	GPIKey *aKey = (GPIKey *)element;
	freeclear(aKey->keyName);
	freeclear(aKey->keyValue);
}

GPResult gpiStatusInfoKeysInit(GPConnection * connection)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	
	iconnection->extendedInfoKeys = ArrayNew(sizeof(GPIKey), GPI_INITIAL_NUM_KEYS, gpiStatusInfoKeyFree);
	if(!iconnection->extendedInfoKeys)
		Error(connection, GP_MEMORY_ERROR, "Out of memory.");

	return GP_NO_ERROR;
}

void gpiStatusInfoKeysDestroy(GPConnection * connection)
{
	GPIConnection * iconnection = (GPIConnection*)*connection;
	if (iconnection->extendedInfoKeys)
	{
		ArrayFree(iconnection->extendedInfoKeys);
		iconnection->extendedInfoKeys = NULL;
	}
}

int gpiStatusInfoKeyCompFunc(const void *elem1, const void *elem2)
{
	GPIKey *key1 = (GPIKey *)elem1, 
		   *key2 = (GPIKey *)elem2;
	return strcmp(key1->keyName, key2->keyName);
}

GPResult gpiStatusInfoAddKey(GPConnection *connection, DArray keys, const char *theKeyName, const char *theKeyValue)
{
	GPIKey aKey;
	GS_ASSERT(keys);
	GS_ASSERT(theKeyName);
	GS_ASSERT(theKeyValue);

	if (!theKeyName)
		Error(connection, GP_PARAMETER_ERROR, "Invalid key name");
	if (!theKeyValue)
		Error(connection, GP_PARAMETER_ERROR, "Invalid key value");

	aKey.keyName = goastrdup(theKeyName);
	aKey.keyValue = goastrdup(theKeyValue);

	ArrayInsertSorted(keys, &aKey, gpiStatusInfoKeyCompFunc);

	return GP_NO_ERROR;
}

GPResult gpiStatusInfoDelKey(GPConnection *connection, DArray keys, const char *keyName)
{
	GPIKey aKey;
	int anIndex;
	GS_ASSERT(keys);
	GS_ASSERT(keyName);
	
	if (!keyName)
		Error(connection, GP_PARAMETER_ERROR, "Invalid key name");

	aKey.keyName = goastrdup(keyName);
	anIndex = ArraySearch(keys, &aKey, gpiStatusInfoKeyCompFunc, 0, 1);
	if (anIndex != NOT_FOUND)
	{
		ArrayDeleteAt(keys, anIndex);
	}
	
	freeclear(aKey.keyName);
	return GP_NO_ERROR;
}

GPResult gpiStatusInfoSetKey(GPConnection *connection, DArray keys, const char *keyName, const char *newKeyValue)
{
	GPIKey aKey;
	int anIndex;
	GS_ASSERT(keys);
	GS_ASSERT(keyName);

	if (!keyName)
		Error(connection, GP_PARAMETER_ERROR, "Invalid key name");

	aKey.keyName = goastrdup(keyName);
	anIndex = ArraySearch(keys, &aKey, gpiStatusInfoKeyCompFunc, 0, 1);
	if (anIndex != NOT_FOUND)
	{
		GPIKey *aKeyFound = (GPIKey *)ArrayNth(keys, anIndex);
		gsifree(aKeyFound->keyValue);
		aKeyFound->keyValue = goastrdup(newKeyValue);
	}
	freeclear(aKey.keyName);
	return GP_NO_ERROR;
}

GPResult gpiStatusInfoGetKey(GPConnection *connection, DArray keys, const char *keyName, char **keyValue)
{
	GPIKey aKey;
	int anIndex;
	GS_ASSERT(keys);
	GS_ASSERT(keyName);
	
	if (!keyName)
		Error(connection, GP_PARAMETER_ERROR, "Invalid key name");

	aKey.keyName = goastrdup(keyName);
	anIndex = ArraySearch(keys, &aKey, gpiStatusInfoKeyCompFunc, 0, 1);
	if (anIndex != NOT_FOUND)
	{
		GPIKey *aKeyFound = (GPIKey *)ArrayNth(keys, anIndex);
		*keyValue = goastrdup(aKeyFound->keyValue);
	}
	freeclear(aKey.keyName);
	return GP_NO_ERROR;
}

GPResult gpiStatusInfoCheckKey(GPConnection *connection, DArray keys, const char *keyName, char **keyValue)
{
	GPIKey aKey;
	int anIndex;
	GS_ASSERT(keys);
	GS_ASSERT(keyName);

	if (!keyName)
		Error(connection, GP_PARAMETER_ERROR, "Invalid key name");

	aKey.keyName = goastrdup(keyName);
	anIndex = ArraySearch(keys, &aKey, gpiStatusInfoKeyCompFunc, 0, 1);
	if (anIndex != NOT_FOUND)
	{
		GPIKey *aKeyFound = (GPIKey *)ArrayNth(keys, anIndex);
		*keyValue = aKeyFound->keyValue;
	}
	freeclear(aKey.keyName);
	return GP_NO_ERROR;
}

GPResult gpiSaveKeysToBuffer(GPConnection *connection, char **buffer)
{
	GPIConnection *iconnection = (GPIConnection *)*connection;
	char *tempPoint;
	int sizeKeys = 0, i, bytesWritten;
	int base64KeyNameLen, base64KeyValLen;
	int aLength = ArrayLength(iconnection->extendedInfoKeys);
	
	char keysHeader[64];
	sprintf(keysHeader, "\\keys\\%d", aLength);
	
	// figure out the size of the buffer to allocate
	// by adding up the key value pairs with backslashes
	for (i = 0; i < aLength; i++)
	{
		GPIKey *aKey = (GPIKey *)ArrayNth(iconnection->extendedInfoKeys, i);
		if (strlen(aKey->keyName) % 3 != 0)
			base64KeyNameLen = (int)(strlen(aKey->keyName) * 4 / 3) + (int)(4 - (strlen(aKey->keyName) % 3));
		else 
			base64KeyNameLen = (int)(strlen(aKey->keyName) * 4 / 3);
		if (strlen(aKey->keyValue) % 3 != 0)
			base64KeyValLen= (int)(strlen(aKey->keyValue) * 4 / 3) + (int)(4 - (strlen(aKey->keyValue) % 3));
		else 
			base64KeyValLen = (int)(strlen(aKey->keyValue) * 4 / 3);
		sizeKeys += 1 + base64KeyNameLen + 1 + base64KeyValLen;
	}
	*buffer = (char *)gsimalloc(strlen(keysHeader) + (size_t)sizeKeys + 1);
	if (*buffer == NULL)
	{
		gsDebugFormat(GSIDebugCat_GP, GSIDebugType_Memory, GSIDebugLevel_HotError, "gpiSaveKeysToBuffer: buffer Out of memory.");
		Error(connection, GP_MEMORY_ERROR, "Out of memory.");
	}
	bytesWritten = sprintf(*buffer, keysHeader);
	tempPoint = *buffer + bytesWritten;
	for (i = 0; i < aLength; i++)
	{
		GPIKey *aKey = (GPIKey *)ArrayNth(iconnection->extendedInfoKeys, i);
		strcat(tempPoint, "\\");
		tempPoint++;
		B64Encode(aKey->keyName, tempPoint, (int)strlen(aKey->keyName), 2);
		if (strlen(aKey->keyName) % 3 != 0)
			tempPoint+= (int)(strlen(aKey->keyName) * 4 / 3) + (4 - (strlen(aKey->keyName) % 3));
		else 
			tempPoint+= (int)(strlen(aKey->keyName) * 4 / 3);
		strcat(tempPoint, "\\");
		tempPoint++;
		B64Encode(aKey->keyValue, tempPoint, (int)strlen(aKey->keyValue), 2);
		if (strlen(aKey->keyValue) % 3 != 0)
			tempPoint+= (int)(strlen(aKey->keyValue) * 4 / 3) + (4 - (strlen(aKey->keyValue) % 3));
		else 
			tempPoint+= (int)(strlen(aKey->keyValue) * 4 / 3);
	}
	return GP_NO_ERROR;
}
