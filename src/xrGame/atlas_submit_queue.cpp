#include "StdAfx.h"
#include "atlas_submit_queue.h"
#include "stats_submitter.h"
#include "login_manager.h"
#include "profile_store.h"
#include "MainMenu.h"

atlas_submit_queue::atlas_submit_queue(gamespy_profile::stats_submitter* stats_submitter)
    : m_stats_submitter(stats_submitter), m_atlas_in_process(false)
{
    VERIFY(m_stats_submitter);
    m_atlas_submitted.bind(this, &atlas_submit_queue::atlas_submitted);
}

atlas_submit_queue::~atlas_submit_queue() {}
void atlas_submit_queue::submit_all()
{
#ifdef WINDOWS
    gamespy_gp::login_manager* tmp_lmngr = MainMenu()->GetLoginMngr();
    VERIFY(tmp_lmngr);
    gamespy_gp::profile const* tmp_curr_prof = tmp_lmngr->get_current_profile();
    if (!tmp_curr_prof || !tmp_curr_prof->online())
    {
        Msg("! ATLAS submit can be only in online profile mode");
        return;
    }
    submit_task tmp_task;
    tmp_task.m_data_type = submit_task::edt_submit_all;
    m_reward_tasks.push_back(tmp_task);
    update();
#endif
}

void atlas_submit_queue::submit_reward(gamespy_profile::enum_awards_t const award_id)
{
#ifdef WINDOWS
    using namespace gamespy_profile;
    gamespy_gp::login_manager* tmp_lmngr = MainMenu()->GetLoginMngr();
    VERIFY(tmp_lmngr);
    gamespy_gp::profile const* tmp_curr_prof = tmp_lmngr->get_current_profile();
    if (!tmp_curr_prof->online())
    {
        Msg("! ATLAS submit can be only in online profile mode");
        return;
    }
    m_stats_submitter->quick_reward_with_award(award_id, tmp_curr_prof);

    profile_store* tmp_store = MainMenu()->GetProfileStore();
    R_ASSERT(tmp_store);
    all_awards_t const& tmp_all_awards = tmp_store->get_awards();
    all_awards_t::const_iterator tmp_iter = tmp_all_awards.find(award_id);
    R_ASSERT(tmp_iter != tmp_all_awards.end());

    submit_task tmp_task;
    tmp_task.m_data_type = submit_task::edt_award_id;
    tmp_task.m_award_id = award_id;
    tmp_task.m_awards_count = tmp_iter->second.m_count; //+1 was on quick_reward_with_award
    m_reward_tasks.push_back(tmp_task);

    update();
#endif
}

void atlas_submit_queue::submit_best_results()
{
#ifdef WINDOWS
    gamespy_gp::login_manager* tmp_lmngr = MainMenu()->GetLoginMngr();
    VERIFY(tmp_lmngr);
    gamespy_gp::profile const* tmp_curr_prof = tmp_lmngr->get_current_profile();
    if (!tmp_curr_prof->online())
    {
        Msg("! ATLAS submit can be only in online profile mode");
        return;
    }

    submit_task tmp_task;
    tmp_task.m_data_type = submit_task::edt_best_scores_ptr;
    m_reward_tasks.push_back(tmp_task);
    m_stats_submitter->quick_set_best_scores(&m_best_results_to_submit, tmp_curr_prof);
    update();
#endif
}

void atlas_submit_queue::update()
{
#ifdef WINDOWS
    if (m_reward_tasks.empty() || is_active())
        return;

    gamespy_gp::login_manager* tmp_lmngr = MainMenu()->GetLoginMngr();
    VERIFY(tmp_lmngr);
    gamespy_gp::profile const* tmp_curr_prof = tmp_lmngr->get_current_profile();
    VERIFY(tmp_curr_prof);
    VERIFY2(tmp_curr_prof->online(), "ATLAS submit can be only in online profile mode");

    if (m_reward_tasks.front().m_data_type == submit_task::edt_award_id)
    {
        do_atlas_reward(tmp_curr_prof, m_reward_tasks.front().m_award_id, m_reward_tasks.front().m_awards_count);
    }
    else if (m_reward_tasks.front().m_data_type == submit_task::edt_best_scores_ptr)
    {
        do_atlas_best_results(tmp_curr_prof, &m_best_results_to_submit);
    }
    else if (m_reward_tasks.front().m_data_type == submit_task::edt_submit_all)
    {
        do_atlas_submit_all(tmp_curr_prof);
    }
    else
    {
        NODEFAULT;
    }
    m_reward_tasks.pop_front();
#endif
}

void atlas_submit_queue::do_atlas_reward(
    gamespy_gp::profile const* profile, gamespy_profile::enum_awards_t const award_id, u32 const count)
{
    VERIFY(m_stats_submitter);
    VERIFY(!m_atlas_in_process);

    m_atlas_in_process = true;

    m_stats_submitter->reward_with_award(award_id, count, profile, m_atlas_submitted);
}

void atlas_submit_queue::do_atlas_best_results(
    gamespy_gp::profile const* profile, gamespy_profile::all_best_scores_t* br_ptr)
{
    VERIFY(m_stats_submitter);
    VERIFY(!m_atlas_in_process);

    m_atlas_in_process = true;

    m_stats_submitter->set_best_scores(br_ptr, profile, m_atlas_submitted);
}

void atlas_submit_queue::do_atlas_submit_all(gamespy_gp::profile const* profile)
{
#ifdef WINDOWS
    VERIFY(m_stats_submitter);
    VERIFY(!m_atlas_in_process);

    gamespy_profile::profile_store* tmp_store = MainMenu()->GetProfileStore();
    VERIFY(tmp_store);

    m_atlas_in_process = true;
    m_stats_submitter->submit_all(&tmp_store->get_awards(), &tmp_store->get_best_scores(), profile, m_atlas_submitted);
#endif
}

void __stdcall atlas_submit_queue::atlas_submitted(bool result, char const* err_string)
{
    if (result)
    {
        Msg("* submit complete successfully !");
    }
    else
    {
        Msg("! failed to submit atlas report: %s", err_string);
    }
    m_atlas_in_process = false;
}
