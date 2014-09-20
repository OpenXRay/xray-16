#ifndef ATLAS_SUBMIT_QUEUE_INCLUDED
#define ATLAS_SUBMIT_QUEUE_INCLUDED

#include <boost/noncopyable.hpp>
#include "../xrCore/fastdelegate.h"
#include "profile_data_types.h"

namespace gamespy_profile
{
	class stats_submitter;
};
namespace gamespy_gp
{
	struct profile;
};

class atlas_submit_queue : private boost::noncopyable
{
public:
			atlas_submit_queue	(gamespy_profile::stats_submitter* stats_submitter);
			~atlas_submit_queue	();
	
	//awards and best scores will be fetched from profile_store object
	void	submit_all			();	
	void	submit_reward		(gamespy_profile::enum_awards_t const award_id);
	void	submit_best_results	();
	bool	is_active			() const { return m_atlas_in_process; };
	void	update				();

	gamespy_profile::all_best_scores_t&	get_best_results_store() { return m_best_results_to_submit; };
private:
	struct submit_task
	{
		enum enum_data_type
		{
			edt_award_id	= 0x00,
			edt_best_scores_ptr,
			edt_submit_all,
		} m_data_type;
		gamespy_profile::enum_awards_t		m_award_id;
		u32									m_awards_count;
		//gamespy_profile::all_best_scores_t*	m_best_scores_ptr;
	};//struct submit_task
	typedef xr_deque<submit_task>			reward_tasks_t;
	reward_tasks_t							m_reward_tasks;
	gamespy_profile::all_best_scores_t		m_best_results_to_submit;

	void				do_task					(submit_task const & td);
	void				do_atlas_reward			(gamespy_gp::profile const * profile,
												 gamespy_profile::enum_awards_t const award_id,
												 u32 const count);
	void				do_atlas_best_results	(gamespy_gp::profile const * profile,
												 gamespy_profile::all_best_scores_t* br_ptr);
	void				do_atlas_submit_all		(gamespy_gp::profile const * profile);

	gamespy_profile::stats_submitter*		m_stats_submitter;
	gamespy_profile::store_operation_cb		m_atlas_submitted;
	bool									m_atlas_in_process;

	void	__stdcall	atlas_submitted		(bool result, char const * err_string);
};//class atlas_submit_quque

#endif //#ifndef ATLAS_SUBMIT_QUEUE_INCLUDED