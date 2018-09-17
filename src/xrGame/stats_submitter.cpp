#include "StdAfx.h"
#include "stats_submitter.h"
#include "xrGameSpy/GameSpy_Full.h"
#include "login_manager.h"
#include "MainMenu.h"
#include "profile_store.h"
#include "awards_store.h"
#include "best_scores_store.h"

namespace gamespy_profile
{
stats_submitter::stats_submitter(CGameSpy_Full* fullgs)
    : m_ltx_file(p_number, q_number, g_number, &stats_submitter::fill_private_key)
{
    shedule.t_min = 500;
    shedule.t_max = 1000;
    VERIFY(fullgs);
    m_fullgs_obj = fullgs;
    m_atlas_obj = fullgs->GetGameSpyATLAS();
    VERIFY(m_atlas_obj);
    m_last_operation_profile = NULL;
    m_atlas_report = NULL;
    m_last_best_scores = NULL;
    ZeroMemory(m_atlas_connection_id, sizeof(m_atlas_connection_id));
}

stats_submitter::~stats_submitter() {}
void stats_submitter::reward_with_award(
    enum_awards_t award_id, u32 const count, gamespy_gp::profile const* profile, store_operation_cb opcb)
{
    if (!opcb)
    {
        m_last_operation_cb.bind(this, &stats_submitter::onlylog_operation);
    }
    else
    {
        m_last_operation_cb = opcb;
    }
    R_ASSERT(!m_last_operation_profile);

    m_last_operation_profile = profile;
    m_report_type = ert_set_award;
    m_last_award_id = award_id;
    m_last_award_count = count;

    begin_session();
}

void stats_submitter::set_best_scores(
    all_best_scores_t const* scores, gamespy_gp::profile const* profile, store_operation_cb opcb)
{
    if (!opcb)
    {
        m_last_operation_cb.bind(this, &stats_submitter::onlylog_operation);
    }
    else
    {
        m_last_operation_cb = opcb;
    }
    R_ASSERT(!m_last_operation_profile);

    m_last_operation_profile = profile;
    m_report_type = ert_set_best_scores;
    m_last_best_scores = scores;

    begin_session();
}

void stats_submitter::submit_all(all_awards_t const* awards, all_best_scores_t const* scores,
    gamespy_gp::profile const* profile, store_operation_cb opcb)
{
    if (!opcb)
    {
        m_last_operation_cb.bind(this, &stats_submitter::onlylog_operation);
    }
    else
    {
        m_last_operation_cb = opcb;
    }
    R_ASSERT(!m_last_operation_profile);

    m_last_operation_profile = profile;
    m_report_type = ert_synchronize_profile;
    m_last_all_awards = awards;
    m_last_best_scores = scores;

    begin_session();
}

void stats_submitter::shedule_Update(u32 dt)
{
    VERIFY(m_fullgs_obj && m_atlas_obj);
#ifdef DEBUG
    Msg("--- GameSpy core and atlas thinking ...");
#endif
    m_fullgs_obj->CoreThink(10); // 10 milliseconds on update
    m_atlas_obj->Think();
}

void __stdcall stats_submitter::onlylog_operation(bool const result, char const* err_descr)
{
    if (!result)
    {
        Msg("! Store operation ERROR: %s", err_descr ? err_descr : "unknown");
        return;
    }
    Msg("* Store operation successfullly complete.");
}

u32 const stats_submitter::operation_timeout_value = 60000; // 60 seconds
void stats_submitter::begin_session()
{
    VERIFY(m_last_operation_profile && m_last_operation_cb);
    SCResult tmp_result = m_atlas_obj->CreateSession(&m_last_operation_profile->mCertificate,
        &m_last_operation_profile->mPrivateData, &stats_submitter::created_session_cb, operation_timeout_value, this);

    if (tmp_result != SCResult_NO_ERROR)
    {
        m_last_operation_cb(false, CGameSpy_ATLAS::TryToTranslate(tmp_result).c_str());
        terminate_session();
        return;
    }

    Engine.Sheduler.Register(this, FALSE);
}

void __cdecl stats_submitter::created_session_cb(
    const SCInterfacePtr theInterface, GHTTPResult theHttpResult, SCResult theResult, void* theUserData)
{
    stats_submitter* my_inst = static_cast<stats_submitter*>(theUserData);
    VERIFY(my_inst && my_inst->m_last_operation_profile);
    if (theHttpResult != GHTTPSuccess)
    {
        my_inst->m_last_operation_cb(false, CGameSpy_ATLAS::TryToTranslate(theHttpResult).c_str());
        my_inst->terminate_session();
        Engine.Sheduler.Unregister(my_inst);
        return;
    }
    if (theResult != SCResult_NO_ERROR)
    {
        my_inst->m_last_operation_cb(false, CGameSpy_ATLAS::TryToTranslate(theResult).c_str());
        my_inst->terminate_session();
        Engine.Sheduler.Unregister(my_inst);
        return;
    }
    SCResult tmp_result = my_inst->m_atlas_obj->SetReportIntention(NULL, gsi_true,
        &my_inst->m_last_operation_profile->mCertificate, &my_inst->m_last_operation_profile->mPrivateData,
        &stats_submitter::set_intension_cb, stats_submitter::operation_timeout_value, my_inst);

    if (tmp_result != SCResult_NO_ERROR)
    {
        my_inst->m_last_operation_cb(false, CGameSpy_ATLAS::TryToTranslate(tmp_result).c_str());
        my_inst->terminate_session();
        Engine.Sheduler.Unregister(my_inst);
        return;
    }
}

void __cdecl stats_submitter::set_intension_cb(
    const SCInterfacePtr theInterface, GHTTPResult theHttpResult, SCResult theResult, void* theUserData)
{
    stats_submitter* my_inst = static_cast<stats_submitter*>(theUserData);
    VERIFY(my_inst && my_inst->m_last_operation_profile);
    if (theHttpResult != GHTTPSuccess)
    {
        my_inst->m_last_operation_cb(false, CGameSpy_ATLAS::TryToTranslate(theHttpResult).c_str());
        my_inst->terminate_session();
        Engine.Sheduler.Unregister(my_inst);
        return;
    }
    if (theResult != SCResult_NO_ERROR)
    {
        my_inst->m_last_operation_cb(false, CGameSpy_ATLAS::TryToTranslate(theResult).c_str());
        my_inst->terminate_session();
        Engine.Sheduler.Unregister(my_inst);
        return;
    }
    char const* tmp_connection_id = my_inst->m_atlas_obj->GetConnectionId();
    VERIFY(tmp_connection_id);
    xr_strcpy(static_cast<char*>((void*)my_inst->m_atlas_connection_id), sizeof(my_inst->m_atlas_connection_id),
        tmp_connection_id);

    if (!my_inst->prepare_report())
    {
        my_inst->m_last_operation_cb(false, "mp_failed_to_create_report");
        my_inst->terminate_session();
        Engine.Sheduler.Unregister(my_inst);
        return;
    }

    SCResult tmp_result = my_inst->m_atlas_obj->SubmitReport(my_inst->m_atlas_report, gsi_true,
        &my_inst->m_last_operation_profile->mCertificate, &my_inst->m_last_operation_profile->mPrivateData,
        &stats_submitter::submitted_cb, stats_submitter::operation_timeout_value, my_inst);
    if (tmp_result != SCResult_NO_ERROR)
    {
        my_inst->m_last_operation_cb(false, CGameSpy_ATLAS::TryToTranslate(tmp_result).c_str());
        my_inst->terminate_session();
        Engine.Sheduler.Unregister(my_inst);
        return;
    }
}

void __cdecl stats_submitter::submitted_cb(
    const SCInterfacePtr theInterface, GHTTPResult theHttpResult, SCResult theResult, void* theUserData)
{
    stats_submitter* my_inst = static_cast<stats_submitter*>(theUserData);
    VERIFY(my_inst && my_inst->m_last_operation_profile);
    if (theHttpResult != GHTTPSuccess)
    {
        my_inst->m_last_operation_cb(false, CGameSpy_ATLAS::TryToTranslate(theHttpResult).c_str());
        my_inst->terminate_session();
        Engine.Sheduler.Unregister(my_inst);
        return;
    }
    if (theResult != SCResult_NO_ERROR)
    {
        my_inst->m_last_operation_cb(false, CGameSpy_ATLAS::TryToTranslate(theResult).c_str());
        my_inst->terminate_session();
        Engine.Sheduler.Unregister(my_inst);
        return;
    }

    my_inst->m_last_operation_cb(true, "");
    my_inst->terminate_session();
    Engine.Sheduler.Unregister(my_inst);
}

bool stats_submitter::prepare_report()
{
    VERIFY(!m_atlas_report);
    SCResult tmp_res = m_atlas_obj->CreateReport(ATLAS_RULE_SET_VERSION, 1, 0, &m_atlas_report);
    VERIFY2((tmp_res == SCResult_NO_ERROR) && (m_atlas_report), "failed to create atlas report");
    if ((tmp_res != SCResult_NO_ERROR) || (!m_atlas_report))
        return false;

    tmp_res = m_atlas_obj->ReportBeginGlobalData(m_atlas_report);
    VERIFY2(tmp_res == SCResult_NO_ERROR, "failed to begin global data");
    if (tmp_res != SCResult_NO_ERROR)
        return false;

    tmp_res = m_atlas_obj->ReportBeginPlayerData(m_atlas_report);
    VERIFY2(tmp_res == SCResult_NO_ERROR, "failed to begin player data");
    if (tmp_res != SCResult_NO_ERROR)
        return false;

    tmp_res = m_atlas_obj->ReportBeginNewPlayer(m_atlas_report);
    VERIFY2(tmp_res == SCResult_NO_ERROR, "failed to begin new player data");
    if (tmp_res != SCResult_NO_ERROR)
        return false;

    tmp_res = m_atlas_obj->ReportSetPlayerData(m_atlas_report, 0, m_atlas_connection_id, 0, SCGameResult_NONE,
        m_last_operation_profile->m_profile_id, &m_last_operation_profile->mCertificate);
    VERIFY2(tmp_res == SCResult_NO_ERROR, "failed to set player data");
    if (tmp_res != SCResult_NO_ERROR)
        return false;

    bool report_creation_result = false;
    switch (m_report_type)
    {
    case ert_set_award: { report_creation_result = create_award_inc_report();
    }
    break;
    case ert_set_best_scores: { report_creation_result = create_best_scores_report();
    }
    break;
    case ert_synchronize_profile:
    {
        report_creation_result = create_all_awards_report();
        report_creation_result &= create_best_scores_report();
    }
    break;
    default: NODEFAULT;
    }; // switch (m_report_type)
    if (report_creation_result)
    {
        tmp_res = m_atlas_obj->ReportEnd(m_atlas_report, gsi_true, SCGameStatus_COMPLETE);
        VERIFY(tmp_res == SCResult_NO_ERROR);
        return (tmp_res == SCResult_NO_ERROR);
    }
    return false;
}

bool stats_submitter::add_player_name_to_report()
{
    SCResult tmp_res =
        m_atlas_obj->ReportAddStringValue(m_atlas_report, KEY_PlayerName, m_last_operation_profile->unique_nick());
    VERIFY(tmp_res == SCResult_NO_ERROR);
    if (tmp_res != SCResult_NO_ERROR)
        return false;

    return true;
}

bool stats_submitter::create_award_inc_report()
{
#ifdef LINUX // FIXME!!
    return false;
#else
    __time32_t tmp_time = 0;
    _time32(&tmp_time);

    SCResult tmp_res = m_atlas_obj->ReportAddIntValue(
        m_atlas_report, get_award_id_key(m_last_award_id), static_cast<gsi_i32>(m_last_award_count));
    VERIFY(tmp_res == SCResult_NO_ERROR);
    if (tmp_res != SCResult_NO_ERROR)
        return false;

    tmp_res = m_atlas_obj->ReportAddIntValue(
        m_atlas_report, get_award_reward_date_key(m_last_award_id), static_cast<gsi_i32>(tmp_time));
    VERIFY(tmp_res == SCResult_NO_ERROR);
    if (tmp_res != SCResult_NO_ERROR)
        return false;

    return add_player_name_to_report();
#endif
}

bool stats_submitter::create_best_scores_report()
{
    VERIFY(m_last_best_scores);
    SCResult tmp_res;
    for (all_best_scores_t::const_iterator si = m_last_best_scores->begin(), sie = m_last_best_scores->end(); si != sie;
         ++si)
    {
        tmp_res = m_atlas_obj->ReportAddIntValue(m_atlas_report,
            get_best_score_id_key(static_cast<gamespy_profile::enum_best_score_type>(si->first)), si->second);
        VERIFY(tmp_res == SCResult_NO_ERROR);
        if (tmp_res != SCResult_NO_ERROR)
            return false;
    }
    return add_player_name_to_report();
}

bool stats_submitter::create_all_awards_report()
{
    VERIFY(m_last_all_awards);
    for (all_awards_t::const_iterator ai = m_last_all_awards->begin(), aie = m_last_all_awards->end(); ai != aie; ++ai)
    {
        SCResult tmp_res = m_atlas_obj->ReportAddIntValue(
            m_atlas_report, get_award_id_key(ai->first), static_cast<gsi_i32>(ai->second.m_count));
        VERIFY(tmp_res == SCResult_NO_ERROR);
        if (tmp_res != SCResult_NO_ERROR)
            return false;

        tmp_res = m_atlas_obj->ReportAddIntValue(
            m_atlas_report, get_award_reward_date_key(ai->first), static_cast<gsi_i32>(ai->second.m_last_reward_date));
        VERIFY(tmp_res == SCResult_NO_ERROR);
        if (tmp_res != SCResult_NO_ERROR)
            return false;
    }
    return add_player_name_to_report();
}

void stats_submitter::terminate_session()
{
    m_last_operation_cb.clear();
    m_last_operation_profile = NULL;
    m_atlas_report = NULL;
    m_last_best_scores = NULL;
    ZeroMemory(m_atlas_connection_id, sizeof(m_atlas_connection_id));
}

void stats_submitter::quick_reward_with_award(enum_awards_t award_id, gamespy_gp::profile const* profile)
{
    profile_store* tmp_prof_store = MainMenu()->GetProfileStore();
    VERIFY(tmp_prof_store);
    awards_store* tmp_awards_store = tmp_prof_store->get_awards_store();
    VERIFY(tmp_awards_store);

    all_awards_t& tmp_awards = tmp_awards_store->get_player_awards();

    all_awards_t::iterator award_iter = tmp_awards.find(award_id);
    R_ASSERT(award_iter != tmp_awards.end());
    ++award_iter->second.m_count;
#ifndef LINUX // FIXME!!!
    __time32_t tmp_time = 0;
    _time32(&tmp_time);
    award_iter->second.m_last_reward_date = static_cast<u32>(tmp_time);
#endif
    save_file(profile);
}

void stats_submitter::quick_set_best_scores(all_best_scores_t const* scores, gamespy_gp::profile const* profile)
{
    VERIFY(scores);
    profile_store* tmp_prof_store = MainMenu()->GetProfileStore();
    VERIFY(tmp_prof_store);
    best_scores_store* tmp_bs_store = tmp_prof_store->get_best_scores_store();
    VERIFY(tmp_bs_store);

    all_best_scores_t& tmp_best_scores = tmp_bs_store->get_player_best_scores();

    for (all_best_scores_t::iterator i = tmp_best_scores.begin(), ie = tmp_best_scores.end(); i != ie; ++i)
    {
        all_best_scores_t::const_iterator tmp_iter = scores->find(i->first);
        if (tmp_iter == scores->end())
            continue;
        i->second = std::max(i->second, tmp_iter->second);
    }
    save_file(profile);
}

void stats_submitter::save_file(gamespy_gp::profile const* profile)
{
    profile_store* tmp_prof_store = MainMenu()->GetProfileStore();
    VERIFY(tmp_prof_store);
    awards_store* tmp_awards_store = tmp_prof_store->get_awards_store();
    VERIFY(tmp_awards_store);
    best_scores_store* tmp_bs_store = tmp_prof_store->get_best_scores_store();
    VERIFY(tmp_prof_store);

    all_awards_t& tmp_awards = tmp_awards_store->get_player_awards();
    all_best_scores_t& tmp_best_scores = tmp_bs_store->get_player_best_scores();

    CInifile& ltx_to_write = m_ltx_file.get_ltx();
    ltx_to_write.sections().clear();

    for (all_awards_t::const_iterator i = tmp_awards.begin(), ie = tmp_awards.end(); i != ie; ++i)
    {
        LPCSTR tmp_award_name = get_award_name(i->first);
        ltx_to_write.w_u16(tmp_award_name, award_count_line, i->second.m_count);
        ltx_to_write.w_u32(tmp_award_name, award_rdate_line, i->second.m_last_reward_date);
    }

    for (all_best_scores_t::const_iterator i = tmp_best_scores.begin(), ie = tmp_best_scores.end(); i != ie; ++i)
    {
        LPCSTR tmp_bs_name = get_best_score_name(i->first);
        ltx_to_write.w_u32(tmp_bs_name, best_score_value_line, i->second);
    }

#ifndef LINUX // FIXME!!!
    __time32_t tmp_time = 0;
    _time32(&tmp_time);

    ltx_to_write.w_s32(profile_data_section, profile_id_line, profile->m_profile_id);
    ltx_to_write.w_u32(profile_data_section, profile_last_submit_time, static_cast<u32>(tmp_time));
#endif
    IWriter* tmp_writer = FS.w_open("$app_data_root$", profile_store_file_name);
    m_ltx_file.sign_and_save(*tmp_writer);
    FS.w_close(tmp_writer);
}

void stats_submitter::fill_private_key(crypto::xr_dsa::private_key_t& priv_key_dest)
{
    priv_key_dest.m_value[0] = 0x82;
    priv_key_dest.m_value[1] = 0xf0;
    priv_key_dest.m_value[2] = 0x41;
    priv_key_dest.m_value[3] = 0xe9;
    priv_key_dest.m_value[4] = 0x9a;
    priv_key_dest.m_value[5] = 0x98;
    priv_key_dest.m_value[6] = 0xa0;
    priv_key_dest.m_value[7] = 0xf9;
    priv_key_dest.m_value[8] = 0xff;
    priv_key_dest.m_value[9] = 0xe1;
    priv_key_dest.m_value[10] = 0x14;
    priv_key_dest.m_value[11] = 0x48;
    priv_key_dest.m_value[12] = 0xb3;
    priv_key_dest.m_value[13] = 0xde;
    priv_key_dest.m_value[14] = 0xe3;
    priv_key_dest.m_value[15] = 0x69;
    priv_key_dest.m_value[16] = 0xea;
    priv_key_dest.m_value[17] = 0x16;
    priv_key_dest.m_value[18] = 0x64;
    priv_key_dest.m_value[19] = 0xf2;
}

} // namespace gamespy_profile
