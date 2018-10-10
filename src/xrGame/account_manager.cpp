#include "StdAfx.h"
#include "account_manager.h"
#include "xrGameSpy/GameSpy_GP.h"

#include "MainMenu.h" //for accesssing to login_manager, if there's deleting profile
#include "login_manager.h" //for deleting profile (verifying) and deleting profile class instance

#define GP_UNIQUENICK_MIN_LEN 3

namespace gamespy_gp
{
account_manager::account_manager(CGameSpy_GP* gsgp_inst)
{
    m_gamespy_gp = gsgp_inst;
    VERIFY(m_gamespy_gp);
}

account_manager::~account_manager() {}
void __stdcall account_manager::only_log_creation_cb(bool success, char const* descr)
{
    if (success)
    {
        Msg("* New profile has created successfully !");
        return;
    }
    Msg("! GameSpy account creation ERROR: %s", descr ? descr : "unknown");
}

void __stdcall account_manager::only_log_profdel_cb(bool success, char const* descr)
{
    if (success)
    {
        Msg("* Profile has been deleted. And logged out.");
        return;
    }
    Msg("! FAILED to delete GameSpy profile: %s", descr ? descr : "unknown");
}

void __stdcall account_manager::only_log_profiles(u32 const profiles_count, char const* description)
{
    if (profiles_count)
    {
        Msg("- GameSpy account profiles:");
        for (profiles_store_t::const_iterator i = m_result_profiles.begin(), ie = m_result_profiles.end(); i != ie; ++i)
        {
            Msg("- %s", i->c_str());
        }
        return;
    }
    Msg("- No GameSpy account profiles found: %s", description ? description : "unknown error");
}
void __stdcall account_manager::only_log_suggestions(u32 const profiles_count, char const* description)
{
    if (description)
        Msg("- GameSpy suggested unique nicks: %s", description);

    for (suggested_nicks_t::const_iterator i = m_suggested_nicks.begin(), ie = m_suggested_nicks.end(); i != ie; ++i)
    {
        Msg("- %s", i->c_str());
    }
}

void __stdcall account_manager::only_log_found_email(bool found, char const* user_name)
{
    if (!found)
    {
        Msg("- Such email not found: %s", user_name ? user_name : "");
        return;
    }
    Msg("- Found email, user nick is: %s", user_name);
}

bool account_manager::verify_nick(char const* nick)
{
    if (!nick)
    {
        Msg("! ERROR: nick name is empty");
        m_verifyer_error = "mp_gp_no_nick";
        return false;
    }
    u32 nick_length = xr_strlen(nick);
    if (nick_length == 0)
    {
        Msg("! ERROR: nick name is empty");
        m_verifyer_error = "mp_gp_no_nick";
        return false;
    }

    if (nick_length >= GP_NICK_LEN)
    {
        Msg("! ERROR: nick name is empty");
        m_verifyer_error = "mp_gp_nick_is_too_big";
        return false;
    }
    return true;
}

bool account_manager::verify_unique_nick(char const* unick)
{
    static char const* denyed_first_symbols = "@+:#1234567890";
    if (!unick)
    {
        Msg("! ERROR: unique nick name is empty");
        m_verifyer_error = "mp_gp_no_unique_nick";
        return false;
    }
    u32 unick_length = xr_strlen(unick);
    if (unick_length == 0)
    {
        Msg("! ERROR: unique nick name is empty");
        m_verifyer_error = "mp_gp_no_unique_nick";
        return false;
    }
    if (unick_length < GP_UNIQUENICK_MIN_LEN)
    {
        Msg("! ERROR: unique nick name is too short (must be greater that 2 character)");
        m_verifyer_error = "mp_gp_unique_nick_too_short";
        return false;
    }
    if (unick_length >= GP_UNIQUENICK_LEN)
    {
        Msg("! ERROR: nick name is too big");
        m_verifyer_error = "mp_gp_unique_nick_is_too_big";
        return false;
    }

    if (strchr(denyed_first_symbols, unick[0]))
    {
        Msg("! ERROR: first symbol is invalid");
        m_verifyer_error = "mp_gp_unique_nick_bad_first_symbol";
        return false;
    }

    if (strchr(unick, ' '))
    {
        Msg("! ERROR: unique nick can't contain spaces");
        m_verifyer_error = "mp_gp_unique_nick_must_contain_nospaces";
        return false;
    }

    for (u32 i = 0; i < unick_length; ++i)
    {
        bool valid = (unick[i] >= 34) && (unick[i] <= 126) && (unick[i] != 44) && (unick[i] != 92) &&
            (unick[i] != 39) && (unick[i] != '%');
        if (!valid)
        {
            Msg("! ERROR: bad %d symbol", i);
            m_verifyer_error = "mp_gp_unique_nick_must_contain_only";
            return false;
        }
    }
    return true;
}

bool account_manager::verify_email(char const* email)
{
    if (!email || (xr_strlen(email) == 0))
    {
        Msg("! ERROR: email is empty");
        m_verifyer_error = "mp_gp_no_email";
        return false;
    }
    u32 email_length = xr_strlen(email);
    if (email_length >= GP_EMAIL_LEN)
    {
        Msg("! ERROR: email is too big");
        m_verifyer_error = "mp_gp_email_is_too_big";
        return false;
    }
    char const* tmp_endchar = email + email_length;
    char const* tmp_char = std::find(email, tmp_endchar, '@');
    if ((tmp_char == tmp_endchar) || (tmp_char == email) || (tmp_char + 1 == tmp_endchar) ||
        !isalnum(*(tmp_char + 1)) || !isalnum(*(tmp_char - 1)))
    {
        Msg("! ERROR: bad email");
        m_verifyer_error = "mp_gp_bad_email";
        return false;
    }
    return true;
}

bool account_manager::verify_password(char const* pass)
{
    if (!pass)
    {
        Msg("! ERROR: password is empty");
        m_verifyer_error = "mp_gp_password_is_too_small";
        return false;
    }
    u32 pass_length = xr_strlen(pass);
    if (pass_length <= 1)
    {
        Msg("! ERROR: password is too small, must be greater than 1 symbol");
        m_verifyer_error = "mp_gp_password_is_too_small";
        return false;
    }
    if (pass_length >= GP_PASSWORD_LEN)
    {
        Msg("! ERROR: password is too big");
        m_verifyer_error = "mp_gp_password_is_too_big";
        return false;
    }
    return true;
}

void account_manager::create_profile(
    char const* nick, char const* unique_nick, char const* email, char const* password, account_operation_cb opcb)
{
    if (!opcb)
        m_account_creation_cb.bind(this, &account_manager::only_log_creation_cb);
    else
        m_account_creation_cb = opcb;

    if (!verify_nick(nick) || !verify_unique_nick(unique_nick) || !verify_email(email) || !verify_password(password))
    {
        m_account_creation_cb(false, get_verify_error_descr());
        return;
    }

    GPResult tmp_res = m_gamespy_gp->NewUser(nick, unique_nick, email, password, &account_manager::new_user_cb, this);
    if (tmp_res != GP_NO_ERROR)
    {
        m_account_creation_cb(false, CGameSpy_GP::TryToTranslate(tmp_res).c_str());
    }
}

void account_manager::delete_profile(account_operation_cb dpcb)
{
    if (!dpcb)
    {
        m_profile_deleting_cb.bind(this, &account_manager::only_log_profdel_cb);
    }
    else
    {
        m_profile_deleting_cb = dpcb;
    }
#ifdef WINDOWS
    login_manager* tmp_lmngr = MainMenu()->GetLoginMngr();
    VERIFY(tmp_lmngr);
    if (!tmp_lmngr->get_current_profile())
    {
        m_profile_deleting_cb(false, "mp_gp_not_logged_in");
        return;
    }

    GPResult tmp_res = m_gamespy_gp->DeleteProfile(&account_manager::delete_profile_cb, this);
    if (tmp_res != GP_NO_ERROR)
    {
        m_profile_deleting_cb(false, CGameSpy_GP::TryToTranslate(tmp_res).c_str());
    }
#endif
}

void account_manager::get_account_profiles(char const* email, char const* password, account_profiles_cb profiles_cb)
{
    if (!profiles_cb)
    {
        profiles_cb.bind(this, &account_manager::only_log_profiles);
    }

    m_result_profiles.clear();
    m_result_profiles_ptrs.clear();

    get_account_params_t tmp_args(email, password);
    m_get_account_profiles_qam.execute(this, tmp_args, profiles_cb);
}

bool account_manager::is_get_account_profiles_active() const { return m_get_account_profiles_qam.is_active(); }
void account_manager::reinit_get_account_profiles()
{
    m_account_profiles_cb.clear();
    m_get_account_profiles_qam.reexecute();
}

void account_manager::stop_fetching_account_profiles() { m_get_account_profiles_qam.stop(); }
void account_manager::get_account_profiles_raw(get_account_params_t const& args, account_profiles_cb profiles_cb)
{
    VERIFY(!m_account_profiles_cb);
    m_account_profiles_cb = profiles_cb;

    GPResult tmp_res =
        m_gamespy_gp->GetUserNicks(args.m_t1.c_str(), args.m_t2.c_str(), &account_manager::user_nicks_cb, this);

    if (tmp_res != GP_NO_ERROR)
    {
        m_account_profiles_cb.clear();
        profiles_cb(0, CGameSpy_GP::TryToTranslate(tmp_res).c_str());
        return;
    }
}

void account_manager::release_account_profiles(u32 const profiles_count, char const* description)
{
    m_result_profiles.clear();
    m_result_profiles_ptrs.clear();
}

void account_manager::search_for_email(char const* email, found_email_cb found_cb)
{
    if (!found_cb)
    {
        found_cb.bind(this, &account_manager::only_log_found_email);
    }

    if (!email || (xr_strlen(email) == 0))
    {
        found_cb(false, "mp_gp_no_email");
        return;
    }
    search_for_email_params_t tmp_args(email);
    m_search_for_email_qam.execute(this, tmp_args, found_cb);
}

bool account_manager::is_email_searching_active() const { return m_search_for_email_qam.is_active(); }
void account_manager::reinit_email_searching()
{
    m_found_email_cb.clear();
    m_search_for_email_qam.reexecute();
}

void account_manager::search_for_email_raw(search_for_email_params_t const& email, found_email_cb found_email_cb)
{
    VERIFY(!m_found_email_cb);
    m_found_email_cb = found_email_cb;

    GPResult tmp_res = m_gamespy_gp->ProfileSearch(
        shared_str(), shared_str(), email.m_t1.c_str(), &account_manager::search_profile_cb, this);

    if (tmp_res != GP_NO_ERROR)
    {
        m_found_email_cb.clear();
        found_email_cb(false, CGameSpy_GP::TryToTranslate(tmp_res).c_str());
        return;
    }
}

void account_manager::release_found_email(bool found, char const* user_name){};

void account_manager::stop_searching_email() { m_search_for_email_qam.stop(); }
void account_manager::suggest_unique_nicks(char const* unick, suggest_nicks_cb sncb)
{
    if (!sncb)
    {
        sncb.bind(this, &account_manager::only_log_suggestions);
    }

    suggest_uniqie_nicks_params_t tmp_args(unick);
    m_suggest_uniqie_nicks_qam.execute(this, tmp_args, sncb);
}

void account_manager::stop_suggest_unique_nicks() { m_suggest_uniqie_nicks_qam.stop(); }
void account_manager::suggest_unique_nicks_raw(suggest_uniqie_nicks_params_t const& unick, suggest_nicks_cb sncb)
{
    VERIFY(!m_suggest_nicks_cb);
    m_suggest_nicks_cb = sncb;

    m_suggested_nicks.clear();
    m_suggested_nicks_ptrs.clear();

    GPResult tmp_res = m_gamespy_gp->SuggestUNicks(unick.m_t1, &account_manager::unicks_suggestion_cb, this);

    if (tmp_res != GP_NO_ERROR)
    {
        sncb(0, CGameSpy_GP::TryToTranslate(tmp_res).c_str());
        m_suggest_nicks_cb.clear();
        return;
    }
}

void account_manager::release_suggest_uniqie_nicks(u32 const, char const*) {}
bool account_manager::is_suggest_unique_nicks_active() const { return m_suggest_uniqie_nicks_qam.is_active(); }
void account_manager::reinit_suggest_unique_nicks()
{
    m_suggest_nicks_cb.clear();
    m_suggest_uniqie_nicks_qam.reexecute();
}

void __cdecl account_manager::new_user_cb(GPConnection* connection, void* arg, void* param)
{
    GPNewUserResponseArg* creation_resp = static_cast<GPNewUserResponseArg*>(arg);
    VERIFY(creation_resp != NULL);
    account_manager* tmp_inst = static_cast<account_manager*>(param);
    VERIFY(tmp_inst);
    VERIFY(tmp_inst->m_account_creation_cb);
    if (creation_resp->result != GP_NO_ERROR)
    {
        tmp_inst->m_account_creation_cb(false, CGameSpy_GP::TryToTranslate(creation_resp->result).c_str());
        return;
    }
    tmp_inst->m_account_creation_cb(true, "");
}

void __cdecl account_manager::user_nicks_cb(GPConnection* connection, void* arg, void* param)
{
    account_manager* tmp_inst = static_cast<account_manager*>(param);
    VERIFY(tmp_inst);
    GPGetUserNicksResponseArg* tmp_arg = static_cast<GPGetUserNicksResponseArg*>(arg);

    account_profiles_cb tmp_cb = tmp_inst->m_account_profiles_cb;
    tmp_inst->m_account_profiles_cb.clear();

    if (tmp_arg->result != GP_NO_ERROR)
    {
        tmp_cb(0, CGameSpy_GP::TryToTranslate(tmp_arg->result).c_str());
        return;
    }

    for (int i = 0; i < tmp_arg->numNicks; ++i)
    {
        tmp_inst->m_result_profiles.push_back(tmp_arg->nicks[i]);
        tmp_inst->m_result_profiles_ptrs.push_back(tmp_inst->m_result_profiles.back().c_str());
    }
    tmp_cb(tmp_arg->numNicks, "");
}

void __cdecl account_manager::unicks_suggestion_cb(GPConnection* connection, void* arg, void* param)
{
    account_manager* tmp_inst = static_cast<account_manager*>(param);
    VERIFY(tmp_inst);
    GPSuggestUniqueNickResponseArg* tmp_arg = static_cast<GPSuggestUniqueNickResponseArg*>(arg);
    VERIFY(tmp_arg);
    suggest_nicks_cb tmp_cb = tmp_inst->m_suggest_nicks_cb;
    tmp_inst->m_suggest_nicks_cb.clear();

    if (tmp_arg->result != GP_NO_ERROR)
    {
        tmp_cb(0, CGameSpy_GP::TryToTranslate(tmp_arg->result).c_str());
        return;
    }

    for (int i = 0; i < tmp_arg->numSuggestedNicks; ++i)
    {
        tmp_inst->m_suggested_nicks.push_back(tmp_arg->suggestedNicks[i]);
        tmp_inst->m_suggested_nicks_ptrs.push_back(tmp_inst->m_suggested_nicks.back().c_str());
    }
    tmp_cb(tmp_arg->numSuggestedNicks, "");
}

void __cdecl account_manager::delete_profile_cb(GPConnection* connection, void* arg, void* param)
{
    account_manager* tmp_inst = static_cast<account_manager*>(param);
    VERIFY(tmp_inst);
    GPDeleteProfileResponseArg* tmp_arg = static_cast<GPDeleteProfileResponseArg*>(arg);
    if (tmp_arg->result != GP_NO_ERROR)
    {
        tmp_inst->m_profile_deleting_cb(false, CGameSpy_GP::TryToTranslate(tmp_arg->result).c_str());
        return;
    }
    VERIFY(tmp_inst->m_gamespy_gp);
#ifdef WINDOWS
    login_manager* tmp_lmngr = MainMenu()->GetLoginMngr();
    VERIFY(tmp_lmngr);
    tmp_lmngr->delete_profile_obj();
    tmp_inst->m_profile_deleting_cb(true, "");
#endif
}

void __cdecl account_manager::search_profile_cb(GPConnection* connection, void* arg, void* param)
{
    account_manager* tmp_inst = static_cast<account_manager*>(param);
    VERIFY(tmp_inst);
    GPProfileSearchResponseArg* tmp_arg = static_cast<GPProfileSearchResponseArg*>(arg);

    found_email_cb tmp_cb = tmp_inst->m_found_email_cb;
    tmp_inst->m_found_email_cb.clear();

    if (tmp_arg->result != GP_NO_ERROR)
    {
        tmp_cb(false, CGameSpy_GP::TryToTranslate(tmp_arg->result).c_str());
        return;
    }
    if (tmp_arg->numMatches == 0)
    {
        tmp_cb(false, "");
        return;
    }
    GPProfileSearchMatch* first_match = tmp_arg->matches;
    VERIFY(first_match->nick);
    tmp_cb(true, first_match->nick);
}

} // namespace gamespy_gp
