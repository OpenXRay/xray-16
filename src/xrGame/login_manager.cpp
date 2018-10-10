#include "StdAfx.h"
#include "login_manager.h"
#include "account_manager.h"
#include "MainMenu.h"
#include "xrGameSpy/GameSpy_Full.h"
#include "xrGameSpy/GameSpy_GP.h"
#include "xrGameSpy/GameSpy_ATLAS.h"
#include "xrGameSpy/GameSpy_Patching.h"
#include "RegistryFuncs.h"
#include "xrGameSpy/xrGameSpy_MainDefs.h"
#include "player_name_modifyer.h"
#include "ui/UICDkey.h"
#include "secure_messaging.h"

#if defined(WINDOWS)
#include <shellapi.h>
#endif
//#pragma comment(lib, "shell32.lib")

namespace gamespy_gp
{
login_manager::login_manager(CGameSpy_Full* fullgs_obj)
{
    VERIFY(fullgs_obj);
    m_gamespy_gp = fullgs_obj->GetGameSpyGP();
    VERIFY(m_gamespy_gp);
    m_gamespy_atlas = fullgs_obj->GetGameSpyATLAS();
    VERIFY(m_gamespy_atlas);
    m_gamespy_patching = fullgs_obj->GetGameSpyPatching();
    VERIFY(m_gamespy_patching);
    m_current_profile = NULL;
}

login_manager::~login_manager() { xr_delete(m_current_profile); }
void login_manager::login(char const* email, char const* nick, char const* password, login_operation_cb logincb)
{
    if (!logincb)
    {
        logincb.bind(this, &login_manager::only_log_login);
    }
    login_params_t tmp_args(email, nick, password);
    m_login_qam.execute(this, tmp_args, logincb);
}

void login_manager::stop_login() { m_login_qam.stop(); }
void login_manager::login_raw(login_params_t const& login_args, login_operation_cb logincb)
{
    VERIFY(!m_login_operation_cb);
    VERIFY2(!m_current_profile, "please, logout first (gs_logout)");

    if (m_current_profile)
    {
        Msg("! WARNING: first you need to log out...");
        m_login_operation_cb(NULL, "mp_first_need_to_logout");
        m_login_operation_cb.clear();
        return;
    }

    m_last_email = login_args.m_t1; // email;
    m_last_nick = login_args.m_t2; // nick;
    m_last_password = login_args.m_t3; // password;
    m_login_operation_cb = logincb;

    GPResult tmp_res =
        m_gamespy_gp->Connect(m_last_email, m_last_nick, m_last_password, &login_manager::login_cb, this);

    if (tmp_res != GP_NO_ERROR)
    {
        m_login_operation_cb.clear();
        logincb(NULL, CGameSpy_GP::TryToTranslate(tmp_res).c_str());
        return;
    }
}

void login_manager::release_login(profile const* prof_arg, char const*)
{
    VERIFY(m_current_profile == prof_arg);
    if (m_current_profile == prof_arg)
    {
        logout();
        // disconnect will reset connection state
        // so we must to reinit all asyncronous tasks
        reinit_connection_tasks();
    }
}

void login_manager::login_offline(char const* nick, login_operation_cb logincb)
{
    if (m_login_operation_cb)
    {
        Msg("! WARNING: login in process...");
        return;
    }
    if (!logincb)
    {
        m_login_operation_cb.bind(this, &login_manager::only_log_login);
    }
    else
    {
        m_login_operation_cb = logincb;
    }

    VERIFY2(!m_current_profile, "please, logout first (gs_logout)");
    if (m_current_profile)
    {
        Msg("! WARNING: first you need to log out...");
        m_login_operation_cb(NULL, "mp_first_need_to_logout");
        m_login_operation_cb.clear();
        return;
    }

    pcstr name_iterator = nick;
    bool has_non_white_chars = false;
    if (nick)
    {
        while (*name_iterator)
        {
            if (*name_iterator != ' ' && *name_iterator != '\t')
            {
                has_non_white_chars = true;
                break;
            }

            ++name_iterator;
        }
    }

    if (!has_non_white_chars)
    {
        Msg("! ERROR: nick name is empty");
        m_login_operation_cb(NULL, "mp_nick_name_not_valid");
        m_login_operation_cb.clear();
        return;
    }

    // verify symbols in new unique nick
    string256 new_nick;
    modify_player_name(nick, new_nick);
    m_current_profile = new profile(0, new_nick, "", false);
    m_login_operation_cb(m_current_profile, "mp_login_success");
    m_login_operation_cb.clear();
}

void login_manager::set_unique_nick(char const* new_unick, login_operation_cb logincb)
{
    if (!logincb)
    {
        logincb.bind(this, &login_manager::only_log_login);
    }
    set_unick_params_t tmp_arg(new_unick);
    m_unique_nick_qam.execute(this, tmp_arg, logincb);
}

void login_manager::stop_setting_unique_nick() { m_unique_nick_qam.stop(); }
void login_manager::set_unique_nick_raw(set_unick_params_t const& new_unick, login_operation_cb logincb)
{
    VERIFY(!m_login_operation_cb);

    if (!m_current_profile)
    {
        Msg("! WARNING: first you need to log in...");
        logincb(NULL, "mp_first_need_to_login");
        return;
    }

    if (!new_unick.m_t1.size())
    {
        Msg("! ERROR: nick name is empty");
        logincb(NULL, "mp_unique_nick_not_valid");
        return;
    }

    if (!m_current_profile->online())
    {
        // verify symbols in new unique nick
        string256 updated_unick;
        modify_player_name(new_unick.m_t1.c_str(), updated_unick);
        m_current_profile->m_unique_nick = updated_unick;
        logincb(m_current_profile, "mp_change_unick_success");
        return;
    }

    m_login_operation_cb = logincb;
    m_last_unick = new_unick.m_t1;

    GPResult tmp_res = m_gamespy_gp->SetUniqueNick(m_last_unick, &login_manager::setunick_cb, this);

    if (tmp_res != GP_NO_ERROR)
    {
        m_login_operation_cb.clear();
        logincb(NULL, CGameSpy_GP::TryToTranslate(tmp_res).c_str());
        return;
    }
}

void login_manager::logout()
{
    VERIFY2(m_current_profile, "not logged in");
    if (m_current_profile->online())
    {
        m_gamespy_gp->Disconnect();
    }
    delete_profile_obj();
    Msg("* GameSpy: Logged out.");
}

void login_manager::reinit_connection_tasks()
{
#ifdef WINDOWS
    account_manager* tmp_acc_mngr = MainMenu()->GetAccountMngr();
    if (tmp_acc_mngr->is_get_account_profiles_active())
    {
        Msg("! WARNING: reiniting get account profiles");
        tmp_acc_mngr->reinit_get_account_profiles();
    }
    if (tmp_acc_mngr->is_email_searching_active())
    {
        Msg("! WARNING: reiniting searching emails");
        tmp_acc_mngr->reinit_email_searching();
    }
    if (tmp_acc_mngr->is_suggest_unique_nicks_active())
    {
        Msg("! WARNING: reiniting suggesting unique nicks");
        tmp_acc_mngr->reinit_suggest_unique_nicks();
    }
#endif
}

void login_manager::delete_profile_obj() { xr_delete(m_current_profile); }
void __stdcall login_manager::only_log_login(profile const* res_profile, char const* description)
{
    if (!res_profile)
    {
        Msg("! GameSpy login ERROR: %s", description ? description : "unknown");
        return;
    }
    Msg("* GameSpy login operation success ! Hello %s !", res_profile->m_unique_nick.c_str());
}

void __cdecl login_manager::setunick_cb(GPConnection* connection, void* arg, void* param)
{
    login_manager* my_inst = static_cast<login_manager*>(param);

    login_operation_cb tmp_cb = my_inst->m_login_operation_cb;
    my_inst->m_login_operation_cb.clear();

    GPRegisterUniqueNickResponseArg* tmp_res = static_cast<GPRegisterUniqueNickResponseArg*>(arg);
    VERIFY(my_inst);
    VERIFY(tmp_res);
    VERIFY(my_inst->m_current_profile);

    if (tmp_res->result != GP_NO_ERROR)
    {
        tmp_cb(NULL, CGameSpy_GP::TryToTranslate(tmp_res->result).c_str());
        return;
    }

    my_inst->m_current_profile->m_unique_nick = my_inst->m_last_unick;
    tmp_cb(my_inst->m_current_profile, "mp_change_unick_success");
}

void __cdecl login_manager::login_cb(GPConnection* connection, void* arg, void* param)
{
    login_manager* my_inst = static_cast<login_manager*>(param);
    GPConnectResponseArg* tmp_res = static_cast<GPConnectResponseArg*>(arg);

    login_operation_cb tmp_cb = my_inst->m_login_operation_cb;

    VERIFY(my_inst);
    VERIFY(tmp_res);

    if (tmp_res->result != GP_NO_ERROR)
    {
        my_inst->m_login_operation_cb.clear();
        tmp_cb(NULL, CGameSpy_GP::TryToTranslate(tmp_res->result).c_str());
        return;
    }

    char tmp_ticket_dest[GP_LOGIN_TICKET_LEN];
    ZeroMemory(tmp_ticket_dest, sizeof(tmp_ticket_dest));

    VERIFY(my_inst->m_gamespy_gp);
    GPResult tmp_lticket_res = my_inst->m_gamespy_gp->GetLoginTicket(tmp_ticket_dest);
    VERIFY(tmp_lticket_res == GP_NO_ERROR);
    if (tmp_lticket_res != GP_NO_ERROR)
    {
        Msg("! ERROR: failed to get login ticket");
        tmp_ticket_dest[0] = 0;
    }

    my_inst->m_current_profile = new profile(tmp_res->profile, tmp_res->uniquenick, tmp_ticket_dest, true);
    my_inst->m_gamespy_patching->PtTrackUsage(tmp_res->profile);

    my_inst->m_gamespy_atlas->WSLoginProfile(
        my_inst->m_last_email, my_inst->m_last_nick, my_inst->m_last_password, &login_manager::wslogin_cb, my_inst);
}

void __cdecl login_manager::wslogin_cb(GHTTPResult httpResult, WSLoginResponse* response, void* userData)
{
    login_manager* my_inst = static_cast<login_manager*>(userData);
    login_operation_cb tmp_cb = my_inst->m_login_operation_cb;
    my_inst->m_login_operation_cb.clear();

    if (httpResult != GHTTPSuccess)
    {
        tmp_cb(NULL, CGameSpy_ATLAS::TryToTranslate(httpResult).c_str());
        my_inst->delete_profile_obj();
        return;
    }
    VERIFY(response);

    if (response->mLoginResult != WSLogin_Success)
    {
        tmp_cb(NULL, CGameSpy_ATLAS::TryToTranslate(response->mLoginResult).c_str());
        my_inst->delete_profile_obj();
        return;
    }

    VERIFY(my_inst->m_current_profile);

    my_inst->m_current_profile->mCertificate = response->mCertificate;
    my_inst->m_current_profile->mPrivateData = response->mPrivateData;

    tmp_cb(my_inst->m_current_profile, "mp_login_success");
}

void login_manager::save_email_to_registry(char const* email)
{
    if (!email || (xr_strlen(email) == 0))
    {
        Msg("! ERROR: email is empty");
        return;
    }
    WriteRegistry_StrValue(REGISTRY_VALUE_USEREMAIL, email);
}

char const* login_manager::get_email_from_registry()
{
    m_reg_email[0] = 0;
    ReadRegistry_StrValue(REGISTRY_VALUE_USEREMAIL, m_reg_email);
    return m_reg_email;
}

static const u32 pass_key_seed = 0x07071984;

void login_manager::save_password_to_registry(char const* password)
{
    using namespace secure_messaging;
    if (!password || (xr_strlen(password) == 0))
    {
        Msg("! ERROR: password is empty");
        return;
    }

    secure_messaging::key_t pass_key;
    generate_key(pass_key_seed, pass_key);
    u32 buffer_size = xr_strlen(password) + 1;
    u8* buffer = static_cast<u8*>(_alloca(buffer_size));
    xr_strcpy((char*)buffer, buffer_size, password);
    buffer[buffer_size - 1] = 0;
    encrypt(buffer, buffer_size, pass_key);
    WriteRegistry_BinaryValue(REGISTRY_VALUE_USERPASSWORD, buffer, buffer_size);
}

char const* login_manager::get_password_from_registry()
{
    using namespace secure_messaging;
    xr_strcpy(m_reg_password, "");
    u8 tmp_password_dest[128]; // max password length is 30 symbols...

    u32 pass_size = ReadRegistry_BinaryValue(REGISTRY_VALUE_USERPASSWORD, tmp_password_dest, sizeof(tmp_password_dest));

    if (pass_size)
    {
        secure_messaging::key_t pass_key;
        generate_key(pass_key_seed, pass_key);
        decrypt(tmp_password_dest, pass_size, pass_key);
        xr_strcpy(m_reg_password, (char*)tmp_password_dest);
    }
    return m_reg_password;
}

void login_manager::save_remember_me_to_registry(bool remember)
{
    DWORD tmp_value = remember ? 1 : 0;
    WriteRegistry_DWValue(REGISTRY_VALUE_REMEMBER_PROFILE, tmp_value);
}

void login_manager::save_nick_to_registry(char const* nickname)
{
    string256 tmp_str;
    xr_strcpy(tmp_str, nickname);
    WritePlayerName_ToRegistry(tmp_str);
}

char const* login_manager::get_nick_from_registry()
{
    m_reg_nick[0] = 0;
    GetPlayerName_FromRegistry(m_reg_nick, sizeof(m_reg_nick));
    return m_reg_nick;
}

bool login_manager::get_remember_me_from_registry()
{
    DWORD tmp_value = 0;
    ReadRegistry_DWValue(REGISTRY_VALUE_REMEMBER_PROFILE, tmp_value);
    return tmp_value != 0;
}

void login_manager::forgot_password(char const* url)
{
#ifdef WINDOWS
    LPCSTR params = NULL;
    STRCONCAT(params, "/C start ", url);
    ShellExecute(0, "open", "cmd.exe", params, NULL, SW_SHOW);
#else
    std::string command = "xdg-open " + std::string{url};
    system(command.c_str());
#endif
}

} // namespace gamespy_gp
