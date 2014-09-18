//--------------------------- QR2 callbacks ---------------------------------------
// Called when a server key needs to be reported
void __cdecl callback_serverkey(int keyid, qr2_buffer_t outbuf, void *userdata);
// Called when a player key needs to be reported
void __cdecl callback_playerkey(int keyid, int index, qr2_buffer_t outbuf, void *userdata);
// Called when a team key needs to be reported
void __cdecl callback_teamkey(int keyid, int index, qr2_buffer_t outbuf, void *userdata);
// Called when we need to report the list of keys we report values for
void __cdecl callback_keylist(qr2_key_type keytype, qr2_keybuffer_t keybuffer, void *userdata);
// Called when we need to report the number of players and teams
int  __cdecl callback_count(qr2_key_type keytype, void *userdata);
// Called if our registration with the GameSpy master server failed
void __cdecl callback_adderror(qr2_error_t error, gsi_char *errmsg, void *userdata);
// Called when a client wants to connect using nat negotiation
//   (Nat Negotiation must be enabled in qr2_init)
void __cdecl callback_nn(int cookie, void *userdata);
// Called when a client sends a message to the server through qr2
//    (Not commonly used)
void __cdecl callback_cm(char *data, int len, void *userdata);
// Called when gamespy responds my public ip and port
//void __cdecl callback_public(unsigned int ip, unsigned short port, void* userdata);
void __cdecl callback_deny_ip(void *userdata, unsigned int sender_ip, int * result);
