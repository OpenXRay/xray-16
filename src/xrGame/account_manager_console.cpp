#include "StdAfx.h"
#include "account_manager_console.h"
#include "xrGameSpy/GameSpy_Full.h"
#include "xrGameSpy/GameSpy_GP.h"
#include "account_manager.h"
#include "login_manager.h"
#include "profile_store.h"
#include "MainMenu.h"
#include "profile_data_types_script.h"

void CCC_CreateGameSpyAccount::Execute(LPCSTR args)
{
#ifdef XR_PLATFORM_WINDOWS
    if (!args || (xr_strlen(args) == 0))
    {
        TInfo tmp_info;
        Info(tmp_info);
        Msg(tmp_info);
        return;
    }
    string256 tmp_nick;
    string256 tmp_unick;
    string256 tmp_email;
    string256 tmp_password;

    sscanf(args, "%s %s %s %s", tmp_nick, tmp_unick, tmp_email,tmp_password);

    VERIFY(MainMenu() && MainMenu()->GetGS());
    CGameSpy_GP* tmp_gp = MainMenu()->GetGS()->GetGameSpyGP();
    VERIFY(tmp_gp);
    gamespy_gp::account_manager* tmp_acc_mngr = MainMenu()->GetAccountMngr();

    gamespy_gp::new_profile_data prof_data;

    prof_data.nick = tmp_nick;
    prof_data.unique_nick = tmp_unick;
    prof_data.email = tmp_email;
    prof_data.password = tmp_password;

    tmp_acc_mngr->create_profile(tmp_nick, tmp_unick, tmp_email, tmp_password, gamespy_gp::account_operation_cb());
#endif
}

void CCC_GapySpyListProfiles::Execute(LPCSTR args)
{
#ifdef XR_PLATFORM_WINDOWS
    if (!args || (xr_strlen(args) == 0))
    {
        TInfo tmp_info;
        Info(tmp_info);
        Msg(tmp_info);
        return;
    }
    string256 tmp_email;
    string256 tmp_password;

    sscanf(args, "%s %s", tmp_email, tmp_password);

    VERIFY(MainMenu() && MainMenu()->GetGS());
    CGameSpy_GP* tmp_gp = MainMenu()->GetGS()->GetGameSpyGP();
    VERIFY(tmp_gp);
    gamespy_gp::account_manager* tmp_acc_mngr = MainMenu()->GetAccountMngr();
    tmp_acc_mngr->get_account_profiles(tmp_email, tmp_password, gamespy_gp::account_profiles_cb());
#endif
}

void CCC_GameSpyLogin::Execute(LPCSTR args)
{
#ifdef XR_PLATFORM_WINDOWS
    if (!args || (xr_strlen(args) == 0))
    {
        TInfo tmp_info;
        Info(tmp_info);
        Msg(tmp_info);
        return;
    }
    string256 tmp_email;
    string256 tmp_nick;
    string256 tmp_password;

    sscanf(args, "%s %s %s", tmp_email, tmp_nick, tmp_password);

    VERIFY(MainMenu() && MainMenu()->GetGS());
    CGameSpy_GP* tmp_gp = MainMenu()->GetGS()->GetGameSpyGP();
    VERIFY(tmp_gp);
    gamespy_gp::login_manager* tmp_lmngr = MainMenu()->GetLoginMngr();
    VERIFY(tmp_lmngr);
    tmp_lmngr->login(tmp_email, tmp_nick, tmp_password, gamespy_gp::login_operation_cb());
#endif
}

void CCC_GameSpyLogout::Execute(LPCSTR args)
{
#ifdef XR_PLATFORM_WINDOWS
    VERIFY(MainMenu() && MainMenu()->GetGS());
    gamespy_gp::login_manager* tmp_lmngr = MainMenu()->GetLoginMngr();
    VERIFY(tmp_lmngr);
    tmp_lmngr->logout();
#endif
}

pcstr print_time(time_t const& src_time, string64& dest_time);

void CCC_GameSpyPrintProfile::Execute(LPCSTR args)
{
#ifdef XR_PLATFORM_WINDOWS
    VERIFY(MainMenu() && MainMenu()->GetGS());
    gamespy_gp::login_manager* tmp_lmngr = MainMenu()->GetLoginMngr();
    gamespy_gp::profile const* tmp_profile = tmp_lmngr->get_current_profile();
    if (tmp_profile)
    {
        Msg("- Current profile:");
        Msg("- ProfileID  : %u", tmp_profile->m_profile_id);
        Msg("- UniqueNick : %s", tmp_profile->m_unique_nick.c_str());
    }
    else
    {
        Msg("- No profile. You are not loged in.");
    }
#endif
}

void CCC_GameSpySuggestUNicks::Execute(LPCSTR args)
{
#ifdef XR_PLATFORM_WINDOWS
    VERIFY(MainMenu() && MainMenu()->GetGS());
    string256 tmp_unick;
    sscanf(args, "%s", tmp_unick);
    gamespy_gp::account_manager* tmp_amngr = MainMenu()->GetAccountMngr();
    VERIFY(tmp_amngr);
    tmp_amngr->suggest_unique_nicks(tmp_unick, gamespy_gp::suggest_nicks_cb());
#endif
}

void CCC_GameSpyRegisterUniqueNick::Execute(LPCSTR args)
{
#ifdef XR_PLATFORM_WINDOWS
    VERIFY(MainMenu() && MainMenu()->GetGS());
    gamespy_gp::login_manager::unique_nick_t tmp_unick;
    sscanf(args, "%s", tmp_unick);
    gamespy_gp::login_manager* tmp_lmngr = MainMenu()->GetLoginMngr();
    VERIFY(tmp_lmngr);
    tmp_lmngr->set_unique_nick(tmp_unick, gamespy_gp::login_operation_cb());
#endif
}

void CCC_GameSpyDeleteProfile::Execute(LPCSTR args)
{
#ifdef XR_PLATFORM_WINDOWS
    VERIFY(MainMenu() && MainMenu()->GetGS());
    gamespy_gp::account_manager* tmp_amngr = MainMenu()->GetAccountMngr();
    VERIFY(tmp_amngr);
    tmp_amngr->delete_profile(gamespy_gp::account_operation_cb());
#endif
}

void CCC_GameSpyProfile::Execute(LPCSTR args)
{
#ifdef XR_PLATFORM_WINDOWS
    VERIFY(MainMenu());
    gamespy_gp::login_manager* tmp_lmngr = MainMenu()->GetLoginMngr();
    VERIFY(tmp_lmngr);
    gamespy_profile::profile_store* tmp_prof_store = MainMenu()->GetProfileStore();
    VERIFY(tmp_prof_store);

    gamespy_gp::profile const* tmp_curr_prof = tmp_lmngr->get_current_profile();
    if (!tmp_curr_prof)
    {
        Msg("- No profile. You are not loged in.");
        return;
    }

    string256 tmp_command;
    sscanf(args, "%s", tmp_command);
    if (!xr_strcmp(tmp_command, "load"))
    {
        /*tmp_prof_store->set_current_profile(
            tmp_curr_prof->m_profile_id,
            tmp_curr_prof->m_login_ticket.c_str()
        );
        parameters_tuple1<gamespy_profile::store_operation_cb> tmp_args;
        tmp_prof_store->load_profile(tmp_args);*/
        tmp_prof_store->load_current_profile(
            gamespy_profile::store_operation_cb(), gamespy_profile::store_operation_cb());
    }
    else if (!xr_strcmp(tmp_command, "reward") || !xr_strcmp(tmp_command, "bestscore"))
    {
        Log("~ This command is unsupported since old gamespy profile code has been removed from the engine.");
    }
#endif
}
