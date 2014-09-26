#ifndef XRGAMESPY_GP
#define XRGAMESPY_GP

#include "xrGameSpy_MainDefs.h"
#include "GameSpy/GP/gp.h"

extern "C"
{

EXPORT_FN_DECL(GPResult,	gpInitialize,			(GPConnection * connection));
EXPORT_FN_DECL(void,		gpDestroy,				(GPConnection * connection));
EXPORT_FN_DECL(GPResult,	gpProcess,				(GPConnection * connection));
EXPORT_FN_DECL(GPResult,	gpSetCallback,			(GPConnection * connection,
													 GPEnum func,
													 GPCallback callback,
													 void * param));
EXPORT_FN_DECL(GPResult,	gpNewProfileA,			(GPConnection * connection,
													 const char nick[GP_NICK_LEN],
													 GPEnum replace,
													 GPEnum blocking,
													 GPCallback callback,
													 void * param));
EXPORT_FN_DECL(GPResult,	gpNewUserA,				(GPConnection * connection,
													 const gsi_char nick[GP_NICK_LEN],
													 const gsi_char uniquenick[GP_UNIQUENICK_LEN],
													 const gsi_char email[GP_EMAIL_LEN],
													 const gsi_char password[GP_PASSWORD_LEN],
													 const gsi_char cdkey[GP_CDKEY_LEN],
													 GPEnum blocking,
													 GPCallback callback,
													 void * param));
EXPORT_FN_DECL(GPResult,	gpProfileSearchA,		(GPConnection * connection,
													 const gsi_char nick[GP_NICK_LEN],
													 const gsi_char uniquenick[GP_UNIQUENICK_LEN],
													 const gsi_char email[GP_EMAIL_LEN],
													 const gsi_char firstname[GP_FIRSTNAME_LEN],
													 const gsi_char lastname[GP_LASTNAME_LEN],
													 int icquin,
													 GPEnum blocking,
													 GPCallback callback,
													 void * param));
EXPORT_FN_DECL(GPResult,	gpGetUserNicksA,		(GPConnection * connection,
													 const gsi_char email[GP_EMAIL_LEN],
													 const gsi_char password[GP_PASSWORD_LEN],
													 GPEnum blocking,
													 GPCallback callback,
													 void * param));
EXPORT_FN_DECL(GPResult,	gpConnectA,				(GPConnection * connection,
													 const gsi_char nick[GP_NICK_LEN],
													 const gsi_char email[GP_EMAIL_LEN],
													 const gsi_char password[GP_PASSWORD_LEN],
													 GPEnum firewall,
													 GPEnum blocking,
													 GPCallback callback,
													 void * param));
EXPORT_FN_DECL(void,		gpDisconnect,			(GPConnection * connection));

EXPORT_FN_DECL(GPResult,	gpSuggestUniqueNickA,	(GPConnection * connection,
													 const gsi_char desirednick[GP_UNIQUENICK_LEN],
													 GPEnum blocking,
													 GPCallback callback,
													 void * param));
EXPORT_FN_DECL(GPResult,	gpDeleteProfile,		(GPConnection * connection,
													 GPCallback callback,
													 void * arg));

EXPORT_FN_DECL(GPResult,	gpGetLoginTicket,		(GPConnection * connection,
													 char loginTicket[GP_LOGIN_TICKET_LEN]));

EXPORT_FN_DECL(GPResult,	gpRegisterUniqueNickA,	(GPConnection * connection,
													 const gsi_char uniquenick[GP_UNIQUENICK_LEN],
													 const gsi_char cdkey[GP_CDKEY_LEN],
													 GPEnum blocking,
													 GPCallback callback,
													 void * param));
}; //extern "C"

#endif //#ifndef XRGAMESPY_GP