/*
gp.c
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
#include <string.h>
#include "gpi.h"

//FUNCTIONS
///////////
GPResult gpInitialize(
  GPConnection * connection,
  int productID,
  int namespaceID,
  int partnerID
)
{
	// Check if the backend is available.
	/////////////////////////////////////
	if(__GSIACResult != GSIACAvailable)
		return GP_PARAMETER_ERROR;

	// Error check.
	///////////////
	if(connection == NULL)
		return GP_PARAMETER_ERROR;

	return gpiInitialize(connection, productID, namespaceID, partnerID);
}

void gpDestroy(
  GPConnection * connection
)
{
	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return;

	gpiDestroy(connection);
}

GPResult gpEnable(
  GPConnection * connection, 
  GPEnum state
)
{
	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	return gpiEnable(connection, state);
}

GPResult gpDisable(
  GPConnection * connection, 
  GPEnum state
)
{
	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	return gpiDisable(connection, state);
}

GPResult gpProcess(
  GPConnection * connection
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection*)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
		return GP_NO_ERROR;
	
	return gpiProcess(connection, 0);
}

GPResult gpSetCallback(
  GPConnection * connection,
  GPEnum func,
  GPCallback callback,
  void * param
)
{
	GPIConnection * iconnection;
	int index;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Find which callback.
	///////////////////////
	index = func;
	if((index < 0) || (index >= GPI_NUM_CALLBACKS))
		Error(connection, GP_PARAMETER_ERROR, "Invalid func.");

	// Set the info.
	////////////////
	iconnection->callbacks[index].callback = callback;
	iconnection->callbacks[index].param = param;

	return GP_NO_ERROR;
}

GPResult gpConnectA(
  GPConnection * connection,
  const char nick[GP_NICK_LEN],
  const char email[GP_EMAIL_LEN],
  const char password[GP_PASSWORD_LEN],
  GPEnum firewall,
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;
	if((nick == NULL) || (nick[0] == '\0'))
		return GP_PARAMETER_ERROR;
	if((email == NULL) || (email[0] == '\0'))
		return GP_PARAMETER_ERROR;
	if((password == NULL) || (password[0] == '\0'))
		return GP_PARAMETER_ERROR;

	// Check for no callback.
	/////////////////////////
	if(callback == NULL)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");

	// Check for simulation mode.
	/////////////////////////////
	iconnection = (GPIConnection*)*connection;
	if(iconnection->simulation)
	{
		GPConnectResponseArg arg;
		memset(&arg, 0, sizeof(arg));
		callback(connection, &arg, param);
		return GP_NO_ERROR;
	}

	// Do it.
	/////////
	return gpiConnect(connection, nick, "", email, password, "", "", NULL, firewall, GPIFalse, blocking, callback, param);
}
#ifdef GSI_UNICODE
GPResult gpConnectW(
  GPConnection * connection,
  const unsigned short nick[GP_NICK_LEN],
  const unsigned short email[GP_EMAIL_LEN],
  const unsigned short password[GP_PASSWORD_LEN],
  GPEnum firewall,
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	char nick_A[GP_NICK_LEN];
	char email_A[GP_EMAIL_LEN];
	char password_A[GP_PASSWORD_LEN];

	UCS2ToAsciiString(nick, nick_A);
	UCS2ToAsciiString(email, email_A);
	UCS2ToAsciiString(password, password_A);

	return gpConnectA(connection, nick_A, email_A, password_A, firewall, blocking, callback, param);
}
#endif

GPResult gpConnectNewUserA(
  GPConnection * connection,
  const char nick[GP_NICK_LEN],
  const char uniquenick[GP_UNIQUENICK_LEN],
  const char email[GP_EMAIL_LEN],
  const char password[GP_PASSWORD_LEN],
  const char cdkey[GP_CDKEY_LEN],
  GPEnum firewall,
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;
	if((nick == NULL) || (nick[0] == '\0'))
		return GP_PARAMETER_ERROR;
	if(uniquenick == NULL)
		uniquenick = "";
	if((email == NULL) || (email[0] == '\0'))
		return GP_PARAMETER_ERROR;
	if((password == NULL) || (password[0] == '\0'))
		return GP_PARAMETER_ERROR;
	if(cdkey && (cdkey[0] == '\0'))
		cdkey = NULL;

	// Check for no callback.
	/////////////////////////
	if(callback == NULL)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");
	
	// Check the length of the nick.
	////////////////////////////////
	if(strlen(nick) >= GP_NICK_LEN)
		Error(connection, GP_PARAMETER_ERROR, "Nick too long.");

	// Check the length of the uniquenick.
	//////////////////////////////////////
	if(strlen(uniquenick) >= GP_UNIQUENICK_LEN)
		Error(connection, GP_PARAMETER_ERROR, "Uniquenick too long.");

	// Check the length of the email.
	/////////////////////////////////
	if(strlen(email) >= GP_EMAIL_LEN)
		Error(connection, GP_PARAMETER_ERROR, "Email too long.");

	// Check the length of the password.
	////////////////////////////////////
	if(strlen(password) >= GP_PASSWORD_LEN)
		Error(connection, GP_PARAMETER_ERROR, "Password too long.");

	// Check for simulation mode.
	/////////////////////////////
	iconnection = (GPIConnection*)*connection;
	if(iconnection->simulation)
	{
		GPConnectResponseArg arg;
		memset(&arg, 0, sizeof(arg));
		callback(connection, &arg, param);
		return GP_NO_ERROR;
	}

	// Do it.
	/////////
	return gpiConnect(connection, nick, uniquenick, email, password, "", "", cdkey, firewall, GPITrue, blocking, callback, param);
}
#ifdef GSI_UNICODE
GPResult gpConnectNewUserW(
  GPConnection * connection,
  const unsigned short nick[GP_NICK_LEN],
  const unsigned short uniquenick[GP_UNIQUENICK_LEN],
  const unsigned short email[GP_EMAIL_LEN],
  const unsigned short password[GP_PASSWORD_LEN],
  const unsigned short cdkey[GP_CDKEY_LEN],
  GPEnum firewall,
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	char nick_A[GP_NICK_LEN];
	char uniquenick_A[GP_UNIQUENICK_LEN];
	char email_A[GP_NICK_LEN];
	char password_A[GP_NICK_LEN];
	char cdkey_A[GP_CDKEY_LEN];

	UCS2ToAsciiString(nick, nick_A);
	UCS2ToAsciiString(uniquenick, uniquenick_A);
	UCS2ToAsciiString(email, email_A);
	UCS2ToAsciiString(password, password_A);
	UCS2ToAsciiString(cdkey, cdkey_A);

	return gpConnectNewUserA(connection, nick_A, uniquenick_A, email_A, password_A, cdkey_A, firewall, blocking, callback, param);
}
#endif

GPResult gpConnectUniqueNickA(
  GPConnection * connection,
  const char uniquenick[GP_UNIQUENICK_LEN],
  const char password[GP_PASSWORD_LEN],
  GPEnum firewall,
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;
	if((uniquenick == NULL) || (uniquenick[0] == '\0'))
		return GP_PARAMETER_ERROR;
	if((password == NULL) || (password[0] == '\0'))
		return GP_PARAMETER_ERROR;

	// Check for no callback.
	/////////////////////////
	if(callback == NULL)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");

	// Check for simulation mode.
	/////////////////////////////
	iconnection = (GPIConnection*)*connection;
	if(iconnection->simulation)
	{
		GPConnectResponseArg arg;
		memset(&arg, 0, sizeof(arg));
		callback(connection, &arg, param);
		return GP_NO_ERROR;
	}

	// Do it.
	/////////
	return gpiConnect(connection, "", uniquenick, "", password, "", "", NULL, firewall, GPIFalse, blocking, callback, param);
}
#ifdef GSI_UNICODE
GPResult gpConnectUniqueNickW(
  GPConnection * connection,
  const unsigned short uniquenick[GP_UNIQUENICK_LEN],
  const unsigned short password[GP_PASSWORD_LEN],
  GPEnum firewall,
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	char uniquenick_A[GP_UNIQUENICK_LEN];
	char password_A[GP_NICK_LEN];

	UCS2ToAsciiString(uniquenick, uniquenick_A);
	UCS2ToAsciiString(password, password_A);

	return gpConnectUniqueNickA(connection, uniquenick_A, password_A, firewall, blocking, callback, param);
}
#endif

GPResult gpConnectPreAuthenticatedA
(
  GPConnection * connection,
  const char authtoken[GP_AUTHTOKEN_LEN],
  const char partnerchallenge[GP_PARTNERCHALLENGE_LEN],
  GPEnum firewall,
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;
	if((authtoken == NULL) || (authtoken[0] == '\0'))
		return GP_PARAMETER_ERROR;
	if((partnerchallenge == NULL) || (partnerchallenge[0] == '\0'))
		return GP_PARAMETER_ERROR;

	// Check for no callback.
	/////////////////////////
	if(callback == NULL)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");

	// Check for simulation mode.
	/////////////////////////////
	iconnection = (GPIConnection*)*connection;
	if(iconnection->simulation)
	{
		GPConnectResponseArg arg;
		memset(&arg, 0, sizeof(arg));
		callback(connection, &arg, param);
		return GP_NO_ERROR;
	}

	// Do it.
	/////////
	return gpiConnect(connection, "", "", "", "", authtoken, partnerchallenge, NULL, firewall, GPIFalse, blocking, callback, param);
}
#ifdef GSI_UNICODE
GPResult gpConnectPreAuthenticatedW
(
  GPConnection * connection,
  const unsigned short authtoken[GP_AUTHTOKEN_LEN],
  const unsigned short partnerchallenge[GP_PARTNERCHALLENGE_LEN],
  GPEnum firewall,
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	char authtoken_A[GP_AUTHTOKEN_LEN];
	char partnerchallenge_A[GP_PARTNERCHALLENGE_LEN];

	UCS2ToAsciiString(authtoken, authtoken_A);
	UCS2ToAsciiString(partnerchallenge, partnerchallenge_A);

	return gpConnectPreAuthenticatedA(connection, authtoken_A, partnerchallenge_A, firewall, blocking, callback, param);
}
#endif

void gpDisconnect(
  GPConnection * connection
)
{
	GPIConnection * iconnection;
    int oldState;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return;

	// Check for simulation mode.
	/////////////////////////////
	iconnection = (GPIConnection*)*connection;
	if(iconnection->simulation)
		return;

    // Make a note of connection state prior to reset
    /////////////////////////////////////////////////
    oldState = iconnection->connectState;

	gpiDisconnect(connection, GPITrue);
	//Added by Saad Nader
	//08-28-2004; fix for memory leaks after being disconnected abruptly
	////////////////////////////////////////////////
    gpiReset(connection);

    // If we were connected prior, set to disconnected to save off info cache
    //////////////////////////////////////////////////////////////////////////
    if (oldState == GPI_CONNECTED)
        iconnection->connectState = GPI_DISCONNECTED;

}

GPResult gpIsConnected
(
  GPConnection * connection,
  GPEnum * connected
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Set the flag.
	////////////////
	if(iconnection->connectState == GPI_CONNECTED)
		*connected = GP_CONNECTED;
	else
		*connected = GP_NOT_CONNECTED;

	return GP_NO_ERROR;
}

GPResult gpCheckUserA(
  GPConnection * connection,
  const char nick[GP_NICK_LEN],
  const char email[GP_EMAIL_LEN],
  const char password[GP_PASSWORD_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Check for no callback.
	/////////////////////////
	if(callback == NULL)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");

	// Check the length of the nick.
	////////////////////////////////
	if(strlen(nick) >= GP_NICK_LEN)
		Error(connection, GP_PARAMETER_ERROR, "Nick too long.");

	// Check the length of the email.
	/////////////////////////////////
	if(strlen(email) >= GP_EMAIL_LEN)
		Error(connection, GP_PARAMETER_ERROR, "Email too long.");

	// Check the length of the password.
	////////////////////////////////////
	if(password && (strlen(password) >= GP_PASSWORD_LEN))
		Error(connection, GP_PARAMETER_ERROR, "Password too long.");

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
	{
		GPCheckResponseArg arg;
		memset(&arg, 0, sizeof(arg));
		callback(connection, &arg, param);
		return GP_NO_ERROR;
	}

	// Do the check.
	////////////////
	return gpiCheckUser(connection, nick, email, password, blocking, callback, param);
}
#ifdef GSI_UNICODE
GPResult gpCheckUserW(
  GPConnection * connection,
  const unsigned short nick[GP_NICK_LEN],
  const unsigned short email[GP_EMAIL_LEN],
  const unsigned short password[GP_PASSWORD_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	char nick_A[GP_NICK_LEN];
	char email_A[GP_NICK_LEN];
	char password_A[GP_NICK_LEN];

	UCS2ToAsciiString(nick, nick_A);
	UCS2ToAsciiString(email, email_A);
	UCS2ToAsciiString(password, password_A);

	return gpCheckUserA(connection, nick_A, email_A, password_A, blocking, callback, param);
}
#endif

GPResult gpNewUserA(
  GPConnection * connection,
  const char nick[GP_NICK_LEN],
  const char uniquenick[GP_UNIQUENICK_LEN],
  const char email[GP_EMAIL_LEN],
  const char password[GP_PASSWORD_LEN],
  const char cdkey[GP_CDKEY_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;
	if((nick == NULL) || (nick[0] == '\0'))
		return GP_PARAMETER_ERROR;
	if(uniquenick == NULL)
		uniquenick = "";
	if((email == NULL) || (email[0] == '\0'))
		return GP_PARAMETER_ERROR;
	if((password == NULL) || (password[0] == '\0'))
		return GP_PARAMETER_ERROR;
	if(cdkey && (cdkey[0] == '\0'))
		cdkey = NULL;

	// Check for no callback.
	/////////////////////////
	if(callback == NULL)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");

	// Check the length of the nick.
	////////////////////////////////
	if(strlen(nick) >= GP_NICK_LEN)
		Error(connection, GP_PARAMETER_ERROR, "Nick too long.");

	// Check the length of the uniquenick.
	//////////////////////////////////////
	if(strlen(uniquenick) >= GP_UNIQUENICK_LEN)
		Error(connection, GP_PARAMETER_ERROR, "Uniquenick too long.");

	// Check the length of the email.
	/////////////////////////////////
	if(strlen(email) >= GP_EMAIL_LEN)
		Error(connection, GP_PARAMETER_ERROR, "Email too long.");

	// Check the length of the password.
	////////////////////////////////////
	if(strlen(password) >= GP_PASSWORD_LEN)
		Error(connection, GP_PARAMETER_ERROR, "Password too long.");

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
	{
		GPNewUserResponseArg arg;
		memset(&arg, 0, sizeof(arg));
		callback(connection, &arg, param);
		return GP_NO_ERROR;
	}

	// Creat the new user.
	//////////////////////
	return gpiNewUser(connection, nick, uniquenick, email, password, cdkey, blocking, callback, param);
}
#ifdef GSI_UNICODE
GPResult gpNewUserW(
  GPConnection * connection,
  const unsigned short nick[GP_NICK_LEN],
  const unsigned short uniquenick[GP_UNIQUENICK_LEN],
  const unsigned short email[GP_EMAIL_LEN],
  const unsigned short password[GP_PASSWORD_LEN],
  const unsigned short cdkey[GP_CDKEY_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	char nick_A[GP_NICK_LEN];
	char uniquenick_A[GP_UNIQUENICK_LEN];
	char email_A[GP_EMAIL_LEN];
	char password_A[GP_PASSWORD_LEN];
	char cdkey_A[GP_CDKEY_LEN];

	UCS2ToAsciiString(nick, nick_A);
	UCS2ToAsciiString(uniquenick, uniquenick_A);
	UCS2ToAsciiString(email, email_A);
	UCS2ToAsciiString(password, password_A);
	UCS2ToAsciiString(cdkey, cdkey_A);

	return gpNewUserA(connection, nick_A, uniquenick_A, email_A, password_A, cdkey_A, blocking, callback, param);
}
#endif

GPResult gpSuggestUniqueNickA(
  GPConnection * connection,
  const char desirednick[GP_NICK_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Check for no callback.
	/////////////////////////
	if(callback == NULL)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");

	// Check the length of the desirednick.
	///////////////////////////////////////
	if(strlen(desirednick) >= GP_UNIQUENICK_LEN)
		Error(connection, GP_PARAMETER_ERROR, "Desirednick too long.");

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
	{
		GPSuggestUniqueNickResponseArg arg;
		memset(&arg, 0, sizeof(arg));
		callback(connection, &arg, param);
		return GP_NO_ERROR;
	}

	// Creat the new user.
	//////////////////////
	return gpiSuggestUniqueNick(connection, desirednick, blocking, callback, param);
}
#ifdef GSI_UNICODE
GPResult gpSuggestUniqueNickW(
  GPConnection * connection,
  const unsigned short desirednick[GP_NICK_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	char desirednick_A[GP_UNIQUENICK_LEN];

	UCS2ToAsciiString(desirednick, desirednick_A);

	return gpSuggestUniqueNickA(connection, desirednick_A, blocking, callback, param);
}
#endif

GPResult gpRegisterUniqueNickA(
  GPConnection * connection,
  const char uniquenick[GP_UNIQUENICK_LEN],
  const char cdkey[GP_CDKEY_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;
	if((uniquenick == NULL) || (uniquenick[0] == '\0'))
		return GP_PARAMETER_ERROR;
	if(cdkey && (cdkey[0] == '\0'))
		cdkey = NULL;

	// Check for no callback.
	/////////////////////////
	if(callback == NULL)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
	{
		GPRegisterUniqueNickResponseArg arg;
		memset(&arg, 0, sizeof(arg));
		callback(connection, &arg, param);
		return GP_NO_ERROR;
	}

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	return gpiRegisterUniqueNick(connection, uniquenick, cdkey, blocking, callback, param);
}
#ifdef GSI_UNICODE
GPResult gpRegisterUniqueNickW(
  GPConnection * connection,
  const unsigned short uniquenick[GP_UNIQUENICK_LEN],
  const unsigned short cdkey[GP_CDKEY_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	char uniquenick_A[GP_UNIQUENICK_LEN];
	char cdkey_A[GP_CDKEY_LEN];

	UCS2ToAsciiString(uniquenick, uniquenick_A);
	UCS2ToAsciiString(cdkey, cdkey_A);

	return gpRegisterUniqueNickA(connection, uniquenick_A, cdkey_A, blocking, callback, param);
}
#endif


GPResult gpRegisterCdKeyA(
  GPConnection * connection,
  const char cdkey[GP_CDKEY_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;
	if((cdkey == NULL) || (cdkey[0] == '\0'))
		return GP_PARAMETER_ERROR;

	// Check for no callback.
	/////////////////////////
	if(callback == NULL)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
	{
		GPRegisterCdKeyResponseArg arg;
		memset(&arg, 0, sizeof(arg));
		callback(connection, &arg, param);
		return GP_NO_ERROR;
	}

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	return gpiRegisterCdKey(connection, cdkey, blocking, callback, param);
}
#ifdef GSI_UNICODE
GPResult gpRegisterCdKeyW(
  GPConnection * connection, 
  const gsi_char cdkey[GP_CDKEY_LEN], 
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	char cdkey_A[GP_CDKEY_LEN];
	UCS2ToAsciiString(cdkey, cdkey_A);
	return gpRegisterCdKeyA(connection, cdkey_A, blocking, callback, param);
}
#endif

GPResult gpGetErrorCode(
  GPConnection * connection,
  GPErrorCode * errorCode
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Error check.
	///////////////
	if(errorCode == NULL)
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
	{
		*errorCode = (GPErrorCode)0;
		return GP_NO_ERROR;
	}

	// Set the code.
	////////////////
	*errorCode = iconnection->errorCode;

	return GP_NO_ERROR;
}

GPResult gpGetErrorStringA(
  GPConnection * connection,
  char errorString[GP_ERROR_STRING_LEN]
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Error check.
	///////////////
	if(errorString == NULL)
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
	{
		errorString[0] = '\0';
		return GP_NO_ERROR;
	}

	// Copy the error string.
	/////////////////////////
	strzcpy(errorString, iconnection->errorString, GP_ERROR_STRING_LEN);
	return GP_NO_ERROR;
}
#ifdef GSI_UNICODE
GPResult gpGetErrorStringW(
  GPConnection * connection,
  unsigned short errorString[GP_ERROR_STRING_LEN]
)
{
	char errorString_A[GP_ERROR_STRING_LEN];
	GPResult result;

	result = gpGetErrorStringA(connection, errorString_A);
	AsciiToUCS2String(errorString_A, errorString);
	return result;
}
#endif

GPResult gpNewProfileA(
  GPConnection * connection,
  const char nick[GP_NICK_LEN],
  GPEnum replace,
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Check for no callback.
	/////////////////////////
	if(callback == NULL)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");

	// Check for no nick.
	// PANTS|05.18.00
	/////////////////////
	if((nick == NULL) || (nick[0] == '\0'))
		Error(connection, GP_PARAMETER_ERROR, "Invalid nick.");

	// Check for simulation mode.
	/////////////////////////////
	iconnection = (GPIConnection*)*connection;
	if(iconnection->simulation)
	{
		GPNewProfileResponseArg arg;
		memset(&arg, 0, sizeof(arg));
		callback(connection, &arg, param);
		return GP_NO_ERROR;
	}

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	return gpiNewProfile(connection, nick, replace, blocking, callback, param);
}
#ifdef GSI_UNICODE
GPResult gpNewProfileW(
  GPConnection * connection,
  const unsigned short nick[GP_NICK_LEN],
  GPEnum replace,
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	char nick_A[GP_NICK_LEN];

	// Since we don't currently allow UNICODE nicknames, make sure the first byte is empty
	// (We make this check as an early alert to devlopers that the parameter is invalid)
	// (Even if it's bypassed, the server will still reject the name.)
	int i = 0;
	for (; (i < GP_NICK_LEN) && (nick[i] != 0); i++)
	{
		if ((nick[i] & 0xFF00) != 0)
			return GP_PARAMETER_ERROR;
	}

	// Convert to ascii and call "A" version
	UCS2ToAsciiString(nick, nick_A);
	return gpNewProfileA(connection, nick_A, replace, blocking, callback, param);
}
#endif

GPResult gpDeleteProfile(
    GPConnection * connection,
    GPCallback callback,
	void * param
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Check for no callback.
	/////////////////////////
	if(callback == NULL)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");

	// Check for simulation mode.
	/////////////////////////////
	iconnection = (GPIConnection*)*connection;
	if(iconnection->simulation)
	{
		GPDeleteProfileResponseArg arg;
		memset(&arg, 0, sizeof(arg));
		callback(connection, &arg, param);
		return GP_NO_ERROR;
	}

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	return gpiDeleteProfile(connection, callback, param);
}

GPResult gpProfileFromID(
  GPConnection * connection, 
  GPProfile * profile, 
  int id
)
{
	GSI_UNUSED(connection);

	// Set the profile.
	// This function is depreciated & may be removed from future versions.
	//////////////////////////////////////////////////////////////////////
	*profile = id;

	return GP_NO_ERROR;
}

// gpIDFromProfile
//////////////////
GPResult gpIDFromProfile(
  GPConnection * connection,
  GPProfile profile,
  int * id
)
{
	GSI_UNUSED(connection);

	// ID is the same as GPProfile
	// This function is depreciated & may be removed from future versions.
	//////////////////////////////////////////////////////////////////////
	*id = profile;

	return GP_NO_ERROR;
}

// gpUserIDFromProfile
//////////////////
GPResult gpUserIDFromProfile(
  GPConnection * connection,
  GPProfile profile,
  int * userid
)
{
	GPIConnection * iconnection;
	GPIProfile * pProfile;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
	{
		*userid = 0;
		return GP_NO_ERROR;
	}

	// Get the profile object.
	//////////////////////////
	if(!gpiGetProfile(connection, profile, &pProfile))
		Error(connection, GP_PARAMETER_ERROR, "Invalid profile.");

	// Set the id.
	//////////////
	*userid = pProfile->userId;

	return GP_NO_ERROR;
}


GPResult gpProfileSearchA(
  GPConnection * connection,
  const char nick[GP_NICK_LEN],
  const char uniquenick[GP_UNIQUENICK_LEN],
  const char email[GP_EMAIL_LEN],
  const char firstname[GP_FIRSTNAME_LEN],
  const char lastname[GP_LASTNAME_LEN],
  int icquin,
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Check for no callback.
	/////////////////////////
	if(callback == NULL)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
	{
		GPProfileSearchResponseArg arg;
		memset(&arg, 0, sizeof(arg));
		arg.more = GP_DONE;
		callback(connection, &arg, param);
		return GP_NO_ERROR;
	}

	// Do the search.
	/////////////////
	return gpiProfileSearch(connection, nick, uniquenick, email, firstname, lastname, icquin, 0, blocking, callback, param);
}
#ifdef GSI_UNICODE
GPResult gpProfileSearchW(
  GPConnection * connection,
  const unsigned short nick[GP_NICK_LEN],
  const unsigned short uniquenick[GP_UNIQUENICK_LEN],
  const unsigned short email[GP_EMAIL_LEN],
  const unsigned short firstname[GP_FIRSTNAME_LEN],
  const unsigned short lastname[GP_LASTNAME_LEN],
  int icquin,
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	char nick_A[GP_NICK_LEN];
	char uniquenick_A[GP_UNIQUENICK_LEN];
	char email_A[GP_NICK_LEN];
	char firstname_A[GP_NICK_LEN];
	char lastname_A[GP_NICK_LEN];

	UCS2ToAsciiString(nick, nick_A); // nicknames are ascii
	UCS2ToAsciiString(uniquenick, uniquenick_A);
	UCS2ToAsciiString(email, email_A);
	UCS2ToAsciiString(firstname, firstname_A);
	UCS2ToAsciiString(lastname, lastname_A);

	return gpProfileSearchA(connection, nick_A, uniquenick_A, email_A, firstname_A, lastname_A, icquin, blocking, callback, param);
}
#endif

GPResult gpProfileSearchUniquenickA(
  GPConnection * connection,
  const char uniquenick[GP_UNIQUENICK_LEN],
  const int namespaceIDs[GP_MAX_NAMESPACEIDS],
  int numNamespaces,
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL) || (namespaceIDs == NULL) || (numNamespaces < 1))
		return GP_PARAMETER_ERROR;

	// Check for no callback.
	/////////////////////////
	if(callback == NULL)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
	{
		GPProfileSearchResponseArg arg;
		memset(&arg, 0, sizeof(arg));
		arg.more = GP_DONE;
		callback(connection, &arg, param);
		return GP_NO_ERROR;
	}

	// Do the search.
	/////////////////
	return gpiProfileSearchUniquenick(connection, uniquenick, namespaceIDs, numNamespaces, blocking, callback, param);
}
#ifdef GSI_UNICODE
GPResult gpProfileSearchUniquenickW(
  GPConnection * connection,
  const unsigned short uniquenick[GP_UNIQUENICK_LEN],
  const int namespaceIDs[GP_MAX_NAMESPACEIDS],
  int numNamespaces,
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	char uniquenick_A[GP_UNIQUENICK_LEN];

	UCS2ToAsciiString(uniquenick, uniquenick_A);

	return gpProfileSearchUniquenickA(connection, uniquenick_A, namespaceIDs, numNamespaces, blocking, callback, param);
}
#endif

GPResult gpGetInfo(
  GPConnection * connection,
  GPProfile profile, 
  GPEnum checkCache,
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL) || (profile == 0))
		return GP_PARAMETER_ERROR;

	// Check for no callback.
	/////////////////////////
	if(callback == NULL)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
	{
		GPGetInfoResponseArg arg;
		memset(&arg, 0, sizeof(arg));
		callback(connection, &arg, param);
		return GP_NO_ERROR;
	}

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	return gpiGetInfo(connection, profile, checkCache, blocking, callback, param);
}

GPResult gpGetInfoNoWait(
  GPConnection * connection,
  GPProfile profile,
  GPGetInfoResponseArg * arg
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL) || (profile == 0) || (arg == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	iconnection = (GPIConnection*)*connection;
	if(iconnection->simulation)
	{
		memset(arg, 0, sizeof(arg));
		return GP_NO_ERROR;
	}

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	return gpiGetInfoNoWait(connection, profile, arg);
}

GPResult gpSetInfoi(
  GPConnection * connection, 
  GPEnum info, 
  int value
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Check for simulation mode.
	/////////////////////////////
	iconnection = (GPIConnection*)*connection;
	if(iconnection->simulation)
		return GP_NO_ERROR;

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	return gpiSetInfoi(connection, info, value);
}

GPResult gpSetInfosA(
  GPConnection * connection, 
  GPEnum info, 
  const char * value
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Check for simulation mode.
	/////////////////////////////
	iconnection = (GPIConnection*)*connection;
	if(iconnection->simulation)
		return GP_NO_ERROR;

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	return gpiSetInfos(connection, info, value);
}
#ifdef GSI_UNICODE
GPResult gpSetInfosW(
  GPConnection * connection, 
  GPEnum info, 
  const unsigned short* value
)
{
	char* value_A = UCS2ToUTF8StringAlloc(value);
	GPResult result = gpSetInfosA(connection, info, value_A);
	gsifree(value_A);
	return result;
}
#endif

GPResult gpSetInfod(
  GPConnection * connection,
  GPEnum info,
  int day,
  int month,
  int year
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Check for simulation mode.
	/////////////////////////////
	iconnection = (GPIConnection*)*connection;
	if(iconnection->simulation)
		return GP_NO_ERROR;

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	return gpiSetInfod(connection, info, day, month, year);
}

GPResult gpSetInfoMask(
  GPConnection * connection,
  GPEnum mask
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Check for simulation mode.
	/////////////////////////////
	iconnection = (GPIConnection*)*connection;
	if(iconnection->simulation)
		return GP_NO_ERROR;

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	return gpiSetInfoMask(connection, mask);
}

GPResult gpSendBuddyRequestA(
  GPConnection * connection,
  GPProfile profile,
  const char reason[GP_REASON_LEN]
)
{
	GPIConnection * iconnection;
	char reasonFixed[GP_REASON_LEN];
	int i;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
		return GP_NO_ERROR;

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	// Error check.
	///////////////
	if(reason == NULL)
		Error(connection, GP_PARAMETER_ERROR, "Invalid reason.");

	// Replace backslashes in reason.
	/////////////////////////////////
	strzcpy(reasonFixed, reason, GP_REASON_LEN);
	for(i = 0 ; reasonFixed[i] ; i++)
		if(reasonFixed[i] == '\\')
			reasonFixed[i] = '/';

	// Send the request.
	////////////////////
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\addbuddy\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\sesskey\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->sessKey);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\newprofileid\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, profile);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\reason\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, reasonFixed);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");

	return GP_NO_ERROR;
}
#ifdef GSI_UNICODE
GPResult gpSendBuddyRequestW(
  GPConnection * connection,
  GPProfile profile,
  const unsigned short reason[GP_REASON_LEN]
)
{
	char reason_A[GP_REASON_LEN];
	UCS2ToUTF8String(reason, reason_A);
	return gpSendBuddyRequestA(connection, profile, reason_A);
}
#endif

GPResult gpAuthBuddyRequest(
  GPConnection * connection,
  GPProfile profile
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Check for simulation mode.
	/////////////////////////////
	iconnection = (GPIConnection*)*connection;
	if(iconnection->simulation)
		return GP_NO_ERROR;

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	return gpiAuthBuddyRequest(connection, profile);
}

GPResult gpDenyBuddyRequest(
  GPConnection * connection,
  GPProfile profile
)
{
	GPIConnection * iconnection;
	GPIProfile * pProfile;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Check for simulation mode.
	/////////////////////////////
	iconnection = (GPIConnection*)*connection;
	if(iconnection->simulation)
		return GP_NO_ERROR;

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	// Get the profile.
	///////////////////
	if(!gpiGetProfile(connection, profile, &pProfile))
		return GP_NO_ERROR;

	// freeclear the sig if no more requests.
	////////////////////////////////////
	pProfile->requestCount--;
	if(!iconnection->infoCaching && (pProfile->requestCount <= 0))
	{
		freeclear(pProfile->authSig);
		if(gpiCanFreeProfile(pProfile))
			gpiRemoveProfile(connection, pProfile);
	}

	return GP_NO_ERROR;
}

GPResult gpGetNumBuddies(
  GPConnection * connection,
  int * numBuddies
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
	{
		*numBuddies = 0;
		return GP_NO_ERROR;
	}

	// Set the number of buddies.
	/////////////////////////////
	*numBuddies = iconnection->profileList.numBuddies;

	return GP_NO_ERROR;
}

#ifndef GP_NEW_STATUS_INFO
GPResult gpGetBuddyStatus(
  GPConnection * connection,
  int index, 
  GPBuddyStatus * status
)
{
	GPIConnection * iconnection;
	int num;
	GPIProfile * profile;
	
	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
	{
		memset(status, 0, sizeof(GPBuddyStatus));
		return GP_NO_ERROR;
	}

	// Check for a NULL status.
	///////////////////////////
	if(status == NULL)
		Error(connection, GP_PARAMETER_ERROR, "Invalid status.");

	// Check the buddy index.
	/////////////////////////
	num = iconnection->profileList.numBuddies;
	if((index < 0) || (index >= num))
		Error(connection, GP_PARAMETER_ERROR, "Invalid index.");

	// Find the buddy with this index.
	//////////////////////////////////
	profile = gpiFindBuddy(connection, index);
	if(!profile)
		Error(connection, GP_PARAMETER_ERROR, "Invalid index.");

	
	assert(profile->buddyStatus);
	status->profile = (GPProfile)profile->profileId;
	status->status = profile->buddyStatus->status;
#ifndef GSI_UNICODE
	if(profile->buddyStatus->statusString)
		strzcpy(status->statusString, profile->buddyStatus->statusString, GP_STATUS_STRING_LEN);
	else
		status->statusString[0] = '\0';
	

	if(profile->buddyStatus->locationString)
		strzcpy(status->locationString, profile->buddyStatus->locationString, GP_LOCATION_STRING_LEN);
	else
		status->locationString[0] = '\0';

#else
	if(profile->buddyStatus->statusString)
		UTF8ToUCS2String(profile->buddyStatus->statusString, status->statusString);
	else
		status->statusString[0] = '\0';

	if(profile->buddyStatus->locationString)
		UTF8ToUCS2String(profile->buddyStatus->locationString, status->locationString);
	else
		status->locationString[0] = '\0';

#endif
	status->ip = profile->buddyStatus->ip;
	status->port = profile->buddyStatus->port;
	status->quietModeFlags = profile->buddyStatus->quietModeFlags;
	
	return GP_NO_ERROR;
}
#endif

#ifdef GP_NEW_STATUS_INFO
GPResult gpGetBuddyStatusInfo(
	GPConnection * connection,
	int index, 
	GPBuddyStatusInfo * statusInfo
)
{
	GPIConnection * iconnection;
	int num;
	GPIProfile * profile;
	GPIBuddyStatus *buddyStatus;
	GPIBuddyStatusInfo * buddyStatusInfo;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
	{
		memset(statusInfo, 0, sizeof(GPBuddyStatusInfo));
		return GP_NO_ERROR;
	}

	// Check for a NULL status.
	///////////////////////////
	if(statusInfo == NULL)
		Error(connection, GP_PARAMETER_ERROR, "Invalid status.");

	// Check the buddy index.
	/////////////////////////
	num = iconnection->profileList.numBuddies;
	if((index < 0) || (index >= num))
		Error(connection, GP_PARAMETER_ERROR, "Invalid index.");

	// Find the buddy with this index.
	//////////////////////////////////
	profile = gpiFindBuddy(connection, index);
	if(!profile)
		Error(connection, GP_PARAMETER_ERROR, "Invalid index.");

	
	buddyStatus = profile->buddyStatus;
	buddyStatusInfo = profile->buddyStatusInfo;
	assert(buddyStatus || buddyStatusInfo);

	statusInfo->profile = (GPProfile)profile->profileId;
	if (buddyStatus)
	{
		statusInfo->statusState = buddyStatus->status;
#ifndef GSI_UNICODE
		if(buddyStatus->statusString)
			strzcpy(statusInfo->richStatus, buddyStatus->statusString, GP_RICH_STATUS_LEN);
		else
			statusInfo->richStatus[0] = '\0';
		statusInfo->gameType[0] = '\0';
		statusInfo->gameVariant[0] = '\0';
		statusInfo->gameMapName[0] = '\0';
#else
		if(buddyStatus->statusString)
			UTF8ToUCS2String(buddyStatus->statusString, statusInfo->richStatus);
		else
			statusInfo->richStatus[0] = '\0';
		statusInfo->gameType[0] = '\0';
		statusInfo->gameVariant[0] = '\0';
		statusInfo->gameMapName[0] = '\0';
#endif
		statusInfo->buddyIp = buddyStatus->ip;
		statusInfo->buddyPort = buddyStatus->port;
		statusInfo->quietModeFlags = buddyStatus->quietModeFlags;
		statusInfo->newStatusInfoFlag = GP_NEW_STATUS_INFO_NOT_SUPPORTED;
	}
	else if (buddyStatusInfo)
	{
		statusInfo->statusState = buddyStatusInfo->statusState;
		statusInfo->buddyIp = buddyStatusInfo->buddyIp;
		statusInfo->buddyPort = buddyStatusInfo->buddyPort;
		statusInfo->hostIp = buddyStatusInfo->hostIp;
		statusInfo->hostPrivateIp = buddyStatusInfo->hostPrivateIp;
		statusInfo->queryPort = buddyStatusInfo->queryPort;
		statusInfo->hostPort = buddyStatusInfo->hostPort;
		statusInfo->sessionFlags = buddyStatusInfo->sessionFlags;
		statusInfo->quietModeFlags = buddyStatusInfo->quietModeFlags;
		statusInfo->newStatusInfoFlag = GP_NEW_STATUS_INFO_SUPPORTED;
#ifndef GSI_UNICODE
		strzcpy(statusInfo->richStatus, buddyStatusInfo->richStatus, GP_RICH_STATUS_LEN);
		strzcpy(statusInfo->gameType, buddyStatusInfo->gameType, GP_STATUS_BASIC_STR_LEN);
		strzcpy(statusInfo->gameVariant, buddyStatusInfo->gameVariant, GP_STATUS_BASIC_STR_LEN);
		strzcpy(statusInfo->gameMapName, buddyStatusInfo->gameMapName, GP_STATUS_BASIC_STR_LEN);
#else
		UTF8ToUCS2String(buddyStatusInfo->richStatus, statusInfo->richStatus);
		UTF8ToUCS2String(buddyStatusInfo->gameType, statusInfo->gameType);
		UTF8ToUCS2String(buddyStatusInfo->gameVariant, statusInfo->gameVariant);
		UTF8ToUCS2String(buddyStatusInfo->gameMapName, statusInfo->gameMapName);
#endif
	}
	
	return GP_NO_ERROR;
}

GPResult gpSetBuddyAddr(
	GPConnection *connection, 
	int index,
	unsigned int buddyIp,
	unsigned short buddyPort
)
{
	GPIConnection * iconnection;
	int num;
	GPIProfile * profile;
	
	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
		return GP_NO_ERROR;

	// Check the buddy index.
	/////////////////////////
	num = iconnection->profileList.numBuddies;
	if((index < 0) || (index >= num))
		Error(connection, GP_PARAMETER_ERROR, "Invalid index.");
	
	// Find the buddy with this index.
	//////////////////////////////////
	profile = gpiFindBuddy(connection, index);
	if(!profile)
		Error(connection, GP_PARAMETER_ERROR, "Invalid index.");

	if (buddyIp == 0 || buddyPort == 0)
		Error(connection, GP_PARAMETER_ERROR, "Invalid IP and port");
	if (profile->buddyStatusInfo)
	{
		profile->buddyStatusInfo->buddyIp = buddyIp;
		profile->buddyStatusInfo->buddyPort = buddyPort;
	}
	return GP_NO_ERROR;
}
#endif

GPResult gpGetBuddyIndex(
  GPConnection * connection, 
  GPProfile profile, 
  int * index
)
{
	GPIProfile * pProfile;
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
	{
		*index = 0;
		return GP_NO_ERROR;
	}

	// Get the index.
	/////////////////
	if(gpiGetProfile(connection, profile, &pProfile) && pProfile->buddyStatus)
		*index = pProfile->buddyStatus->buddyIndex;
	else if (gpiGetProfile(connection, profile, &pProfile) && pProfile->buddyStatusInfo)
		*index = pProfile->buddyStatusInfo->buddyIndex;
	else
		*index = -1;

	return GP_NO_ERROR;
}

int gpIsBuddy(
  GPConnection * connection,
  GPProfile profile
)
{
	GPIProfile * pProfile;
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return 0;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
		return 0;

	// Get the index.
	/////////////////
	if(gpiGetProfile(connection, profile, &pProfile) && pProfile->buddyStatus)
		return 1;
	else if (gpiGetProfile(connection, profile, &pProfile) &&pProfile->buddyStatusInfo)
		return 1;

	return 0;
}

int gpIsBuddyConnectionOpen(
  GPConnection * connection, 
  GPProfile profile
)
{
	GPIConnection * iconnection;
	GPIPeer *aPeer;
	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return 0;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
		return 0;

	aPeer = gpiGetPeerByProfile(connection, profile);

	if (aPeer == NULL || !gpiIsPeerConnected(aPeer))
		return 0; // not connected
	else
		return 1; // connected
}

GPResult gpDeleteBuddy(
  GPConnection * connection,
  GPProfile profile
)
{
	GPIConnection * iconnection;
	
	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
		return GP_NO_ERROR;

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	// Delete the buddy.
	////////////////////
	CHECK_RESULT(gpiDeleteBuddy(connection, profile, GPITrue));

	return GP_NO_ERROR;
}

GPResult gpAddToBlockedList(
  GPConnection * connection,
  GPProfile profile
)
{
    GPIConnection * iconnection;

    // Error check.
    ///////////////
    if((connection == NULL) || (*connection == NULL))
        return GP_PARAMETER_ERROR;

    // Get the connection object.
    /////////////////////////////
    iconnection = (GPIConnection *)*connection;

    // Check for disconnected.
    //////////////////////////
    if(iconnection->connectState == GPI_DISCONNECTED)
        Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

    // Check for simulation mode.
    /////////////////////////////
    if(iconnection->simulation)
        return GP_NO_ERROR;
    
    // Add em to the internal list - remove buddy status if already a buddy
    ///////////////////////////////////////////////////////////////////////
    return gpiAddToBlockedList(connection, profile);
}

GPResult gpRemoveFromBlockedList(
  GPConnection * connection,
  GPProfile profile
)
{
    GPIConnection * iconnection;

    // Error check.
    ///////////////
    if((connection == NULL) || (*connection == NULL))
        return GP_PARAMETER_ERROR;

    // Get the connection object.
    /////////////////////////////
    iconnection = (GPIConnection *)*connection;

    // Check for disconnected.
    //////////////////////////
    if(iconnection->connectState == GPI_DISCONNECTED)
        Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

    // Check for simulation mode.
    /////////////////////////////
    if(iconnection->simulation)
        return GP_NO_ERROR;

    // Remove blocked association from internal list if it's there
    //////////////////////////////////////////////////////////////
    return gpiRemoveFromBlockedList(connection, profile);
}

GPResult gpGetNumBlocked(
  GPConnection * connection,
  int * numBlocked
)
{
    GPIConnection * iconnection;

    // Error check.
    ///////////////
    if((connection == NULL) || (*connection == NULL))
        return GP_PARAMETER_ERROR;

    // Get the connection object.
    /////////////////////////////
    iconnection = (GPIConnection *)*connection;

    // Check for simulation mode.
    /////////////////////////////
    if(iconnection->simulation)
    {
        *numBlocked = 0;
        return GP_NO_ERROR;
    }

    // Set the number of blocked profiles
    /////////////////////////////////////
    *numBlocked = iconnection->profileList.numBlocked;

    return GP_NO_ERROR;
}

GPResult gpGetBlockedProfile(
  GPConnection * connection, 
  int index,
  GPProfile * profile
)
{
    GPIConnection * iconnection;
    int num;
    GPIProfile * pProfile;

    // Error check.
    ///////////////
    if((connection == NULL) || (*connection == NULL))
        return GP_PARAMETER_ERROR;

    // Get the connection object.
    /////////////////////////////
    iconnection = (GPIConnection *)*connection;

    // Check for simulation mode.
    /////////////////////////////
    if(iconnection->simulation)
        return GP_NO_ERROR;
       
    // Check for a NULL profile.
    ////////////////////////////
    if(profile == NULL)
        Error(connection, GP_PARAMETER_ERROR, "Invalid profile container");

    // Check the block index.
    /////////////////////////
    num = iconnection->profileList.numBlocked;
    if((index < 0) || (index >= num))
        Error(connection, GP_PARAMETER_ERROR, "Invalid index.");

    // Find the blocked profile with this index.
    ////////////////////////////////////////////
    pProfile = gpiFindBlockedProfile(connection, index);
    if(!pProfile)
        Error(connection, GP_PARAMETER_ERROR, "Invalid index.");

    *profile = (GPProfile)pProfile->profileId;

    return GP_NO_ERROR;
}

gsi_bool gpIsBlocked(
  GPConnection * connection,
  GPProfile profile
)
{
    GPIProfile * pProfile;
    GPIConnection * iconnection;

    // Error check.
    ///////////////
    if((connection == NULL) || (*connection == NULL))
        return gsi_false;

    // Get the connection object.
    /////////////////////////////
    iconnection = (GPIConnection *)*connection;

    // Check for simulation mode.
    /////////////////////////////
    if(iconnection->simulation)
        return gsi_false;

    // Get the index.
    /////////////////
    if(gpiGetProfile(connection, profile, &pProfile) && pProfile->blocked)
        return gsi_true;

    return gsi_false;
}

#ifndef GP_NEW_STATUS_INFO
GPResult gpSetStatusA(
  GPConnection * connection,
  GPEnum status,
  const char statusString[GP_STATUS_STRING_LEN],
  const char locationString[GP_LOCATION_STRING_LEN]
)
{
	char statusStringFixed[GP_STATUS_STRING_LEN];
	char locationStringFixed[GP_LOCATION_STRING_LEN];
	GPIConnection * iconnection;
	int i;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
		return GP_NO_ERROR;

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	// Error check.
	///////////////
	if(statusString == NULL)
		Error(connection, GP_PARAMETER_ERROR, "Invalid statusString.");
	if(locationString == NULL)
		Error(connection, GP_PARAMETER_ERROR, "Invalid locationString.");

	// Replace backslashes with slashes.
	////////////////////////////////////
	strzcpy(statusStringFixed, statusString, GP_STATUS_STRING_LEN);
	for(i = 0 ; statusStringFixed[i] ; i++)
		if(statusStringFixed[i] == '\\')
			statusStringFixed[i] = '/';
	strzcpy(locationStringFixed, locationString, GP_LOCATION_STRING_LEN);
	for(i = 0 ; locationStringFixed[i] ; i++)
		if(locationStringFixed[i] == '\\')
			locationStringFixed[i] = '/';

	// Don't send it if its the same as the previous.
	/////////////////////////////////////////////////
	if((status == iconnection->lastStatusState) &&
	   (strcmp(statusStringFixed, iconnection->lastStatusString) == 0) &&
	   (strcmp(locationStringFixed, iconnection->lastLocationString) == 0))
	{
		return GP_NO_ERROR;
	}

	// Copy off the new status.
	///////////////////////////
	iconnection->lastStatusState = status;
#ifndef GSI_UNICODE
	strzcpy(iconnection->lastStatusString, statusStringFixed, GP_STATUS_STRING_LEN);
	strzcpy(iconnection->lastLocationString, locationStringFixed, GP_LOCATION_STRING_LEN);
#else
	UTF8ToUCS2StringLen(iconnection->lastStatusString, iconnection->lastStatusString_W, GP_STATUS_STRING_LEN);
	UTF8ToUCS2StringLen(iconnection->lastStatusString, iconnection->lastLocationString_W, GP_LOCATION_STRING_LEN);
#endif

	// Send the new status.
	///////////////////////
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\status\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, status);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\sesskey\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->sessKey);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\statstring\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, statusStringFixed);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\locstring\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, locationStringFixed);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");

	return GP_NO_ERROR;
}
#ifdef GSI_UNICODE
GPResult gpSetStatusW(
  GPConnection * connection,
  GPEnum status,
  const unsigned short statusString[GP_STATUS_STRING_LEN],
  const unsigned short locationString[GP_LOCATION_STRING_LEN]
)
{
	char statusString_A[GP_STATUS_STRING_LEN];
	char locationString_A[GP_LOCATION_STRING_LEN];
	UCS2ToUTF8String(statusString, statusString_A);
	UCS2ToUTF8String(locationString, locationString_A);
	return gpSetStatusA(connection, status, statusString_A, locationString_A);
}
#endif
#endif

#ifdef GP_NEW_STATUS_INFO
GPResult gpSetStatusInfoA(
						 GPConnection *connection, 
						 GPEnum statusState,
						 unsigned int hostIp, 
						 unsigned int hostPrivateIp,
						 unsigned short queryPort,
						 unsigned short hostPort,
						 unsigned int sessionFlags,
						 const char *richStatus,
						 int richStatusLen,
						 const char *gameType,
						 int gameTypeLen,
						 const char *gameVariant,
						 int gameVariantLen,
						 const char *gameMapName,
						 int gameMapNameLen
)
{
	GPIConnection * iconnection;
	
#ifndef GSI_UNICODE
	char gameTypeFixed[GP_STATUS_BASIC_STR_LEN];
	char gameVariantFixed[GP_STATUS_BASIC_STR_LEN];
	char gameMapNameFixed[GP_STATUS_BASIC_STR_LEN];
#else
	char *gameTypeFixed;
	char *gameVariantFixed;
	char *gameMapNameFixed;
#endif

	GS_ASSERT(connection != NULL);
	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
		return GP_NO_ERROR;

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	// Error check.
	///////////////
	GS_ASSERT(richStatus != NULL);
	if (richStatus == NULL)
		Error(connection, GP_PARAMETER_ERROR, "Invalid richStatus.");

#ifndef GSI_UNICODE
	GS_ASSERT(richStatusLen <= GP_RICH_STATUS_LEN);
	GS_ASSERT(gameTypeLen <= GP_STATUS_BASIC_STR_LEN);
	GS_ASSERT(gameVariantLen <= GP_STATUS_BASIC_STR_LEN);
	GS_ASSERT(gameMapNameLen <= GP_STATUS_BASIC_STR_LEN);

	if (gameType == NULL)
		strncpy(gameTypeFixed, "", GP_STATUS_BASIC_STR_LEN);
	else
		strncpy(gameTypeFixed, gameType, GP_STATUS_BASIC_STR_LEN);
	if (gameVariant == NULL)
		strncpy(gameVariantFixed, "", GP_STATUS_BASIC_STR_LEN);
	else
		strncpy(gameVariantFixed, gameVariant, GP_STATUS_BASIC_STR_LEN);
	if (gameMapName == NULL)
		strncpy(gameMapNameFixed, "", GP_STATUS_BASIC_STR_LEN);
	else
		strncpy(gameMapNameFixed, gameMapName, GP_STATUS_BASIC_STR_LEN);

	// Don't send it if its the same as the previous.
	/////////////////////////////////////////////////
	if((statusState == iconnection->lastStatusState) &&
		(strcmp(richStatus, iconnection->richStatus) == 0) &&
		(strcmp(gameTypeFixed, iconnection->gameType) == 0) &&
		(strcmp(gameVariantFixed, iconnection->gameVariant) == 0) &&
		(strcmp(gameMapNameFixed, iconnection->gameMapName) == 0) &&
		(sessionFlags == iconnection->sessionFlags) &&
		(hostIp == iconnection->hostIp) &&
		(hostPrivateIp == iconnection->hostPrivateIp) &&
		(queryPort == iconnection->queryPort) &&
		(hostPort == iconnection->hostPort))
	{
		return GP_NO_ERROR;
	}
#else
	gameTypeFixed = goastrdup(gameType);
	gameVariantFixed = goastrdup(gameVariant);
	gameMapNameFixed = goastrdup(gameMapName);
#endif
	
	iconnection->lastStatusState = statusState;
	iconnection->hostIp = hostIp;
	iconnection->hostPrivateIp = hostPrivateIp;
	iconnection->queryPort = queryPort;
	iconnection->hostPort = hostPort;
	iconnection->sessionFlags = sessionFlags;

#ifndef GSI_UNICODE
	strzcpy(iconnection->gameType, gameTypeFixed, GP_STATUS_BASIC_STR_LEN);
	strzcpy(iconnection->gameVariant, gameVariantFixed, GP_STATUS_BASIC_STR_LEN);
	strzcpy(iconnection->gameMapName, gameMapNameFixed, GP_STATUS_BASIC_STR_LEN);
	strzcpy(iconnection->richStatus, richStatus, GP_RICH_STATUS_LEN);
#endif
	
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\statusinfo\\\\state\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, statusState);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\sesskey\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->sessKey);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\hostIp\\");
	gpiAppendUIntToBuffer(connection, &iconnection->outputBuffer, ntohl(hostIp));
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\hprivIp\\");
	gpiAppendUIntToBuffer(connection, &iconnection->outputBuffer, ntohl(hostPrivateIp));
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\qport\\");
	gpiAppendUShortToBuffer(connection, &iconnection->outputBuffer, queryPort);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\hport\\");
	gpiAppendUShortToBuffer(connection, &iconnection->outputBuffer, hostPort);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\sessflags\\");
	gpiAppendUIntToBuffer(connection, &iconnection->outputBuffer, sessionFlags);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\richStatus\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, richStatus);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\gameType\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, gameTypeFixed);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\gameVariant\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, gameVariantFixed);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\gameMapName\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, gameMapNameFixed);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");

	GSI_UNUSED(richStatusLen);
	GSI_UNUSED(gameTypeLen);
	GSI_UNUSED(gameVariantLen);
	GSI_UNUSED(gameMapNameLen);
	return GP_NO_ERROR;
}

#ifdef GSI_UNICODE
GPResult gpSetStatusInfoW(
						  GPConnection *connection, 
						  GPEnum statusState,
						  unsigned int hostIp, 
						  unsigned int hostPrivateIp,
						  unsigned short queryPort,
						  unsigned short hostPort,
						  unsigned int sessionFlags,
						  const unsigned short *richStatus,
						  int richStatusLen,
						  const unsigned short *gameType,
						  int gameTypeLen,
						  const unsigned short *gameVariant,
						  int gameVariantLen,
						  const unsigned short *gameMapName,
						  int gameMapNameLen
						  )
{
	char *richStatus_A, *gameType_A, *gameVariant_A, *gameMapName_A;
	GPResult aResult;
	GPIConnection * iconnection;
	GS_ASSERT(connection != NULL);
	GS_ASSERT(richStatusLen <= GP_RICH_STATUS_LEN);
	GS_ASSERT(gameTypeLen <= GP_STATUS_BASIC_STR_LEN);
	GS_ASSERT(gameVariantLen <= GP_STATUS_BASIC_STR_LEN);
	GS_ASSERT(gameMapNameLen <= GP_STATUS_BASIC_STR_LEN);
	GS_ASSERT(richStatus != NULL);

	if (connection != NULL && (*connection != NULL))
		iconnection = (GPIConnection *)*connection;
	else return GP_PARAMETER_ERROR;

	if (richStatus == NULL)
		Error(connection, GP_PARAMETER_ERROR, "Invalid richStatus.");

	if (richStatusLen <= GP_RICH_STATUS_LEN)
		richStatus_A = UCS2ToUTF8StringAlloc(richStatus);
	else 
		richStatus_A = UCS2ToUTF8StringAlloc((UCS2String)_T(""));
	if (gameType && (gameTypeLen <= GP_STATUS_BASIC_STR_LEN))
		gameType_A = UCS2ToUTF8StringAlloc(gameType);
	else 
		gameType_A = UCS2ToUTF8StringAlloc((UCS2String)_T(""));
	if (gameVariant && (gameVariantLen <= GP_STATUS_BASIC_STR_LEN))
		gameVariant_A = UCS2ToUTF8StringAlloc(gameVariant);
	else 
		gameVariant_A = UCS2ToUTF8StringAlloc((UCS2String)_T(""));
	if (gameMapName && (gameMapNameLen <= GP_STATUS_BASIC_STR_LEN))
		gameMapName_A = UCS2ToUTF8StringAlloc(gameMapName);
	else 
		gameMapName_A = UCS2ToUTF8StringAlloc((UCS2String)_T(""));

	if ((statusState == iconnection->lastStatusState) &&
		(sessionFlags == iconnection->sessionFlags) &&
		(hostIp == iconnection->hostIp) &&
		(hostPrivateIp == iconnection->hostPrivateIp) &&
		(queryPort == iconnection->queryPort) &&
		(hostPort == iconnection->hostPort) &&
		(_tcscmp(richStatus, iconnection->richStatus_W) == 0) &&
		(_tcscmp(gameType, iconnection->gameType_W) == 0) &&
		(_tcscmp(gameVariant, iconnection->gameVariant_W) == 0) &&
		(_tcscmp(gameMapName, iconnection->gameMapName_W) == 0))
	{
		return GP_NO_ERROR;
	}
	_tcsncpy(iconnection->richStatus_W, richStatus, GP_RICH_STATUS_LEN);
	_tcsncpy(iconnection->gameType_W, gameType, GP_STATUS_BASIC_STR_LEN);
	_tcsncpy(iconnection->gameVariant_W, gameVariant, GP_STATUS_BASIC_STR_LEN);
	_tcsncpy(iconnection->gameMapName_W, gameMapName, GP_STATUS_BASIC_STR_LEN);

	aResult = gpSetStatusInfoA(connection, statusState, hostIp, hostPrivateIp, queryPort, hostPort, 
		sessionFlags, richStatus_A, (int)strlen(richStatus_A), gameType_A, (int)strlen(gameType_A), 
		gameVariant_A, (int)strlen(gameVariant_A), gameMapName_A, (int)strlen(gameMapName_A));
	gsifree(richStatus_A);
	gsifree(gameType_A);
	gsifree(gameVariant_A);
	gsifree(gameMapName_A);
	return aResult;
}
#endif

GPResult gpAddStatusInfoKeyA(GPConnection *connection, const char *keyName, const char *keyValue)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if ((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if ( iconnection->simulation )
		return GP_NO_ERROR;

	// Check for disconnected.
	//////////////////////////
	if (iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	return gpiStatusInfoAddKey(connection, iconnection->extendedInfoKeys, keyName, keyValue);
}

#ifdef GSI_UNICODE
GPResult gpAddStatusInfoKeyW(GPConnection *connection, const unsigned short *keyName, const unsigned short *keyValue)
{
	GPResult aResult;
	char *keyName_A = UCS2ToUTF8StringAlloc(keyName);
	char *keyValue_A = UCS2ToUTF8StringAlloc(keyValue);
	aResult = gpAddStatusInfoKeyA(connection, keyName_A, keyValue_A);
	gsifree(keyName_A);
	gsifree(keyValue_A);
	return aResult;
}
#endif

GPResult gpSetStatusInfoKeyA(GPConnection *connection, const char *keyName, const char *keyValue)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if ((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if ( iconnection->simulation )
		return GP_NO_ERROR;

	// Check for disconnected.
	//////////////////////////
	if (iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	return gpiStatusInfoSetKey(connection, iconnection->extendedInfoKeys, keyName, keyValue);
}

#ifdef GSI_UNICODE
GPResult gpSetStatusInfoKeyW(GPConnection *connection, const unsigned short *keyName, const unsigned short *keyValue)
{
	GPResult aResult;
	char *keyName_A = UCS2ToUTF8StringAlloc(keyName);
	char *keyValue_A = UCS2ToUTF8StringAlloc(keyValue);
	aResult = gpSetStatusInfoKeyA(connection, keyName_A, keyValue_A);
	gsifree(keyName_A);
	gsifree(keyValue_A);
	return aResult;
}
#endif

GPResult gpDelStatusInfoKeyA(GPConnection *connection, const char *keyName)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if ((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if ( iconnection->simulation )
		return GP_NO_ERROR;

	// Check for disconnected.
	//////////////////////////
	if (iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	return gpiStatusInfoDelKey(connection, iconnection->extendedInfoKeys, keyName);
}

#ifdef GSI_UNICODE
GPResult gpDelStatusInfoKeyW(GPConnection *connection, const unsigned short *keyName)
{
	GPResult aResult;
	char *keyName_A = UCS2ToUTF8StringAlloc(keyName);
	aResult = gpDelStatusInfoKeyA(connection, keyName_A);
	gsifree(keyName_A);
	return aResult;
}
#endif

GPResult gpGetStatusInfoKeyValA(GPConnection *connection, const char *keyName, char **keyValue)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if ((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if ( iconnection->simulation )
		return GP_NO_ERROR;

	// Check for disconnected.
	//////////////////////////
	if (iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	return gpiStatusInfoGetKey(connection, iconnection->extendedInfoKeys, keyName, keyValue);
}
#ifdef GSI_UNICODE
GPResult gpGetStatusInfoKeyValW(GPConnection *connection, const unsigned short *keyName, unsigned short **keyValue)
{
	GPResult aResult;
	char *keyValue_A;
	
	char *keyName_A = UCS2ToUTF8StringAlloc(keyName);
	
	aResult = gpGetStatusInfoKeyValA(connection, keyName_A, &keyValue_A);
	*keyValue = UTF8ToUCS2StringAlloc(keyValue_A);

	gsifree(keyName_A);
	gsifree(keyValue_A);
	return aResult;
}
#endif



GPResult gpGetBuddyStatusInfoKeys(GPConnection *connection, int index, GPCallback callback, void *userData)
{
	GPIConnection *iconnection;
	GPIProfile * pProfile;
	GPResult aResult;
	GPIPeerOp *aPeerOp;
	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Check for no callback.
	/////////////////////////
	if(callback == NULL)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
		return GP_NO_ERROR;

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	pProfile = gpiFindBuddy(connection, index);
	if (!pProfile)
		Error(connection, GP_PARAMETER_ERROR, "Invalid index.");

	if (pProfile->buddyStatus)
		CallbackError(connection, GP_PARAMETER_ERROR, GP_BM_EXT_INFO_NOT_SUPPORTED, "The profile does not support extended info keys.")
		
	if (!pProfile->buddyStatusInfo && !pProfile->buddyStatus)
		CallbackError(connection, GP_PARAMETER_ERROR, GP_BM_NOT_BUDDY, "The profile used to get extended info keys is not a buddy.")
	
	if (pProfile->buddyStatusInfo && pProfile->buddyStatusInfo->statusState == GP_OFFLINE)
		CallbackError(connection, GP_NETWORK_ERROR, GP_BM_BUDDY_OFFLINE, "The profile used to get extended info keys is offline.");

	aPeerOp = (GPIPeerOp *)gsimalloc(sizeof(GPIPeerOp));
	aPeerOp->callback = callback;
	aPeerOp->next = NULL;
	aPeerOp->state = GPI_PEER_OP_STATE_REQUESTED;
	aPeerOp->type = GPI_BM_KEYS_REQUEST;
	aPeerOp->userData = userData;
	aPeerOp->timeout = current_time() + GPI_PEER_OP_TIMEOUT;
	aResult = gpiSendBuddyMessage(connection, pProfile->profileId, GPI_BM_KEYS_REQUEST, "Keys?", GP_DONT_ROUTE, aPeerOp);
	return aResult;
}
#endif

GPResult gpSendBuddyMessageA(
  GPConnection * connection,
  GPProfile profile,
  const char * message
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
		return GP_NO_ERROR;

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	// Error check.
	///////////////
	if(message == NULL)
		Error(connection, GP_PARAMETER_ERROR, "Invalid message.");

	return gpiSendBuddyMessage(connection, profile, GPI_BM_MESSAGE, message, 0, NULL);
}
#ifdef GSI_UNICODE
GPResult gpSendBuddyMessageW(
  GPConnection * connection,
  GPProfile profile,
  const unsigned short* message
)
{
	char* message_A;
	GPResult result;

	assert(message != NULL);
	message_A = UCS2ToUTF8StringAlloc(message);						// convert to UTF8
	result = gpSendBuddyMessageA(connection, profile, message_A);	// send
	gsifree(message_A);	// free the converted string
	return result;
}
#endif

GPResult gpSendBuddyUTMA(
  GPConnection * connection,
  GPProfile profile,
  const char * message,
  int sendOption
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
		return GP_NO_ERROR;

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	// Error check.
	///////////////
	if(message == NULL)
		Error(connection, GP_PARAMETER_ERROR, "Invalid message.");

	return gpiSendBuddyMessage(connection, profile, GPI_BM_UTM, message, sendOption, NULL);
}

#ifdef GSI_UNICODE
GPResult gpSendBuddyUTMW(
 GPConnection * connection,
 GPProfile profile,
 const unsigned short* message,
 int sendOption
)
{
	char* message_A;
	GPResult result;

	assert(message != NULL);
	message_A = UCS2ToUTF8StringAlloc(message);						// convert to UTF8
	result = gpSendBuddyUTMA(connection, profile, message_A, sendOption);	// send
	gsifree(message_A);	// free the converted string
	return result;
}
#endif

GPResult gpIsValidEmailA(
  GPConnection * connection,
  const char email[GP_EMAIL_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Check for no callback.
	/////////////////////////
	if(callback == NULL)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");

	// Check the length of the email.
	/////////////////////////////////
	if(strlen(email) >= GP_EMAIL_LEN)
		Error(connection, GP_PARAMETER_ERROR, "Email too long.");

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
	{
		GPIsValidEmailResponseArg arg;
		memset(&arg, 0, sizeof(arg));
#ifndef GSI_UNICODE
		strzcpy(arg.email, email, GP_EMAIL_LEN);
#else
		UTF8ToUCS2String(email, arg.email);
#endif
		arg.isValid = GP_INVALID;
		callback(connection, &arg, param);
		return GP_NO_ERROR;
	}

	// Do the validation.
	/////////////////////
	return gpiIsValidEmail(connection, email, blocking, callback, param);
}
#ifdef GSI_UNICODE
GPResult gpIsValidEmailW(
  GPConnection * connection,
  const unsigned short email[GP_EMAIL_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	char email_A[GP_EMAIL_LEN];
	UCS2ToAsciiString(email, email_A);
	return gpIsValidEmailA(connection, email_A, blocking,callback, param);
}
#endif

GPResult gpGetUserNicksA(
  GPConnection * connection,
  const char email[GP_EMAIL_LEN],
  const char password[GP_PASSWORD_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Check for no callback.
	/////////////////////////
	if(callback == NULL)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");

	// Check the length of the email.
	/////////////////////////////////
	if(strlen(email) >= GP_EMAIL_LEN)
		Error(connection, GP_PARAMETER_ERROR, "Email too long.");

	// Check the length of the password.
	////////////////////////////////////
	if(strlen(password) >= GP_PASSWORD_LEN)
		Error(connection, GP_PARAMETER_ERROR, "Password too long.");

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
	{
		GPGetUserNicksResponseArg arg;
		memset(&arg, 0, sizeof(arg));
#ifndef GSI_UNICODE
		strzcpy(arg.email, email, GP_EMAIL_LEN);
#else
		AsciiToUCS2String(email, arg.email);
#endif
		callback(connection, &arg, param);
		return GP_NO_ERROR;
	}

	// Do the validation.
	/////////////////////
	return gpiGetUserNicks(connection, email, password, blocking, callback, param);
}
#ifdef GSI_UNICODE
GPResult gpGetUserNicksW(
  GPConnection * connection,
  const unsigned short email[GP_EMAIL_LEN],
  const unsigned short password[GP_PASSWORD_LEN],
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
  char email_A[GP_EMAIL_LEN];
  char password_A[GP_PASSWORD_LEN];
  UCS2ToAsciiString(email, email_A);
  UCS2ToAsciiString(password, password_A);
  return gpGetUserNicksA(connection, email_A, password_A, blocking, callback, param);
}
#endif

GPResult gpSetInvitableGames(
  GPConnection * connection,
  int numProductIDs,
  const int * productIDs
)
{
	GPIConnection * iconnection;
	int i;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
		return GP_NO_ERROR;

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	// Error check.
	///////////////
	if(numProductIDs < 0)
		Error(connection, GP_PARAMETER_ERROR, "Invalid numProductIDs.");
	if((numProductIDs > 0) && (productIDs == NULL))
		Error(connection, GP_PARAMETER_ERROR, "Invalid productIDs.");

	// Send the list.
	/////////////////
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\inviteto\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\sesskey\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->sessKey);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\products\\");
	for(i = 0 ; i < numProductIDs ; i++)
	{
		gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, productIDs[i]);
		if(i < (numProductIDs - 1))
			gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, ",");
	}
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");

	return GP_NO_ERROR;
}

GPResult gpFindPlayers(
  GPConnection * connection,
  int productID,
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Check for no callback.
	/////////////////////////
	if(callback == NULL)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
	{
		GPFindPlayersResponseArg arg;
		memset(&arg, 0, sizeof(arg));
		arg.productID = productID;
		arg.numMatches = 0;
		arg.matches = NULL;
		callback(connection, &arg, param);
		return GP_NO_ERROR;
	}

	// Start the find.
	//////////////////
	return gpiFindPlayers(connection, productID, blocking, callback, param);
}

GPResult gpInvitePlayerA(
  GPConnection * connection,
  GPProfile profile,
  int productID,
  const char location[GP_LOCATION_STRING_LEN]
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
		return GP_NO_ERROR;

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	// Send the invite.
	///////////////////
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\pinvite\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\sesskey\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->sessKey);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\profileid\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, profile);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\productid\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, productID);

	if (location && location[0])
	{
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\location\\");
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, location);
	}

	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");

	return GP_NO_ERROR;
}
#ifdef GSI_UNICODE
GPResult gpInvitePlayerW(
  GPConnection * connection,
  GPProfile profile,
  int productID,
  const gsi_char location[GP_LOCATION_STRING_LEN]
)
{
	char location_A[GP_LOCATION_STRING_LEN];
	UCS2ToAsciiString(location, location_A);
	return gpInvitePlayerA(connection, profile, productID, location_A);
}
#endif

GPResult gpGetReverseBuddies(
  GPConnection * connection,
  GPEnum blocking,
  GPCallback callback,
  void * param
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Check for no callback.
	/////////////////////////
	if(callback == NULL)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
	{
		GPGetReverseBuddiesResponseArg arg;
		memset(&arg, 0, sizeof(arg));
		callback(connection, &arg, param);
		return GP_NO_ERROR;
	}

	// Start the search.
	////////////////////
	return gpiOthersBuddy(connection, blocking, callback, param);
}

GPResult gpGetReversBuddiesList( GPConnection * connection,
	GPProfile *targets, int numOfTargets, 
	GPEnum blocking,
	GPCallback callback,
	void * param)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Check for no callback.
	/////////////////////////
	if(callback == NULL)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
	{
		GPGetReverseBuddiesListResponseArg arg;
		memset(&arg, 0, sizeof(arg));
		callback(connection, &arg, param);
		return GP_NO_ERROR;
	}

	// Start the search.
	////////////////////
	return gpiOthersBuddyList(connection, targets, numOfTargets, blocking, callback, param);

}


GPResult gpRevokeBuddyAuthorization(
  GPConnection * connection,
  GPProfile profile
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
		return GP_NO_ERROR;

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	// Send the invite.
	///////////////////
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\revoke\\");
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\sesskey\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->sessKey);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\profileid\\");
	gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, profile);
	gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");

	return GP_NO_ERROR;
}


GPResult gpGetLoginTicket(
  GPConnection * connection,
  char loginTicket[GP_LOGIN_TICKET_LEN]
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;
	memcpy(loginTicket, iconnection->loginTicket, GP_LOGIN_TICKET_LEN);
	return GP_NO_ERROR;
}

GPResult gpSetQuietMode(
  GPConnection * connection,
  GPEnum flags
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Store the flags locally.
	///////////////////////////
	iconnection->quietModeFlags = flags;

	// Check for a connection.
	//////////////////////////
	if(iconnection->connectState == GPI_CONNECTED)
	{
		// Send the flags.
		//////////////////
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\quiet\\");
		gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->quietModeFlags);
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\sesskey\\");
		gpiAppendIntToBuffer(connection, &iconnection->outputBuffer, iconnection->sessKey);
		gpiAppendStringToBuffer(connection, &iconnection->outputBuffer, "\\final\\");
	}

	return GP_NO_ERROR;
}

#ifndef NOFILE
void gpSetInfoCacheFilenameA(
  const char * filename
)
{
	gpiSetInfoCacheFilename(filename);
}
void gpSetInfoCacheFilenameW(
  const unsigned short * filename
)
{
	char* filename_A = UCS2ToUTF8StringAlloc(filename);
	gpiSetInfoCacheFilename(filename_A);
	gsifree(filename_A);
}

static GPResult gpiAddSendingFileA(
  GPConnection * connection,
  GPITransfer * transfer,
  const char * path,
  const char * name
)
{
	GPIFile * file = NULL;
	int size = 0;
	gsi_time modTime = 0;

	// Check for a bad path or name.
	////////////////////////////////
	if(!path && !name)
		Error(connection, GP_PARAMETER_ERROR, "File missing path and name.");
	if(path && !path[0])
		Error(connection, GP_PARAMETER_ERROR, "Empty path.");
	if(name && !name[0])
		Error(connection, GP_PARAMETER_ERROR, "Empty name.");

	// Check that the file exists and is readable.
	//////////////////////////////////////////////
	if(path)
	{
		FILE * fileVerify;

		fileVerify = fopen(path, "r");
		if(!fileVerify)
			Error(connection, GP_PARAMETER_ERROR, "Can't find file.");

		if(!gpiGetTransferFileInfo(fileVerify, &size, &modTime))
		{
			fclose(fileVerify);
			Error(connection, GP_PARAMETER_ERROR, "Can't get info on file.");
		}

		fclose(fileVerify);
	}

	// Validate the name.
	/////////////////////
	if(name)
	{
		size_t len;

		len = strlen(name);

		if(strstr(name, "//") || strstr(name, "\\\\"))
			Error(connection, GP_PARAMETER_ERROR, "Empty directory in filename.");
		if(strstr(name, "./") || strstr(name, ".\\") || (name[len - 1] == '.'))
			Error(connection, GP_PARAMETER_ERROR, "Directory level in filename.");
		if((name[0] == '/') || (name[0] == '\\'))
			Error(connection, GP_PARAMETER_ERROR, "Filename can't start with a slash.");
		if(strcspn(name, ":*?\"<>|\n") != len)
			Error(connection, GP_PARAMETER_ERROR, "Invalid character in filename.");
	}
	// The name is the path's title.
	////////////////////////////////
	else
	{
		const char * str;

		// Find the end of the path.
		////////////////////////////
		name = strrchr(path, '/');
		str = strrchr(path, '\\');
		if(str > name)
			name = str;

		// Point the name at the title.
		///////////////////////////////
		if(name)
			name++;
		else
			name = path;
	}

	// Add this to the list.
	////////////////////////
	file = gpiAddFileToTransfer(transfer, path, name);
	if(!file)
		Error(connection, GP_MEMORY_ERROR, "Out of memory.");

	// Set the size and time.
	/////////////////////////
	file->size = size;
	file->modTime = modTime;

	// Update the total size.
	/////////////////////////
	transfer->totalSize += size;

	return GP_NO_ERROR;
}
#ifdef GSI_UNICODE
static GPResult gpiAddSendingFileW(
  GPConnection * connection,
  GPITransfer * transfer,
  const unsigned short * path,
  const unsigned short * name
)
{
	char* path_A = UCS2ToUTF8StringAlloc(path);
	char* name_A = UCS2ToUTF8StringAlloc(name);
	GPResult result = gpiAddSendingFileA(connection, transfer, path_A, name_A);
	gsifree(path_A);
	gsifree(name_A);
	return result;
}
#endif


GPResult gpSendFilesA(
  GPConnection * connection,
  GPTransfer * transfer,
  GPProfile profile,
  const char * message,
  gpSendFilesCallback callback,
  void * param
)
{
	GPIConnection * iconnection;
	GPITransfer * pTransfer;
	GPResult result;
	const gsi_char * path;
	const gsi_char * name;
	int numFiles;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Check for simulation mode.
	/////////////////////////////
	if(iconnection->simulation)
		Error(connection, GP_PARAMETER_ERROR, "Cannot send files in simulation mode.");

	// Check for disconnected.
	//////////////////////////
	if(iconnection->connectState == GPI_DISCONNECTED)
		Error(connection, GP_PARAMETER_ERROR, "The connection has already been disconnected.");

	// Check other stuff.
	/////////////////////
	if(!callback)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");
	if(!iconnection->callbacks[GPI_TRANSFER_CALLBACK].callback)
		Error(connection, GP_PARAMETER_ERROR, "No callback.");

	// No message is an empty message.
	//////////////////////////////////
	if(!message)
		message = "";

	// Create the transfer object.
	//////////////////////////////
	CHECK_RESULT(gpiNewSenderTransfer(connection, &pTransfer, profile));

	// Fill in the message.
	///////////////////////
	pTransfer->message = goastrdup(message);
	if(!pTransfer->message)
	{
		gpiFreeTransfer(connection, pTransfer);
		Error(connection, GP_MEMORY_ERROR, "Out of memory.");
	}

	// Add all the files.
	/////////////////////
	numFiles = 0;
	do
	{
		path = NULL;
		name = NULL;
		callback(connection, numFiles++, &path, &name, param);
		if(path && !path[0])
			path = NULL;
		if(name && !name[0])
			name = NULL;

		if(name || path)
		{
#ifndef GSI_UNICODE
			result = gpiAddSendingFileA(connection, pTransfer, path, name);
#else
			result = gpiAddSendingFileW(connection, pTransfer, path, name);
#endif
			if(result != GP_NO_ERROR)
			{
				gpiFreeTransfer(connection, pTransfer);
				return result;
			}
		}
	}
	while(name || path);

	// Check that we got at least 1 file.
	/////////////////////////////////////
	if(!ArrayLength(pTransfer->files))
	{
		gpiFreeTransfer(connection, pTransfer);
		Error(connection, GP_PARAMETER_ERROR, "No files to send.");
	}

	// Ping the receiver.
	/////////////////////
	result = gpiSendBuddyMessage(connection, profile, GPI_BM_PING, "1", 0, NULL);
	if(result != GP_NO_ERROR)
	{
		gpiFreeTransfer(connection, pTransfer);
		return result;
	}

	// Successful so far.
	/////////////////////
	if(transfer)
		*transfer = pTransfer->localID;

	return GP_NO_ERROR;
}
GPResult gpSendFilesW(
  GPConnection * connection,
  GPTransfer * transfer,
  GPProfile profile,
  const unsigned short* message,
  gpSendFilesCallback callback,
  void * param
)
{
	char* message_A = NULL;
	GPResult result;
	
	if (message == NULL)
		return gpSendFilesA(connection, transfer, profile, NULL, callback, param);
	
	message_A = UCS2ToUTF8StringAlloc(message);
	result = gpSendFilesA(connection, transfer, profile, message_A, callback, param);
	gsifree(message_A);
	return result;
}

GPResult gpAcceptTransferA(
  GPConnection * connection,
  GPTransfer transfer,
  const char * message
)
{
	GPITransfer * pTransfer;
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection*)*connection;

	// Get the transfer.
	////////////////////
	pTransfer = gpiFindTransferByLocalID(connection, transfer);
	if(!pTransfer)
		Error(connection, GP_PARAMETER_ERROR, "Invalid transfer.");

	// Check that we have a directory set.
	//////////////////////////////////////
	if(!pTransfer->baseDirectory)
		Error(connection, GP_PARAMETER_ERROR, "No transfer directory set.");

	// Check if this transfer has been cancelled.
	/////////////////////////////////////////////
	if(pTransfer->state & GPITransferCancelled)
		Error(connection, GP_PARAMETER_ERROR, "Transfer already cancelled.");

	// Send a reply.
	////////////////
	CHECK_RESULT(gpiSendTransferReply(connection, &pTransfer->transferID, pTransfer->peer, GPI_ACCEPTED, message));

	// We're now transferring.
	//////////////////////////
	pTransfer->state = GPITransferTransferring;

	// Set the current file index to the first file.
	////////////////////////////////////////////////
	pTransfer->currentFile = 0;

	return GP_NO_ERROR;
}
GPResult gpAcceptTransferW(
  GPConnection * connection,
  GPTransfer transfer,
  const unsigned short * message
)
{
	char* message_A = NULL;
	GPResult result;
	
	if (message == NULL)
		return gpAcceptTransferA(connection, transfer, NULL);
	
	message_A = UCS2ToUTF8StringAlloc(message);
	result = gpAcceptTransferA(connection, transfer, message_A);
	gsifree(message_A);
	return result;
}
	

GPResult gpRejectTransferA(
  GPConnection * connection,
  GPTransfer transfer,
  const char * message
)
{
	GPITransfer * pTransfer;

	// Get the transfer.
	////////////////////
	pTransfer = gpiFindTransferByLocalID(connection, transfer);
	if(!pTransfer)
		return GP_NO_ERROR;

	// Check if this transfer has been cancelled.
	/////////////////////////////////////////////
	if(pTransfer->state & GPITransferCancelled)
		return GP_NO_ERROR;

	// Send the reply.
	//////////////////
	gpiSendTransferReply(connection, &pTransfer->transferID, pTransfer->peer, GPI_REJECTED, message);

	// Free the transfer.
	/////////////////////
	gpiFreeTransfer(connection, pTransfer);

	return GP_NO_ERROR;
}
GPResult gpRejectTransferW(
  GPConnection * connection,
  GPTransfer transfer,
  const unsigned short* message
)
{
	char* message_A = NULL;
	GPResult result;
	
	if (message == NULL)
		return gpRejectTransferA(connection, transfer, NULL);
	
	message_A = UCS2ToUTF8StringAlloc(message);
	result = gpRejectTransferA(connection, transfer, message_A);
	gsifree(message_A);
	return result;
}

GPResult gpFreeTransfer(
  GPConnection * connection,
  GPTransfer transfer
)
{
	GPITransfer * pTransfer;

	// Get the transfer.
	////////////////////
	pTransfer = gpiFindTransferByLocalID(connection, transfer);
	if(!pTransfer)
		return GP_NO_ERROR;

	// Check if this should be a reject.
	////////////////////////////////////
	if(!pTransfer->sender && (pTransfer->state == GPITransferWaiting))
		return gpRejectTransfer(connection, transfer, NULL);

	// Check for cancelling.
	////////////////////////
	if(pTransfer->state < GPITransferComplete)
		gpiCancelTransfer(connection, pTransfer);

	// Free the transfer.
	/////////////////////
	gpiFreeTransfer(connection, pTransfer);

	return GP_NO_ERROR;
}

GPResult gpSetTransferData(
  GPConnection * connection,
  GPTransfer transfer,
  void * userData
)
{
	GPITransfer * pTransfer;

	// Get the transfer.
	////////////////////
	pTransfer = gpiFindTransferByLocalID(connection, transfer);
	if(!pTransfer)
		Error(connection, GP_PARAMETER_ERROR, "Invalid transfer.");

	// Set the data.
	////////////////
	pTransfer->userData = userData;

	return GP_NO_ERROR;
}

void * gpGetTransferData(
  GPConnection * connection,
  GPTransfer transfer
)
{
	GPITransfer * pTransfer;

	// Get the transfer.
	////////////////////
	pTransfer = gpiFindTransferByLocalID(connection, transfer);
	if(!pTransfer)
		return NULL;

	// Return the data.
	///////////////////
	return pTransfer->userData;
}

GPResult gpSetTransferDirectoryA(
  GPConnection * connection,
  GPTransfer transfer,
  const char * directory
)
{
	GPITransfer * pTransfer;
	char lastChar;

	if(!directory || !directory[0])
		Error(connection, GP_PARAMETER_ERROR, "Invalid directory.");
	lastChar = directory[strlen(directory) - 1];
	if((lastChar != '\\') && (lastChar != '/'))
		Error(connection, GP_PARAMETER_ERROR, "Invalid directory.");

	// Get the transfer.
	////////////////////
	pTransfer = gpiFindTransferByLocalID(connection, transfer);
	if(!pTransfer)
		Error(connection, GP_PARAMETER_ERROR, "Invalid transfer.");

	// This has to be set before transferring.
	//////////////////////////////////////////
	if(pTransfer->sender)
		Error(connection, GP_PARAMETER_ERROR, "Sender has no transfer directory.");
	if(pTransfer->state != GPITransferWaiting)
		Error(connection, GP_PARAMETER_ERROR, "Can only set transfer directory before transferring.");

	// Free any existing directory.
	///////////////////////////////
	if(pTransfer->baseDirectory)
		gsifree(pTransfer->baseDirectory);
	pTransfer->baseDirectory = NULL;

	// Set the directory.
	/////////////////////
	pTransfer->baseDirectory = goastrdup(directory);
	if(!pTransfer->baseDirectory)
		Error(connection, GP_MEMORY_ERROR, "Out of memory.");

	return GP_NO_ERROR;
}
GPResult gpSetTransferDirectoryW(
  GPConnection * connection,
  GPTransfer transfer,
  const unsigned short * directory
)
{
	char* directory_A = UCS2ToUTF8StringAlloc(directory);
	GPResult result = gpSetTransferDirectoryA(connection, transfer, directory_A);
	gsifree(directory_A);
	return result;
}

GPResult gpSetTransferThrottle(
  GPConnection * connection,
  GPTransfer transfer,
  int throttle
)
{
	GPITransfer * pTransfer;
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection*)*connection;

	// Negative means no throttle.
	//////////////////////////////
	if(throttle < 0)
		throttle = -1;

	// Get the transfer.
	////////////////////
	pTransfer = gpiFindTransferByLocalID(connection, transfer);
	if(!pTransfer)
		Error(connection, GP_PARAMETER_ERROR, "Invalid transfer.");

	// Store the throttle setting.
	//////////////////////////////
	pTransfer->throttle = throttle;

	// Send the rate.
	/////////////////
	CHECK_RESULT(gpiPeerStartTransferMessage(connection, pTransfer->peer, GPI_BM_FILE_TRANSFER_THROTTLE, (GPITransferID_st)&pTransfer->transferID));
	gpiSendOrBufferString(connection, pTransfer->peer, "\\rate\\");
	gpiSendOrBufferInt(connection, pTransfer->peer, throttle);
	gpiPeerFinishTransferMessage(connection, pTransfer->peer, NULL, 0);

	// If we're the sender, call the callback.
	//////////////////////////////////////////
	if(pTransfer->sender)
	{
		GPTransferCallbackArg * arg;

		// Call the callback.
		/////////////////////
		arg = (GPTransferCallbackArg *)gsimalloc(sizeof(GPTransferCallbackArg));
		if(arg)
		{
			memset(arg, 0, sizeof(GPTransferCallbackArg));
			arg->transfer = pTransfer->localID;
			arg->type = GP_TRANSFER_THROTTLE;
			arg->num = throttle;
			gpiAddCallback(connection, iconnection->callbacks[GPI_TRANSFER_CALLBACK], arg, NULL, GPI_ADD_TRANSFER_CALLBACK);
		}
	}

	return GP_NO_ERROR;
}

GPResult gpGetTransferThrottle(
  GPConnection * connection,
  GPTransfer transfer,
  int * throttle
)
{
	GPITransfer * pTransfer;

	// Get the transfer.
	////////////////////
	pTransfer = gpiFindTransferByLocalID(connection, transfer);
	if(!pTransfer)
		Error(connection, GP_PARAMETER_ERROR, "Invalid transfer.");

	// Get the throttle.
	////////////////////
	*throttle = pTransfer->throttle;

	return GP_NO_ERROR;
}

GPResult gpGetTransferProfile(
  GPConnection * connection,
  GPTransfer transfer,
  GPProfile * profile
)
{
	GPITransfer * pTransfer;

	// Get the transfer.
	////////////////////
	pTransfer = gpiFindTransferByLocalID(connection, transfer);
	if(!pTransfer)
		Error(connection, GP_PARAMETER_ERROR, "Invalid transfer.");

	// Get the profile.
	///////////////////
	*profile = pTransfer->profile;

	return GP_NO_ERROR;
}

GPResult gpGetTransferSide(
  GPConnection * connection,
  GPTransfer transfer,
  GPEnum * side
)
{
	GPITransfer * pTransfer;

	// Get the transfer.
	////////////////////
	pTransfer = gpiFindTransferByLocalID(connection, transfer);
	if(!pTransfer)
		Error(connection, GP_PARAMETER_ERROR, "Invalid transfer.");

	// Get the side.
	////////////////
	if(pTransfer->sender)
		*side = GP_TRANSFER_SENDER;
	else
		*side = GP_TRANSFER_RECEIVER;

	return GP_NO_ERROR;
}

GPResult gpGetTransferSize(
  GPConnection * connection,
  GPTransfer transfer,
  int * size
)
{
	GPITransfer * pTransfer;

	// Get the transfer.
	////////////////////
	pTransfer = gpiFindTransferByLocalID(connection, transfer);
	if(!pTransfer)
		Error(connection, GP_PARAMETER_ERROR, "Invalid transfer.");

	// Get the size.
	////////////////
	*size = pTransfer->totalSize;

	return GP_NO_ERROR;
}

GPResult gpGetTransferProgress(
  GPConnection * connection,
  GPTransfer transfer,
  int * progress
)
{
	GPITransfer * pTransfer;

	// Get the transfer.
	////////////////////
	pTransfer = gpiFindTransferByLocalID(connection, transfer);
	if(!pTransfer)
		Error(connection, GP_PARAMETER_ERROR, "Invalid transfer.");

	// Get the progress.
	////////////////////
	*progress = pTransfer->progress;

	return GP_NO_ERROR;
}

GPResult gpGetNumFiles(
  GPConnection * connection,
  GPTransfer transfer,
  int * num
)
{
	GPITransfer * pTransfer;

	// Get the transfer.
	////////////////////
	pTransfer = gpiFindTransferByLocalID(connection, transfer);
	if(!pTransfer)
		Error(connection, GP_PARAMETER_ERROR, "Invalid transfer.");

	// Get the progress.
	////////////////////
	*num = ArrayLength(pTransfer->files);

	return GP_NO_ERROR;
}

GPResult gpGetCurrentFile(
  GPConnection * connection,
  GPTransfer transfer,
  int * index
)
{
	GPITransfer * pTransfer;

	// Get the transfer.
	////////////////////
	pTransfer = gpiFindTransferByLocalID(connection, transfer);
	if(!pTransfer)
		Error(connection, GP_PARAMETER_ERROR, "Invalid transfer.");

	// Get the current file.
	////////////////////////
	*index = pTransfer->currentFile;

	return GP_NO_ERROR;
}

GPResult gpSkipFile(
  GPConnection * connection,
  GPTransfer transfer,
  int index
)
{
	GPIFile * file;
	GPITransfer * pTransfer;
	GPTransferCallbackArg * arg;
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection*)*connection;

	// Get the transfer.
	////////////////////
	pTransfer = gpiFindTransferByLocalID(connection, transfer);
	if(!pTransfer)
		Error(connection, GP_PARAMETER_ERROR, "Invalid transfer.");

	// Get the file.
	////////////////
	if((index < 0) || (index >= ArrayLength(pTransfer->files)))
		Error(connection, GP_PARAMETER_ERROR, "Invalid index.");
	file = (GPIFile *)ArrayNth(pTransfer->files, index);

	// Are we already past this file?
	/////////////////////////////////
	if(index < pTransfer->currentFile)
		return GP_NO_ERROR;

	// Did we not get to this file yet?
	///////////////////////////////////
	if(pTransfer->currentFile != index)
	{
		// Mark it.
		///////////
		file->flags |= GPI_FILE_SKIP;

		// If we're receiving, let the sender know we want to skip it.
		//////////////////////////////////////////////////////////////
		if(!pTransfer->sender)
			gpiSkipFile(connection, pTransfer, index, GPI_SKIP_USER_SKIP);

		return GP_NO_ERROR;
	}

	// If we're receiving, delete our temp file.
	////////////////////////////////////////////
	if(!pTransfer->sender && (index == pTransfer->currentFile) && file->file)
	{
		fclose(file->file);
		file->file = NULL;
		remove(file->path);
	}

	// Skip the current file.
	/////////////////////////
	gpiSkipCurrentFile(connection, pTransfer, GPI_SKIP_USER_SKIP);

	// Call the callback.
	/////////////////////
	arg = (GPTransferCallbackArg *)gsimalloc(sizeof(GPTransferCallbackArg));
	if(arg)
	{
		memset(arg, 0, sizeof(GPTransferCallbackArg));
		arg->transfer = pTransfer->localID;
		arg->index = index;
		arg->type = GP_FILE_SKIP;
		gpiAddCallback(connection, iconnection->callbacks[GPI_TRANSFER_CALLBACK], arg, NULL, GPI_ADD_TRANSFER_CALLBACK);
	}

	return GP_NO_ERROR;
}

GPResult gpGetFileName(
  GPConnection * connection,
  GPTransfer transfer,
  int index,
  gsi_char ** name
)
{
	GPIFile * file;
	GPITransfer * pTransfer;

	// Get the transfer.
	////////////////////
	pTransfer = gpiFindTransferByLocalID(connection, transfer);
	if(!pTransfer)
		Error(connection, GP_PARAMETER_ERROR, "Invalid transfer.");

	// Get the file.
	////////////////
	if((index < 0) || (index >= ArrayLength(pTransfer->files)))
		Error(connection, GP_PARAMETER_ERROR, "Invalid index.");
	file = (GPIFile *)ArrayNth(pTransfer->files, index);

	// Get the name.
	////////////////
#ifndef GSI_UNICODE
	*name = file->name;
#else
	*name = file->name_W;
#endif

	return GP_NO_ERROR;
}


GPResult gpGetFilePath(
  GPConnection * connection,
  GPTransfer transfer,
  int index,
  gsi_char ** path
)
{
	GPIFile * file;
	GPITransfer * pTransfer;

	// Get the transfer.
	////////////////////
	pTransfer = gpiFindTransferByLocalID(connection, transfer);
	if(!pTransfer)
		Error(connection, GP_PARAMETER_ERROR, "Invalid transfer.");

	// Get the file.
	////////////////
	if((index < 0) || (index >= ArrayLength(pTransfer->files)))
		Error(connection, GP_PARAMETER_ERROR, "Invalid index.");
	file = (GPIFile *)ArrayNth(pTransfer->files, index);

	// Get the path.
	////////////////
#ifndef GSI_UNICODE
	*path = file->path;
#else
	*path = file->path_W;
#endif

	return GP_NO_ERROR;
}

GPResult gpGetFileSize(
  GPConnection * connection,
  GPTransfer transfer,
  int index,
  int * size
)
{
	GPIFile * file;
	GPITransfer * pTransfer;

	// Get the transfer.
	////////////////////
	pTransfer = gpiFindTransferByLocalID(connection, transfer);
	if(!pTransfer)
		Error(connection, GP_PARAMETER_ERROR, "Invalid transfer.");

	// Get the file.
	////////////////
	if((index < 0) || (index >= ArrayLength(pTransfer->files)))
		Error(connection, GP_PARAMETER_ERROR, "Invalid index.");
	file = (GPIFile *)ArrayNth(pTransfer->files, index);

	// Get the size.
	////////////////
	*size = file->size;

	return GP_NO_ERROR;
}

GPResult gpGetFileProgress(
  GPConnection * connection,
  GPTransfer transfer,
  int index,
  int * progress
)
{
	GPIFile * file;
	GPITransfer * pTransfer;

	// Get the transfer.
	////////////////////
	pTransfer = gpiFindTransferByLocalID(connection, transfer);
	if(!pTransfer)
		Error(connection, GP_PARAMETER_ERROR, "Invalid transfer.");

	// Get the file.
	////////////////
	if((index < 0) || (index >= ArrayLength(pTransfer->files)))
		Error(connection, GP_PARAMETER_ERROR, "Invalid index.");
	file = (GPIFile *)ArrayNth(pTransfer->files, index);

	// Get the progress.
	////////////////////
	*progress = file->progress;

	return GP_NO_ERROR;
}

GPResult gpGetFileModificationTime(
  GPConnection * connection,
  GPTransfer transfer,
  int index,
  gsi_time * modTime
)
{
	GPIFile * file;
	GPITransfer * pTransfer;

	// Get the transfer.
	////////////////////
	pTransfer = gpiFindTransferByLocalID(connection, transfer);
	if(!pTransfer)
		Error(connection, GP_PARAMETER_ERROR, "Invalid transfer.");

	// Get the file.
	////////////////
	if((index < 0) || (index >= ArrayLength(pTransfer->files)))
		Error(connection, GP_PARAMETER_ERROR, "Invalid index.");
	file = (GPIFile *)ArrayNth(pTransfer->files, index);

	// Get the modTime.
	///////////////////
	*modTime = file->modTime;

	return GP_NO_ERROR;
}

GPResult gpGetNumTransfers(
  GPConnection * connection,
  int * num
)
{
	GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Check for NULL.
	//////////////////
	if(num == NULL)
		Error(connection, GP_PARAMETER_ERROR, "NULL pointer.");

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Set num.
	///////////
	*num = ArrayLength(iconnection->transfers);

	return GP_NO_ERROR;
}

GPResult gpGetTransfer(
  GPConnection * connection,
  int index,
  GPTransfer * transfer
)
{
	GPIConnection * iconnection;
	int localID;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return GP_PARAMETER_ERROR;

	// Check for NULL.
	//////////////////
	if(transfer == NULL)
		Error(connection, GP_PARAMETER_ERROR, "NULL pointer.");

	// Get the connection object.
	/////////////////////////////
	iconnection = (GPIConnection *)*connection;

	// Get the local ID.
	////////////////////
	localID = gpiGetTransferLocalIDByIndex(connection, index);

	// Check if it was a bad index.
	///////////////////////////////
	if(localID == -1)
		Error(connection, GP_PARAMETER_ERROR, "Index out of range.");

	// Set the transfer they want.
	//////////////////////////////
	*transfer = localID;

	return GP_NO_ERROR;
}
#endif

#ifdef _DEBUG
void gpProfilesReport(
  GPConnection * connection,
  void (* report)(const char * output)
)
{
	//GPIConnection * iconnection;

	// Error check.
	///////////////
	if((connection == NULL) || (*connection == NULL))
		return;

	// Get the connection object.
	/////////////////////////////
	//iconnection = (GPIConnection *)*connection;

	gpiReport(connection, report);
}
#endif
