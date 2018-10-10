// battleye_system.h
// BattlEyeSystem class

#ifndef XR_BATTLEYE_SYSTEM_H_INCLUDED
#define XR_BATTLEYE_SYSTEM_H_INCLUDED

#include "battleye.h"

#ifdef BATTLEYE

#include "xr_Server_BattlEye.h"
#include "xr_Client_BattlEye.h"

#define BATTLEYE_SERVER_DLL "BattlEye" DELIMITER "BEServer.dll"
#define BATTLEYE_CLIENT_DLL "BattlEye" DELIMITER "BEClient.dll"

class xrServer;
class BattlEyeServer;
class BattlEyeClient;

class BattlEyeSystem
{
private:
    shared_str m_server_path;
    shared_str m_client_path;

    bool m_test_load_client;
    bool InitDLL(LPCSTR dll_name, string_path& out_file);

public:
    BattlEyeSystem();
    ~BattlEyeSystem();

    void SetServerPath(LPCSTR path);
    void SetClientPath(LPCSTR path);
    LPCSTR GetServerPath();
    LPCSTR GetClientPath();

    bool ReloadServerDLL(xrServer* xr_server);
    bool ReloadClientDLL();

    bool LoadClient();
    bool LoadServer(xrServer* xr_server);
    void UpdateClient();
    void UpdateServer(xrServer* xr_server);

    void ReadPacketClient(NET_Packet* pack);
    void ReadPacketServer(u32 sender, NET_Packet* pack);

    bool TestLoadClient();
    IC bool GetTestClient() { return m_test_load_client; }
    bool InitDir();

public:
    BattlEyeClient* client;
    BattlEyeServer* server;

    //-	int					auto_update;

}; // class BattlEyeSystem

#endif // BATTLEYE

#endif // XR_BATTLEYE_H_INCLUDED
