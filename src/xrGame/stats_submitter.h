#ifndef STATS_SUBMITTER_INCLUDED
#define STATS_SUBMITTER_INCLUDED

#include <boost/noncopyable.hpp>
#include "gamespy/GameSpy_ATLAS.h"
#include "profile_data_types.h"
#include "gsc_dsigned_ltx.h"
#include "../3rd party/crypto/crypto.h"

class CGameSpy_Full;


namespace gamespy_gp
{
	struct profile;
};

namespace gamespy_profile
{

class stats_submitter :
	public ISheduled,
	private boost::noncopyable
{
public:
							stats_submitter			(CGameSpy_Full* fullgs);
							~stats_submitter		();

			void			reward_with_award		(enum_awards_t award_id,
													 u32 const count,
													 gamespy_gp::profile const * profile,
													 store_operation_cb opcb);
			void			quick_reward_with_award	(enum_awards_t const award_id, gamespy_gp::profile const * profile);

			void			set_best_scores			(all_best_scores_t const * scores, gamespy_gp::profile const * profile, store_operation_cb opcb);
			void			quick_set_best_scores	(all_best_scores_t const * scores, gamespy_gp::profile const * profile);

			void			submit_all				(all_awards_t const * awards,
													 all_best_scores_t const * scores,
													 gamespy_gp::profile const * profile,
													 store_operation_cb opcb);

	virtual void			shedule_Update			(u32 dt);
	virtual	shared_str		shedule_Name			() const	{ return shared_str("gamespy_atlas_updator"); };
	virtual bool			shedule_Needed			()			{ return true; };
	virtual float			shedule_Scale			()			{ return 1.0f; };
	
	static u8 const p_number[crypto::xr_dsa::public_key_length];
	static u8 const q_number[crypto::xr_dsa::private_key_length];
	static u8 const g_number[crypto::xr_dsa::public_key_length];
	static u8 const public_key[crypto::xr_dsa::public_key_length];

private:
	CGameSpy_Full*			m_fullgs_obj;
	CGameSpy_ATLAS*			m_atlas_obj;
	gsc_dsigned_ltx_writer	m_ltx_file;
	
	gamespy_gp::profile const *	m_last_operation_profile;
	gsi_u8						m_atlas_connection_id[SC_CONNECTION_GUID_SIZE];
	SCReportPtr					m_atlas_report;

	enum enum_report_type
	{
		ert_set_award			=	0x00,
		ert_set_best_scores,
		ert_synchronize_profile
	};//enum enum_report_type

	enum_report_type			m_report_type;
	enum_awards_t				m_last_award_id;
	u32							m_last_award_count;
	all_awards_t const *		m_last_all_awards;
	all_best_scores_t const *	m_last_best_scores;
	
	store_operation_cb			m_last_operation_cb;

			void			begin_session				();
			bool			prepare_report				();
			
			bool			add_player_name_to_report	();
			bool			create_award_inc_report		();
			bool			create_best_scores_report	();
			bool			create_all_awards_report	();

			void			terminate_session			();

			void			save_file					(gamespy_gp::profile const * profile);

	void __stdcall			onlylog_operation		(bool const result, char const * err_descr);

	static void				fill_private_key(crypto::xr_dsa::private_key_t & priv_key_dest);

	//------- callbacks --------
	static u32 const		operation_timeout_value;
	static void __cdecl		created_session_cb		(const SCInterfacePtr theInterface,
													 GHTTPResult          theHttpResult,
													 SCResult             theResult,
													 void *               theUserData);
	static void __cdecl		set_intension_cb		(const SCInterfacePtr theInterface,
													 GHTTPResult          theHttpResult,
													 SCResult             theResult,
													 void *               theUserData);
	static void __cdecl		submitted_cb			(const SCInterfacePtr theInterface,
													 GHTTPResult          theHttpResult,
													 SCResult             theResult,
													 void *               theUserData);

};

} //namespace gamespy_profile

#endif //#ifndef STATS_SUBMITTER_INCLUDED