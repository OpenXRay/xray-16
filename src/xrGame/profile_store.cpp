#include "StdAfx.h"
#include "profile_store.h"
#include "xrGameSpy/GameSpy_Full.h"
#include "xrGameSpy/GameSpy_SAKE.h"
#include "MainMenu.h"
#include "login_manager.h"
#include "awards_store.h"
#include "best_scores_store.h"
#include "stats_submitter.h"
#include "atlas_submit_queue.h"

namespace gamespy_profile
{
profile_store::profile_store(CGameSpy_Full* fullgs_obj)
    : m_dsigned_reader(
          stats_submitter::p_number, stats_submitter::q_number, stats_submitter::g_number, stats_submitter::public_key)
{
    VERIFY(fullgs_obj && fullgs_obj->GetGameSpySAKE());
    m_fullgs_obj = fullgs_obj;
    m_sake_obj = fullgs_obj->GetGameSpySAKE();

    m_awards_store = new awards_store(fullgs_obj);
    m_best_scores_store = new best_scores_store(fullgs_obj);
}

profile_store::~profile_store()
{
    if (m_progress_indicator)
        Engine.Sheduler.Unregister(this);

    xr_delete(m_awards_store);
    xr_delete(m_best_scores_store);
}

all_awards_t const& profile_store::get_awards()
{
    VERIFY(m_awards_store);
    return m_awards_store->get_player_awards();
}

all_best_scores_t const& profile_store::get_best_scores()
{
    VERIFY(m_best_scores_store);
    return m_best_scores_store->get_player_best_scores();
}

void profile_store::shedule_Update(u32 dt)
{
    VERIFY(m_fullgs_obj);
#ifdef DEBUG
    Msg("--- GameSpy core (SAKE) thinking ...");
#endif
    m_fullgs_obj->CoreThink(10); // 10 milliseconds on update
}

void profile_store::set_current_profile(int profileId, char const* loginTicket)
{
    VERIFY(m_sake_obj);
    m_sake_obj->SetProfile(profileId, loginTicket);
}

void profile_store::load_current_profile(store_operation_cb progress_indicator_cb, store_operation_cb complete_cb)
{
#ifdef WINDOWS
    if (!complete_cb)
    {
        complete_cb.bind(this, &profile_store::onlylog_completion);
    }
    gamespy_gp::login_manager* tmp_lmngr = MainMenu()->GetLoginMngr();
    R_ASSERT(tmp_lmngr);
    gamespy_gp::profile const* tmp_curr_prof = tmp_lmngr->get_current_profile();
    if (!tmp_curr_prof)
    {
        complete_cb(false, "mp_first_need_to_login");
        return;
    }
    set_current_profile(tmp_curr_prof->m_profile_id, tmp_curr_prof->m_login_ticket.c_str());

    load_prof_params_t tmp_args(progress_indicator_cb);
    m_load_current_profile_qam.execute(this, tmp_args, complete_cb);
#endif
}

void profile_store::load_current_profile_raw(load_prof_params_t const& args, store_operation_cb complete_cb)
{
    if (m_complete_cb)
    {
        Msg("! ERROR: loading already in progress.");
        if (complete_cb)
        {
            complete_cb(false, "mp_loading_already_in_progress");
        }
        return;
    }
    m_complete_cb = complete_cb;
    load_profile(args.m_t1);
}

void profile_store::stop_loading()
{
    m_load_current_profile_qam.stop();
    m_progress_indicator.clear();
    m_progress_indicator.bind(this, &profile_store::onlylog_operation);
}

void profile_store::release_current_profile(bool, char const*) {}
void profile_store::load_profile(store_operation_cb progress_indicator_cb)
{
    VERIFY(!m_progress_indicator);
    if (!progress_indicator_cb)
    {
        m_progress_indicator.bind(this, &profile_store::onlylog_operation);
    }
    else
    {
        m_progress_indicator = progress_indicator_cb;
    }

    string_path tmp_path;
    FS.update_path(tmp_path, "$app_data_root$", profile_store_file_name);
    IReader* tmp_reader = NULL;
    m_valid_ltx = false;

    if (FS.exist(tmp_path))
    {
        tmp_reader = FS.r_open("$app_data_root$", profile_store_file_name);
    }

    if (tmp_reader)
    {
        u32 const tmp_length = tmp_reader->length();
        if (tmp_length)
        {
            m_valid_ltx = m_dsigned_reader.load_and_verify(static_cast<u8*>(tmp_reader->pointer()), tmp_length);
            FS.r_close(tmp_reader);
        }
    }
#ifdef WINDOWS
    if (m_valid_ltx)
    {
        s32 tmp_profile_id = m_dsigned_reader.get_ltx().r_s32(profile_data_section, profile_id_line);
        gamespy_gp::login_manager* tmp_lmngr = MainMenu()->GetLoginMngr();
        R_ASSERT(tmp_lmngr);
        gamespy_gp::profile const* tmp_curr_prof = tmp_lmngr->get_current_profile();
        R_ASSERT(tmp_curr_prof);
        m_valid_ltx = (tmp_profile_id == tmp_curr_prof->m_profile_id);
    }
#endif
    m_awards_store->reset_awards();
    m_best_scores_store->reset_scores();

    merge_fields(m_best_scores_store->get_field_names(), m_awards_store->get_field_names());

    m_progress_indicator(true, "mp_loading_awards");
    // m_progress_indicator		(true, "mp_loading_best_scores"); - merged
    Engine.Sheduler.Register(this, FALSE);
    load_profile_fields();
}

void profile_store::merge_fields(
    best_scores_store::best_fields_names_t const& best_results, awards_store::award_fields_names_t const& awards_fields)
{
    unsigned int i = 0;
    for (unsigned int bf = 0; bf < best_scores_store::fields_count; ++bf)
    {
        m_field_names_store[i] = best_results[bf];
        ++i;
    }
    for (unsigned int af = 0; af < awards_store::fields_count; ++af)
    {
        m_field_names_store[i] = awards_fields[af];
        ++i;
    }
    VERIFY(i == merged_fields_count);
    m_get_records_input.mNumFields = i;
    m_get_records_input.mFieldNames = m_field_names_store;
    m_get_records_input.mTableId = (char*)profile_table_name;
}

void profile_store::load_profile_fields()
{
    SAKERequest reqres = m_sake_obj->GetMyRecords(&m_get_records_input, &profile_store::get_my_fields_cb, this);

    if (!reqres)
    {
        SAKEStartRequestResult tmp_result = m_sake_obj->GetRequestResult();
        loaded_fields(false, CGameSpy_SAKE::TryToTranslate(tmp_result).c_str());
    }
}

void __cdecl profile_store::get_my_fields_cb(
    SAKE sake, SAKERequest request, SAKERequestResult result, void* inputData, void* outputData, void* userData)
{
    profile_store* my_inst = static_cast<profile_store*>(userData);
    if (result != SAKERequestResult_SUCCESS)
    {
        my_inst->loaded_fields(false, CGameSpy_SAKE::TryToTranslate(result).c_str());
        return;
    }
    SAKEGetMyRecordsOutput* tmp_out = static_cast<SAKEGetMyRecordsOutput*>(outputData);
    VERIFY(tmp_out);
    my_inst->m_awards_store->process_aw_out_response(tmp_out, merged_fields_count);
    my_inst->m_best_scores_store->process_scores_out_response(tmp_out, merged_fields_count);
    my_inst->loaded_fields(true, "");
}

void profile_store::loaded_fields(bool const result, char const* err_descr)
{
    if (!m_complete_cb)
    {
        Msg("WARNING: loading awards terminated by user");
        VERIFY(!m_progress_indicator);
        Engine.Sheduler.Unregister(this);
        return;
    }

    store_operation_cb tmp_cb = m_complete_cb;
    m_complete_cb.clear();
    m_progress_indicator.clear();
    Engine.Sheduler.Unregister(this);

    if (!result)
    {
        tmp_cb(false, err_descr);
        return;
    }

    if (m_valid_ltx)
    {
        m_awards_store->load_awards_from_ltx(m_dsigned_reader.get_ltx());
        m_best_scores_store->load_best_scores_from_ltx(m_dsigned_reader.get_ltx());
        m_awards_store->merge_sake_to_ltx_awards();
        m_best_scores_store->merge_sake_to_ltx_best_scores();
        check_sake_actuality();
    }
    tmp_cb(true, "");
}

void __stdcall profile_store::onlylog_operation(bool const result, char const* descr)
{
    if (!result)
    {
        Msg("! Profile store ERROR: %s", descr ? descr : "unknown");
        return;
    }
    Msg("* Profile store: %s", descr ? descr : "");
}

void __stdcall profile_store::onlylog_completion(bool const result, char const* err_descr)
{
    if (!result)
    {
        Msg("! Profile loading ERROR: %s", err_descr ? err_descr : "unknown");
        return;
    }
    Msg("* Profile loading successfully complete");
}

#ifdef DEBUG
static u32 const actuality_update_time = 120;
#else
static u32 const actuality_update_time = 3600;
#endif //#ifdef DEBUG

void profile_store::check_sake_actuality()
{
    if (!m_awards_store->is_sake_equal_to_file() || !m_best_scores_store->is_sake_equal_to_file())
    {
#ifndef LINUX // FIXME!!!
        __time32_t current_time;
        _time32(&current_time);

        __time32_t last_submit_time =
            static_cast<__time32_t>(m_dsigned_reader.get_ltx().r_u32(profile_data_section, profile_last_submit_time));
        if ((current_time - last_submit_time) >= actuality_update_time)
        {
            atlas_submit_queue* tmp_submit_queue = MainMenu()->GetSubmitQueue();
            VERIFY(tmp_submit_queue);
            tmp_submit_queue->submit_all();
        }
#endif
    }
}

} // namespace gamespy_profile
