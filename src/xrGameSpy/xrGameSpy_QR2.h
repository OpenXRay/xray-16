#pragma once
#include "xrGameSpy_MainDefs.h"

#include "GameSpy\QR2\qr2regkeys.h"
#include "GameSpy\QR2\qr2.h"


extern "C"
{
	EXPORT_FN_DECL(const char*, RegisteredKey, (DWORD KeyID));
	EXPORT_FN_DECL(void, qr2_register_keyA, (int keyid, const gsi_char *key));
	EXPORT_FN_DECL(void, qr2_think, (qr2_t qrec));
	EXPORT_FN_DECL(void, qr2_shutdown, (qr2_t qrec));
	EXPORT_FN_DECL(void, qr2_buffer_addA, (qr2_buffer_t outbuf, const char *value));
	EXPORT_FN_DECL(void, qr2_buffer_add_int, (qr2_buffer_t outbuf, int value));
	EXPORT_FN_DECL(void, qr2_keybuffer_add, (qr2_keybuffer_t keybuffer, int keyid));

	EXPORT_FN_DECL(void, qr2_register_natneg_callback, (qr2_t qrec, qr2_natnegcallback_t nncallback));
	EXPORT_FN_DECL(void, qr2_register_clientmessage_callback, (qr2_t qrec, qr2_clientmessagecallback_t cmcallback));
	EXPORT_FN_DECL(void, qr2_register_publicaddress_callback, (qr2_t qrec, qr2_publicaddresscallback_t pacallback));
	EXPORT_FN_DECL(void, qr2_register_denyresponsetoip_callback, (qr2_t qrec, qr2_denyqr2responsetoipcallback_t dertoipcallback));

//	EXPORT_FN_DECL(qr2_error_t, qr2_init, (/*[out]*/qr2_t *qrec, const gsi_char *ip, int baseport, const gsi_char *gamename, const gsi_char *secret_key,
	EXPORT_FN_DECL(qr2_error_t, qr2_initA, (/*[out]*/qr2_t *qrec, const gsi_char *ip, int baseport, 
			int ispublic, int natnegotiate,
			qr2_serverkeycallback_t server_key_callback,
			qr2_playerteamkeycallback_t player_key_callback,
			qr2_playerteamkeycallback_t team_key_callback,
			qr2_keylistcallback_t key_list_callback,
			qr2_countcallback_t playerteam_count_callback,
			qr2_adderrorcallback_t adderror_callback,
			void *userdata));


}