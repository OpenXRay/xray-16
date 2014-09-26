#ifndef GAMESPY_PROFILE_STORE
#define GAMESPY_PROFILE_STORE

#include <boost/noncopyable.hpp>
#include "../xrEngine/ISheduled.h"
#include "profile_data_types.h"
#include "script_export_space.h"
#include "queued_async_method.h"
#include "gsc_dsigned_ltx.h"
#include "best_scores_store.h"
#include "awards_store.h"


class CGameSpy_Full;
class CGameSpy_SAKE;
//class CGameSpy_ATLAS;

namespace gamespy_profile
{

class awards_store;
class best_scores_store;

class profile_store : 
	public ISheduled,
	private boost::noncopyable
{
public:
	explicit		profile_store			(CGameSpy_Full* fullgs_obj);
					~profile_store			();
	
	void			set_current_profile			(int profileId, char const * loginTicket);
	void			load_current_profile		(store_operation_cb progress_indicator_cb,
												 store_operation_cb complete_cb);
	void			stop_loading				();

	virtual void			shedule_Update	(u32 dt);
	virtual	shared_str		shedule_Name	() const	{ return shared_str("gamespy_sake_updator"); };
	virtual bool			shedule_Needed	()			{ return true; };
	virtual float			shedule_Scale	()			{ return 1.0f; };
	
	all_awards_t const &		get_awards		();
	all_best_scores_t const &	get_best_scores	();
	
	awards_store*				get_awards_store		() { return m_awards_store; };
	best_scores_store*			get_best_scores_store	() { return m_best_scores_store; };
private:
	typedef			parameters_tuple1<store_operation_cb>	load_prof_params_t;
	void			load_current_profile_raw	(load_prof_params_t const & args,
												 store_operation_cb complete_cb);
	void			release_current_profile		(bool, char const *);
	void			check_sake_actuality		();
	queued_async_method<
		profile_store,
		load_prof_params_t,
		store_operation_cb,
		&profile_store::load_current_profile_raw,
		&profile_store::release_current_profile>	m_load_current_profile_qam;



	CGameSpy_SAKE*		m_sake_obj;
	//CGameSpy_ATLAS*		m_atlas_obj;
	CGameSpy_Full*		m_fullgs_obj;

	store_operation_cb	m_progress_indicator;
	store_operation_cb	m_complete_cb;

	gsc_dsigned_ltx_reader	m_dsigned_reader;
	bool					m_valid_ltx;
	awards_store*			m_awards_store;
	best_scores_store*		m_best_scores_store;


	static unsigned int const merged_fields_count = best_scores_store::fields_count + awards_store::fields_count;
	typedef	char*		merged_fields_names_t[merged_fields_count];

	void				load_profile				(store_operation_cb progress_indicator_cb);
	void				merge_fields				(best_scores_store::best_fields_names_t const & best_results,
													 awards_store::award_fields_names_t const & awards_fields);
	merged_fields_names_t	m_field_names_store;
	SAKEGetMyRecordsInput	m_get_records_input;
	
	void				load_profile_fields			();
	


	void				loaded_fields				(bool const result, char const * err_descr);
	void __stdcall		loaded_best_scores			(bool const result, char const * err_descr);

	void __stdcall		onlylog_operation			(bool const result, char const * err_descr);
	void __stdcall		onlylog_completion			(bool const result, char const * err_descr);

	static void __cdecl	get_my_fields_cb			(SAKE sake,
													 SAKERequest request,
													 SAKERequestResult result,
													 void * inputData,
													 void * outputData,
													 void * userData);

	DECLARE_SCRIPT_REGISTER_FUNCTION
};//class profile_store

} //namespace gamespy_profile

typedef gamespy_profile::profile_store	gamespy_profile_profile_store;

add_to_type_list(gamespy_profile_profile_store)
#undef script_type_list
#define script_type_list save_type_list(gamespy_profile_profile_store)

#endif //#ifndef GAMESPY_PROFILE_STORE