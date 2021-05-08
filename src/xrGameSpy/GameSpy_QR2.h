#pragma once
#include "xrGameSpy/xrGameSpy.h"

class xrGameSpyServer;

class XRGAMESPY_API CGameSpy_QR2
{
public:
    struct SInitConfig
    {
    public:
        void(__cdecl* OnServerKey)(int keyid, qr2_buffer_t outbuf, void* userdata);
        void(__cdecl* OnPlayerKey)(int keyid, int index, qr2_buffer_t outbuf, void* userdata);
        void(__cdecl* OnTeamKey)(int keyid, int index, qr2_buffer_t outbuf, void* userdata);
        void(__cdecl* OnKeyList)(qr2_key_type keytype, qr2_keybuffer_t keybuffer, void* userdata);
        int(__cdecl* OnCount)(qr2_key_type keytype, void* userdata);
        void(__cdecl* OnError)(qr2_error_t error, gsi_char* errmsg, void* userdata);
        void(__cdecl* OnNatNeg)(int cookie, void* userdata);
        void(__cdecl* OnClientMessage)(char* data, int len, void* userdata);
        void(__cdecl* OnDenyIP)(void* userdata, u32 senderIP, int* result);
        xrGameSpyServer* GSServer;
    };

private:
    //	string16	m_SecretKey;

public:
    bool Init(int PortID, int Public, SInitConfig& ctx);
    void Think(void* qrec);
    void ShutDown(void* qrec);
    void RegisterAdditionalKeys();
    void BufferAdd(void* outbuf, const char* value);
    void BufferAdd_Int(void* outbuf, int value);
    void KeyBufferAdd(void* keybuffer, int keyid);

    const char* GetGameVersion();
    const char* RegisteredKey(u32 KeyID);
};
