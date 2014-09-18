#include "stdafx.h"
#include "windows.h"
#include "xrGameSpy_MainDefs.h"

#include "xrGameSpy_QR2.h"

XRGAMESPY_API const char* xrGS_RegisteredKey(DWORD KeyID)
{
	return qr2_registered_key_list[KeyID];
};

XRGAMESPY_API void xrGS_qr2_register_keyA(int keyid, const gsi_char *key)
{
	qr2_register_keyA(keyid, key);
}

XRGAMESPY_API void xrGS_qr2_think(qr2_t qrec)
{
	qr2_think(qrec);
};

XRGAMESPY_API void xrGS_qr2_shutdown (qr2_t qrec)
{
	qr2_shutdown(qrec);
};

XRGAMESPY_API void xrGS_qr2_buffer_addA(qr2_buffer_t outbuf, const char *value)
{
	qr2_buffer_addA(outbuf, value);
};

XRGAMESPY_API void xrGS_qr2_buffer_add_int(qr2_buffer_t outbuf, int value)
{
	qr2_buffer_add_int(outbuf, value);
}

XRGAMESPY_API void xrGS_qr2_keybuffer_add(qr2_keybuffer_t keybuffer, int keyid)
{
	qr2_keybuffer_add(keybuffer, keyid);
}

XRGAMESPY_API void xrGS_qr2_register_natneg_callback (qr2_t qrec, qr2_natnegcallback_t nncallback)
{
	qr2_register_natneg_callback (qrec, nncallback);
};
XRGAMESPY_API void xrGS_qr2_register_clientmessage_callback (qr2_t qrec, qr2_clientmessagecallback_t cmcallback)
{
	qr2_register_clientmessage_callback (qrec, cmcallback);
};
XRGAMESPY_API void xrGS_qr2_register_publicaddress_callback (qr2_t qrec, qr2_publicaddresscallback_t pacallback)
{
	qr2_register_publicaddress_callback (qrec, pacallback);
};
XRGAMESPY_API void xrGS_qr2_register_denyresponsetoip_callback(qr2_t qrec, qr2_denyqr2responsetoipcallback_t dertoipcallback)
{
	qr2_register_denyresponsetoip_callback (qrec, dertoipcallback);
};


//XRGAMESPY_API qr2_error_t xrGS_qr2_init(/*[out]*/qr2_t *qrec, const gsi_char *ip, int baseport, const gsi_char *gamename, const gsi_char *secret_key,
XRGAMESPY_API qr2_error_t xrGS_qr2_initA(/*[out]*/qr2_t *qrec, const gsi_char *ip, int baseport, 
			   int ispublic, int natnegotiate,
			   qr2_serverkeycallback_t server_key_callback,
			   qr2_playerteamkeycallback_t player_key_callback,
			   qr2_playerteamkeycallback_t team_key_callback,
			   qr2_keylistcallback_t key_list_callback,
			   qr2_countcallback_t playerteam_count_callback,
			   qr2_adderrorcallback_t adderror_callback,
			   void *userdata)
{
	int BasePort = baseport;
	if (BasePort == -1) BasePort = GAMESPY_QR2_BASEPORT;
	else
	{
		if (BasePort < START_PORT) BasePort = START_PORT;
		if (BasePort > END_PORT) BasePort = END_PORT;
	}

	char SecretKey[16];
	FillSecretKey(SecretKey);

	qr2_error_t res = 
		qr2_initA(qrec, ip, BasePort, GAMESPY_GAMENAME, SecretKey,
		ispublic, 
		natnegotiate,
		server_key_callback,
		player_key_callback,
		team_key_callback,
		key_list_callback,
		playerteam_count_callback,
		adderror_callback,
		userdata);

	return res;
}