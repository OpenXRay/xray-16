#ifndef BEST_SCORES_SYSTEM_INCLUDED
#define BEST_SCORES_SYSTEM_INCLUDED

#include <boost/noncopyable.hpp>
#include "../xrServerEntities/associative_vector.h"
#include "../xrGameSpy/GameSpy/sake/sake.h"
#include "profile_data_types.h"

class CGameSpy_Full;
class CGameSpy_SAKE;

namespace gamespy_profile
{

typedef fastdelegate::FastDelegate<void (bool, shared_str const &)>	bestscore_operation_cb;

class best_scores_store
{
public:
								best_scores_store		(CGameSpy_Full* fullgs);
								~best_scores_store		();

	void						load_best_scores			(store_operation_cb & opcb);
	void						load_best_scores_from_ltx	(CInifile& ini);
	bool						is_sake_equal_to_file		() const;
	void						reset_scores				();

	all_best_scores_t&			get_player_best_scores			();
	void						merge_sake_to_ltx_best_scores	();
	
	static int const			fields_count = bst_score_types_count;
	typedef char* best_fields_names_t[fields_count];

	best_fields_names_t const &	get_field_names				() const { return m_field_names_store; }
	void						process_scores_out_response	(SAKEGetMyRecordsOutput* tmp_out, int const out_fields_count);
private:
	all_best_scores_t	m_result_scores;
	all_best_scores_t	m_ltx_result_scores;
	store_operation_cb	m_scores_operation_cb;

	CGameSpy_SAKE*		m_sake_obj;
	CGameSpy_Full*		m_fullgs_obj;

	best_fields_names_t		m_field_names_store;
	SAKEGetMyRecordsInput	m_get_records_input;
	void					init_field_names();

	static void __cdecl get_my_player_scores_cb			(SAKE sake,
														 SAKERequest request,
														 SAKERequestResult result,
														 void * inputData,
														 void * outputData,
														 void * userData);
}; //best_scores_store

} //namespace gamespy_profile

#endif //#ifndef BEST_SCORES_SYSTEM_INCLUDED