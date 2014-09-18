#include <stdexcept>
#include <iostream>
#include <algorithm>
#include "gamespy_sake.h"

extern char const *	game_name;
extern int			game_id;
extern int			game_product_id;
extern int			game_namespace_id;

char const * gp_nick	= "profile_printer@ua";
char const * gp_email	= "profile_printer@ua";
char const * gp_pass	= "loopzilla";

typedef char secret_key_t[32];
void	fill_secret_key(secret_key_t & sc)
{
	sc[0] = 'L';
	sc[1] = 'T';
	sc[2] = 'U';
	sc[3] = '2';
	sc[4] = 'z';
	sc[5] = '2';
	sc[6] = '\0';
}

#ifdef GSI_COMMON_DEBUG
static void debug_callback(GSIDebugCategory theCat, GSIDebugType theType,
                          GSIDebugLevel theLevel, const char * theTokenStr,
                          va_list theParamList)
{
	char tmp_prefix_buffer[256];
	char error_buffer[1024];

	//hope compiler will find specializations like: template <size_t size> int sprintf( char (&buffer)[size],
	
	sprintf(tmp_prefix_buffer, "GameSpy: [%s][%s] ",
		gGSIDebugCatStrings[theCat], 
		gGSIDebugTypeStrings[theType]);

	vsprintf(error_buffer, theTokenStr, theParamList); 
	std::cerr << tmp_prefix_buffer << error_buffer << std::endl;
}
#endif

GSIACResult check_gamespy_services(void)
{
	GSIACResult aResult;
	GSIStartAvailableCheck(game_name);
	// Continue processing while the check is in progress
	do
	{
		aResult = GSIAvailableCheckThink();
		msleep(10);
	} while(aResult == GSIACWaiting);

	// Check the result
	switch(aResult)
	{
	case GSIACAvailable:
		break;
	case GSIACUnavailable:
		std::cerr << "GameSpy: Online services are unavailable\r\n"
			<< "GameSpy: Please visit www.mygame.com for more information.\r\n";
		break;
	case GSIACTemporarilyUnavailable:
		std::cerr << "GameSpy: Online services are temporarily unavailable.\r\n"
			<< "GameSpy: Please visit www.mygame.com for more information.\r\n";
		break;
	default:
		break;
	};
	return aResult;
}

sake_processor::core_initializer::core_initializer()
{
	if (check_gamespy_services() != GSIACAvailable)
		throw std::runtime_error("GameSpy services are unavailable");

#ifdef GSI_COMMON_DEBUG
	gsSetDebugCallback(debug_callback);
	gsSetDebugLevel(GSIDebugCat_All, GSIDebugType_All, GSIDebugLevel_Hardcore);
#endif
	gsCoreInitialize();
}
sake_processor::core_initializer::~core_initializer()
{
	gsCoreShutdown();
}

sake_processor::gp_processor::gp_processor() : 
	m_connection(NULL)
{
	memset(&m_login_ticket, 0, sizeof(m_login_ticket));
	GPResult tmp_res = gpInitialize(&m_connection,
		game_product_id,
		game_namespace_id,
		GP_PARTNERID_GAMESPY);
	m_complete = false;
	if (tmp_res != GP_NO_ERROR)
	{
		std::cerr << "GameSpy: GP initialization failed, errcode: "
			<< tmp_res << std::endl;
		throw std::runtime_error("GameSpy GP initialization failed");
	}
}

bool sake_processor::gp_processor::login()
{
	assert(m_connection);
	GPResult tmp_res = gpConnect(&m_connection, gp_nick, gp_email, gp_pass,
		GP_NO_FIREWALL, GP_BLOCKING, &gp_processor::login_result_cb, this);

	if (tmp_res != GP_NO_ERROR)
		return false;
	
	return true;
}

void sake_processor::gp_processor::login_result_cb(GPConnection * connection,
														   void * arg,
														   void * param)
{
	gp_processor* me				= static_cast<gp_processor*>(param);
	GPConnectResponseArg* tmp_rarg	= static_cast<GPConnectResponseArg*>(arg);
	me->m_complete					= true;

	assert(tmp_rarg);
	if (tmp_rarg->result != GP_NO_ERROR)
	{
		std::cerr << "GameSpy: GP failed to connect, errcode: "
			<< tmp_rarg->result << std::endl;
		return;
	}
	me->m_profile_id	= tmp_rarg->profile;
	gpGetLoginTicket	(&me->m_connection, me->m_login_ticket);
}

sake_processor::gp_processor::~gp_processor()
{
	gpDestroy(&m_connection);
}


sake_processor::sake_processor()
{
	static bool already_initialized = false;
	assert(!already_initialized &&
		"sake_processor instance must be single (please implement singleton)");
	// setup the common debugging
	already_initialized = true;

	if (!m_gp_processor.login())
	{
		std::cerr << "GameSpy: GP failed to login" << std::endl;
		throw std::runtime_error(std::string("failed to login with GameSpy GP"));
	}

	if (sakeStartup(&m_sake_inst) != SAKEStartupResult_SUCCESS)
	{
		std::cerr << "GameSpy: failed to initialize SAKE" << std::endl;
		throw std::runtime_error(std::string("failed to initialize GameSpy SAKE")); 
	}
	secret_key_t tmp_sc;
	memset			(tmp_sc, 0, sizeof(secret_key_t));
	fill_secret_key	(tmp_sc);
	
	sakeSetGame		(m_sake_inst, game_name, game_id, tmp_sc);
	sakeSetProfile	(m_sake_inst, m_gp_processor.m_profile_id, m_gp_processor.m_login_ticket);

	init_request_fields();
	m_processing_request = false;
}

void sake_processor::think(gsi_time ms)
{
	gsCoreThink(ms);
}

sake_processor::~sake_processor()
{
	sakeShutdown	(m_sake_inst);
}

void sake_processor::begin_fetch()
{
	m_request_names.clear	();
	m_result_players.clear	();
	m_current_offset		= 0;
}
void sake_processor::add_name(char const * name)
{
	for (player_names_t::const_iterator i = m_request_names.begin(),
		ie = m_request_names.end(); i != ie; ++i)
	{
		if (!strcmp(i->c_str(), name))
			return;
	}
	m_request_names.resize(m_request_names.size() + 1);
	m_request_names.back() = name;
}

bool sake_processor::get_profile(char const * name, gamespy_profile::profile_data & dest_data)
{
	assert(!m_processing_request);
	m_tmp_string = name;
	players_map_t::iterator tmp_iter = m_result_players.find(m_tmp_string);
	if (tmp_iter != m_result_players.end())
	{
		dest_data = tmp_iter->second;
		return true;
	}
	return false;
}

bool security_treat(std::string const & name)
{
	return (std::find(name.begin(), name.end(), '\'') != name.end());
}

bool sake_processor::create_request_string()
{
	m_tmp_string = "";
	m_request_names.erase(
		std::remove_if(
			m_request_names.begin(),
			m_request_names.end(),
			security_treat),
		m_request_names.end());
	
	player_names_t::const_iterator i	= m_request_names.begin();
	player_names_t::const_iterator ie	= m_request_names.end();
	if (i == ie)
		return false;
	do
	{
		m_tmp_string.append("(STAT_PlayerName='");
		m_tmp_string.append(*i);
		++i;
		if (i == ie)
		{
			m_tmp_string.append("')");
			break;
		} else
		{
			m_tmp_string.append("')or");
		}
	} while (1);
	return true;
}

void sake_processor::fetch()
{
	memset(&m_get_records_input, 0, sizeof(m_get_records_input));
	m_get_records_input.mTableId	= gamespy_profile::profile_table_name;
	m_get_records_input.mFieldNames	= m_field_names_store;
	m_get_records_input.mNumFields	= merged_fields_count;
	m_get_records_input.mCacheFlag	= gsi_true;
	if (!create_request_string())
		return;
	
	m_get_records_input.mFilter		= &*m_tmp_string.begin();
	m_get_records_input.mOffset		= m_current_offset;
	m_get_records_input.mMaxRecords	= max_request_records;
	
	SAKERequest	tmp_request = sakeSearchForRecords(m_sake_inst,
		&m_get_records_input,
		&sake_processor::request_callback,
		this);
	m_processing_request = true;
	
	if (!tmp_request)
	{
		SAKEStartRequestResult tmp_result = sakeGetStartRequestResult(m_sake_inst);
		std::cerr << "GameSpy: SAKE request failed, errcode: " << tmp_result << std::endl;
		request_callback(m_sake_inst, tmp_request, SAKERequestResult_UNKNOWN_ERROR, NULL, NULL, this); 
	}
}

bool sake_processor::is_result_ready() const
{
	return (m_processing_request == false);
}

void sake_processor::init_request_fields()
{
	using namespace gamespy_profile;
	int ci = 0;
	for (int i = 0; i < bst_score_types_count; ++i)
	{
		m_field_names_store[ci]	= ATLAS_GET_STAT_NAME(
			get_best_score_id_stat(static_cast<enum_best_score_type>(i))
		);
		++ci;
	}
	for (int i = 0; i < at_awards_count; ++i)
	{
		int findex = i * ap_award_params_count;
		m_field_names_store[ci + ap_award_id]		= ATLAS_GET_STAT_NAME(
			get_award_id_stat(static_cast<enum_awards_t>(i))
		);
		m_field_names_store[ci + ap_award_rdate]	= ATLAS_GET_STAT_NAME(
			get_award_reward_date_stat(static_cast<enum_awards_t>(i))
		);
		ci += ap_award_params_count;
	}
	m_field_names_store[ci] = ATLAS_GET_STAT_NAME(STAT_PlayerName);
}

bool sake_processor::process_record(SAKESearchForRecordsInput* in,
									SAKEField* fields,
									gamespy_profile::profile_data & dest_data,
									std::string & dest_name)
{
	using namespace gamespy_profile;
	int last_fi = 0;
	dest_name.clear();
	for (int i = 0; i < in->mNumFields; ++i, ++last_fi)
	{
		if (strcmp(fields[last_fi].mName, in->mFieldNames[i]))
		{
			bool found = false;
			last_fi = 0;
			do
			{
				if (!strcmp(fields[last_fi].mName, in->mFieldNames[i]))
				{
					found = true;
					break;
				}
				++last_fi;
			} while (last_fi < in->mNumFields);
			if (!found)
			{
				last_fi = 0;
				continue;
			}
		}
		enum_awards_t tmp_award_id = get_award_by_stat_id_name(fields[last_fi].mName);
		if (tmp_award_id < at_awards_count)
		{
			dest_data.m_awards[tmp_award_id].m_count = fields[last_fi].mValue.mShort;
			continue;
		}
		tmp_award_id = get_award_by_stat_rdate_name(fields[last_fi].mName);
		if (tmp_award_id < at_awards_count)
		{
			dest_data.m_awards[tmp_award_id].m_last_reward_date = fields[last_fi].mValue.mInt;
			continue;
		}
		enum_best_score_type tmp_best_score_id = get_best_score_type_by_sname(fields[last_fi].mName);
		if (tmp_best_score_id < bst_score_types_count)
		{
			dest_data.m_best_scores[tmp_best_score_id] = fields[last_fi].mValue.mInt;
			continue;
		}
		if (strcmp(fields[last_fi].mName, ATLAS_GET_KEY_NAME(STAT_PlayerName)))
		{
			if (strlen(fields[last_fi].mValue.mAsciiString))
			{
				dest_name = fields[last_fi].mValue.mAsciiString;
			}
		}
	}
	return !dest_name.empty();
}

void sake_processor::process_out_request(SAKESearchForRecordsInput* in,
										 SAKESearchForRecordsOutput* out)
{
	using namespace gamespy_profile;
	for (int ri = 0; ri < out->mNumRecords; ++ri)
	{
		profile_data tmp_data;
		if (process_record(in, out->mRecords[ri], tmp_data, m_tmp_string))
		{
			m_result_players.insert(std::make_pair(
				m_tmp_string, tmp_data));
			m_tmp_string.clear();
		}
	}
}


void sake_processor::request_callback(SAKE sake,
											  SAKERequest request,
											  SAKERequestResult result,
											  void * inputData,
											  void * outputData,
											  void * userData)
{
	sake_processor* me			= static_cast<sake_processor*>(userData);
	SAKESearchForRecordsInput*	tmp_in = 
		static_cast<SAKESearchForRecordsInput*>(inputData);
	SAKESearchForRecordsOutput*	tmp_out = 
		static_cast<SAKESearchForRecordsOutput*>(outputData);
	if (tmp_out)
	{
		me->process_out_request(tmp_in, tmp_out);
		me->m_current_offset += tmp_out->mNumRecords;
		if ((tmp_out->mNumRecords == max_request_records) &&
			(static_cast<std::size_t>(me->m_current_offset) < me->m_request_names.size()))
		{
			me->fetch();
			return;
		}
	}
	me->m_processing_request = false;
}
