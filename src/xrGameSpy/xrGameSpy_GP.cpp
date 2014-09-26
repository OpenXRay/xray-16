#include "stdafx.h"
#include "xrGameSpy_MainDefs.h"
#include "xrGameSpy_GP.h"

XRGAMESPY_API GPResult xrGS_gpInitialize(GPConnection * connection)
{
	return gpInitialize(
		connection,
		GAMESPY_PRODUCTID,
		GAMESPY_GP_NAMESPACE_ID,
		GP_PARTNERID_GAMESPY
	);
}

XRGAMESPY_API void xrGS_gpDestroy(GPConnection * connection)
{
	gpDestroy(connection);
}

XRGAMESPY_API GPResult xrGS_gpProcess(GPConnection * connection)
{
	return gpProcess(connection);
}

XRGAMESPY_API GPResult xrGS_gpSetCallback(GPConnection * connection,
										  GPEnum func,
										  GPCallback callback,
										  void * param)
{
	return gpSetCallback(connection, func, callback, param);
}

XRGAMESPY_API GPResult xrGS_gpNewProfileA(GPConnection * connection,
										 const char nick[GP_NICK_LEN],
										 GPEnum replace,
										 GPEnum blocking,
										 GPCallback callback,
										 void * param)
{
	return gpNewProfile(connection, nick, replace, blocking, callback, param);
}

XRGAMESPY_API GPResult xrGS_gpNewUserA	(GPConnection * connection,
										 const gsi_char nick[GP_NICK_LEN],
										 const gsi_char uniquenick[GP_UNIQUENICK_LEN],
										 const gsi_char email[GP_EMAIL_LEN],
										 const gsi_char password[GP_PASSWORD_LEN],
										 const gsi_char cdkey[GP_CDKEY_LEN],
										 GPEnum blocking,
										 GPCallback callback,
										 void * param)
{
	return gpNewUserA(
		connection,
		nick,
		uniquenick,
		email,
		password,
		cdkey,
		blocking,
		callback,
		param
	);
}

XRGAMESPY_API GPResult xrGS_gpProfileSearchA(GPConnection * connection,
											const gsi_char nick[GP_NICK_LEN],
											const gsi_char uniquenick[GP_UNIQUENICK_LEN],
											const gsi_char email[GP_EMAIL_LEN],
											const gsi_char firstname[GP_FIRSTNAME_LEN],
											const gsi_char lastname[GP_LASTNAME_LEN],
											int icquin,
											GPEnum blocking,
											GPCallback callback,
											void * param)
{
	return gpProfileSearchA(
		connection,
		nick,
		uniquenick,
		email,
		firstname,
		lastname,
		icquin,
		blocking,
		callback,
		param
	);
}

XRGAMESPY_API GPResult xrGS_gpGetUserNicksA(GPConnection * connection,
											const gsi_char email[GP_EMAIL_LEN],
											const gsi_char password[GP_PASSWORD_LEN],
											GPEnum blocking,
											GPCallback callback,
											void * param)
{
	return gpGetUserNicksA(
		connection,
		email,
		password,
		blocking,
		callback,
		param
	);
}


XRGAMESPY_API GPResult xrGS_gpConnectA(GPConnection * connection,
									   const gsi_char nick[GP_NICK_LEN],
									   const gsi_char email[GP_EMAIL_LEN],
									   const gsi_char password[GP_PASSWORD_LEN],
									   GPEnum firewall,
									   GPEnum blocking,
									   GPCallback callback,
									   void * param)
{
	return gpConnectA(
		connection,
		nick,
		email,
		password,
		firewall,
		blocking,
		callback,
		param
	);
}

XRGAMESPY_API void xrGS_gpDisconnect(GPConnection * connection)
{
	gpDisconnect(connection);
}

XRGAMESPY_API GPResult xrGS_gpSuggestUniqueNickA(GPConnection * connection,
												 const gsi_char desirednick[GP_UNIQUENICK_LEN],
												 GPEnum blocking,
												 GPCallback callback,
												 void * param)
{
	return gpSuggestUniqueNickA(
		connection,
		desirednick,
		blocking,
		callback,
		param
	);
}

XRGAMESPY_API GPResult xrGS_gpDeleteProfile(GPConnection * connection,
											GPCallback callback,
											void * arg)
{
	return gpDeleteProfile(connection, callback, arg);
}

XRGAMESPY_API GPResult xrGS_gpGetLoginTicket(GPConnection * connection,
											 char loginTicket[GP_LOGIN_TICKET_LEN])
{
	return gpGetLoginTicket(connection, loginTicket);
}

XRGAMESPY_API GPResult xrGS_gpRegisterUniqueNickA(GPConnection * connection,
												 const gsi_char uniquenick[GP_UNIQUENICK_LEN],
												 const gsi_char cdkey[GP_CDKEY_LEN],
												 GPEnum blocking,
												 GPCallback callback,
												 void * param)
{
	return gpRegisterUniqueNick(
		connection,
		uniquenick,
		cdkey,
		blocking,
		callback,
		param
	);
}
