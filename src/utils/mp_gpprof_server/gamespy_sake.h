#ifndef GAMESPY_SAKE_INCLUDED
#define GAMESPY_SAKE_INCLUDED

#include <vector>
#include <list>
#include <map>

#include <common/gsCommon.h>
#include <common/gsAvailable.h>
#include <common/gsCore.h>
#include <common/gsSoap.h>
#include <sake/sake.h>
#include <ghttp/ghttp.h>
#include <GP/gp.h>

#include "profile_data_types.h"

#undef max


//must be singleton
class sake_processor 
{
public:
				sake_processor		();
				~sake_processor		();
	void		think				(gsi_time ms);
	
	void		begin_fetch			();
	void		add_name			(char const * name);
	void		fetch				();

	bool		is_result_ready		() const;
	bool		get_profile			(char const * name, gamespy_profile::profile_data & dest_data);
private:
	struct core_initializer
	{
		core_initializer	();
		~core_initializer	();
	};
	
	class gp_processor
	{
	public:
		gp_processor		();
		bool login			();
		~gp_processor		();
		
		char				m_login_ticket[GP_LOGIN_TICKET_LEN];
		GPProfile			m_profile_id;
	private:
		GPConnection		m_connection;
		bool				m_complete;
		static void 		login_result_cb	(GPConnection * connection,
											 void * arg, 
											 void * param); 
	};

	core_initializer	m_core_initilizer;
	gp_processor		m_gp_processor;
	SAKE				m_sake_inst;

	static unsigned int const merged_fields_count = 
		(gamespy_profile::at_awards_count * gamespy_profile::ap_award_params_count) +
		gamespy_profile::bst_score_types_count + 1; //+1 = STAT_PlayerName
	static int const max_request_records = 128;
	typedef	char* merged_fields_names_t[merged_fields_count];
	typedef std::map<std::string, gamespy_profile::profile_data> players_map_t;
	typedef std::list<std::string> player_names_t;
	
	merged_fields_names_t		m_field_names_store;
	SAKESearchForRecordsInput 	m_get_records_input;
	bool						m_processing_request;
	int							m_current_offset;
	player_names_t				m_request_names;
	std::string					m_tmp_string;
	players_map_t				m_result_players;
	

	void				init_request_fields		();
	bool				create_request_string	();
	void				process_out_request	(SAKESearchForRecordsInput* in,
											 SAKESearchForRecordsOutput* out);
	bool				process_record		(SAKESearchForRecordsInput* in,
											 SAKEField* fields,
											 gamespy_profile::profile_data & dest_data,
											 std::string & dest_name);
	static void 			request_callback	(SAKE sake,
											 SAKERequest request,
											 SAKERequestResult result,
											 void * inputData,
											 void * outputData,
											 void * userData);
};//class sake_processor

#endif//#ifndef GAMESPY_SAKE_INCLUDED

