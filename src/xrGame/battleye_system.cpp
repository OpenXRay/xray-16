// battleye_system.cpp
// BattlEyeSystem class implementation

#include "StdAfx.h"
#include "battleye_system.h"
#include "object_broker.h"
#include "MainMenu.h"

#ifdef BATTLEYE

#define BATTLEYE_DIR "BattlEye"

int g_be_message_out = 1;

BattlEyeSystem::BattlEyeSystem()
{
    client = NULL;
    server = NULL;

    m_test_load_client = false;
    //-	auto_update = 1;
}

BattlEyeSystem::~BattlEyeSystem()
{
    xr_delete(client);
    xr_delete(server);
}

void BattlEyeSystem::SetServerPath(LPCSTR path)
{
    VERIFY(path);
    m_server_path._set(path);
}

void BattlEyeSystem::SetClientPath(LPCSTR path)
{
    VERIFY(path);
    m_client_path._set(path);
}

LPCSTR BattlEyeSystem::GetServerPath() { return m_server_path.c_str(); }
LPCSTR BattlEyeSystem::GetClientPath() { return m_client_path.c_str(); }
bool BattlEyeSystem::ReloadServerDLL(xrServer* xr_server)
{
    xr_delete(server);
    server = NULL;

    LPCSTR dll_file_name = m_server_path.c_str();
    string_path old_file_name;
    string_path new_file_name;
    string_path err_file_name;

    strconcat(sizeof(old_file_name), old_file_name, dll_file_name, ".old");
    strconcat(sizeof(new_file_name), new_file_name, dll_file_name, ".new");
    strconcat(sizeof(err_file_name), err_file_name, dll_file_name, ".error");

    if (rename(dll_file_name, old_file_name))
    {
        Msg("! Could not rename %s to %s", dll_file_name, old_file_name);
        return false;
    }
    if (rename(new_file_name, dll_file_name))
    {
        Msg("! Could not rename %s to %s", new_file_name, dll_file_name);
        return false;
    }

    server = xr_new<BattlEyeServer>(xr_server);
    if (server->IsLoaded()) // (1) if loaded from BEServer.dll (new version)
    {
        server->AddConnectedPlayers();
        if (!DeleteFile(old_file_name))
        {
            Msg("! Could not delete old %s", old_file_name);
            // return _false;
        }
        return true;
    }

    xr_delete(server);
    server = NULL;

    if (rename(dll_file_name, err_file_name))
    {
        Msg("! Could not rename %s to %s", dll_file_name, err_file_name);
        return false;
    }
    if (rename(old_file_name, dll_file_name))
    {
        Msg("! Could not rename %s to %s", old_file_name, dll_file_name);
        return false;
    }

    server = xr_new<BattlEyeServer>(xr_server);
    if (server->IsLoaded()) // (2) if loaded from BEServer.dll (prev version)
    {
        server->AddConnectedPlayers();
        return true;
    }

    xr_delete(server);
    server = NULL;
    return false;
}

bool BattlEyeSystem::ReloadClientDLL()
{
    xr_delete(client);
    client = NULL;

    LPCSTR dll_file_name = m_client_path.c_str();
    string_path old_file_name;
    string_path new_file_name;
    string_path err_file_name;
    strconcat(sizeof(old_file_name), old_file_name, dll_file_name, ".old");
    strconcat(sizeof(new_file_name), new_file_name, dll_file_name, ".new");
    strconcat(sizeof(err_file_name), err_file_name, dll_file_name, ".error");

    if (rename(dll_file_name, old_file_name))
    {
        Msg("! Could not rename %s to %s", dll_file_name, old_file_name);
        return false;
    }
    if (rename(new_file_name, dll_file_name))
    {
        Msg("! Could not rename %s to %s", new_file_name, dll_file_name);
        return false;
    }

    client = xr_new<BattlEyeClient>();
    if (client->IsLoaded()) // (1) if loaded from BEClient.dll (new version)
    {
        if (!DeleteFile(old_file_name))
        {
            Msg("! Could not delete old %s", old_file_name);
            // return _false;
        }
        return true;
    }

    xr_delete(client);
    client = NULL;

    if (rename(dll_file_name, err_file_name))
    {
        Msg("! Could not rename %s to %s", dll_file_name, err_file_name);
        return false;
    }
    if (rename(old_file_name, dll_file_name))
    {
        Msg("! Could not rename %s to %s", old_file_name, dll_file_name);
        return false;
    }

    client = xr_new<BattlEyeClient>();
    if (client->IsLoaded()) // (2) if loaded from BEClient.dll (prev version)
    {
        return true;
    }

    xr_delete(client);
    client = NULL;
    return false;
}

bool BattlEyeSystem::LoadClient()
{
    // if ( Level().IsClient() )
    {
        if (client)
        {
            return true;
        }
        client = xr_new<BattlEyeClient>();
        if (client->IsLoaded())
        {
            return true;
        }
        xr_delete(client);
        client = NULL;
        Msg("! Error loading BattlEye Client!");
        MainMenu()->OnLoadError(BATTLEYE_CLIENT_DLL);
    }
    return false;
}

bool BattlEyeSystem::LoadServer(xrServer* xr_server)
{
    if (server)
    {
        return true;
    }
    server = xr_new<BattlEyeServer>(xr_server);
    if (server->IsLoaded())
    {
        return true;
    }
    xr_delete(server);
    server = NULL;

    Msg("! Error loading BattlEye Server!");
    MainMenu()->OnLoadError(BATTLEYE_SERVER_DLL);
    return false;
}

void BattlEyeSystem::UpdateClient()
{
    if (!client)
    {
        return;
    }
    if (!client->Run())
    {
        ReloadClientDLL();
    }
}

void BattlEyeSystem::UpdateServer(xrServer* xr_server)
{
    if (!server)
    {
        return;
    }
    if (!server->Run())
    {
        ReloadServerDLL(xr_server);
    }
}

void BattlEyeSystem::ReadPacketClient(NET_Packet* pack)
{
    if (!client)
    {
        return;
    }
    u32 len;
    pack->r_u32(len);

    void* data = xr_malloc(len);
    pack->r(data, len);

    client->NewPacket(data, len);
    xr_delete(data);
}

void BattlEyeSystem::ReadPacketServer(u32 sender, NET_Packet* pack)
{
    if (!server)
    {
        return;
    }
    u32 len;
    pack->r_u32(len);

    void* data = xr_malloc(len);
    pack->r(data, len);

    server->NewPacket(sender, data, len);
    xr_delete(data);
}

bool BattlEyeSystem::TestLoadClient()
{
    m_test_load_client = false;
    if (g_dedicated_server)
    {
        return true; // false = Error
    }

    if (LoadClient()) // test load
    {
        xr_delete(client);
        client = NULL;

        m_test_load_client = true;
    }
    return m_test_load_client;
}

bool BattlEyeSystem::InitDir()
{
    string_path dir_be;
    FS.update_path(dir_be, "$app_data_root$", "");
    strcat_s(dir_be, sizeof(dir_be), BATTLEYE_DIR);

    if (FS.can_write_to_folder(dir_be) == NULL)
    {
        if (!CreateDirectory(dir_be, NULL))
        {
            Msg("! Cannot make folder >>> %s", dir_be);
            return false;
        }
    }

    string_path fn_sv;
    if (!InitDLL(BATTLEYE_SERVER_DLL, fn_sv))
    {
        return false;
    }
    SetServerPath(fn_sv);

    string_path fn_cl;
    if (!InitDLL(BATTLEYE_CLIENT_DLL, fn_cl))
    {
        return false;
    }
    SetClientPath(fn_cl);

    return true;
}

bool BattlEyeSystem::InitDLL(LPCSTR dll_name, string_path& out_file)
{
    FILE* ft;
    FS.update_path(out_file, "$app_data_root$", "");
    strcat_s(out_file, sizeof(out_file), dll_name);

    ft = fopen(out_file, "r");
    if (ft == NULL)
    {
        HMODULE h_game = GetModuleHandle("xrCore");
        R_ASSERT(h_game);

        string_path cur_dir, full_dir;
        GetModuleFileName(h_game, full_dir, sizeof(full_dir));

        string_path fn_orig;
        string16 disk, ext;
        string64 fn_temp;
        _splitpath(full_dir, disk, cur_dir, fn_temp, ext);
        strconcat(sizeof(fn_orig), fn_orig, disk, cur_dir, dll_name);

        ft = fopen(fn_orig, "r");
        if (ft == NULL)
        {
            Msg("! File not found >>> %s", fn_orig);
            return false;
        }
        fclose(ft);

        CopyFile(fn_orig, out_file, FALSE);
        ft = fopen(out_file, "r");
        if (ft == NULL)
        {
            Msg("! File not found >>> %s", out_file);
            return false;
        }
        fclose(ft);
        return true;
    }
    fclose(ft);
    return true;
}

#endif // BATTLEYE
