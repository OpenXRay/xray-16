#pragma once

#include "GameSpy_FuncDefs.h"

class CGameSpy_QR2
{
private:
//	string16	m_SecretKey;

	HMODULE	m_hGameSpyDLL;

	void	LoadGameSpy(HMODULE hGameSpyDLL);
public:
	CGameSpy_QR2();
	CGameSpy_QR2(HMODULE hGameSpyDLL);
	~CGameSpy_QR2();

//	bool	Init		(u32 PortID, int Public, void* instance);
	bool	Init		(int PortID, int Public, void* instance);
	void	Think		(void* qrec);
	void	ShutDown	(void* qrec);
	void	RegisterAdditionalKeys	();
	void	BufferAdd		(void* outbuf, const char* value);
	void	BufferAdd_Int	(void* outbuf, int value);
	void	KeyBufferAdd	(void* keybuffer, int keyid);

	const char*	GetGameVersion		(const	char*result);
private:
	//--------------------- QR2 --------------------------------------------------
public:
	GAMESPY_FN_VAR_DECL(const char*, RegisteredKey, (DWORD KeyID));	
private:
	GAMESPY_FN_VAR_DECL(void, qr2_register_keyA, (int keyid, const char *key));
	GAMESPY_FN_VAR_DECL(void, qr2_think, (void* qrec));
	GAMESPY_FN_VAR_DECL(void, qr2_shutdown, (void* qrec));
	GAMESPY_FN_VAR_DECL(void, qr2_buffer_addA, (void* outbuf, const char *value));
	GAMESPY_FN_VAR_DECL(void, qr2_buffer_add_int, (void* outbuf, int value));
	GAMESPY_FN_VAR_DECL(void, qr2_keybuffer_add, (void* keybuffer, int keyid));

	GAMESPY_FN_VAR_DECL(void, qr2_register_natneg_callback, (void* qrec, qr2_natnegcallback_t nncallback));
	GAMESPY_FN_VAR_DECL(void, qr2_register_clientmessage_callback, (void* qrec, qr2_clientmessagecallback_t cmcallback));
	GAMESPY_FN_VAR_DECL(void, qr2_register_publicaddress_callback, (void* qrec, qr2_publicaddresscallback_t pacallback));
	GAMESPY_FN_VAR_DECL(void, qr2_register_denyresponsetoip_callback, (void* qrec, qr2_denyqr2responsetoipcallback_t dertoipcallback));

	GAMESPY_FN_VAR_DECL(const char*, GetGameVersion, (const	char*));

	GAMESPY_FN_VAR_DECL(qr2_error_t, qr2_initA, (
			qr2_t *qrec,
			const gsi_char *ip,
			int baseport, 
			int ispublic,
			int natnegotiate,
			qr2_serverkeycallback_t server_key_callback,
			qr2_playerteamkeycallback_t player_key_callback,
			qr2_playerteamkeycallback_t team_key_callback,
			qr2_keylistcallback_t key_list_callback,
			qr2_countcallback_t playerteam_count_callback,
			qr2_adderrorcallback_t adderror_callback,
			void *userdata));

};

