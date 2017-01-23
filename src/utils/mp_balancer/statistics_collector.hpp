#ifndef STATISTICS_COLLECTOR
#define STATISTICS_COLLECTOR

#include "pch.h"

class weapon_collection;

class statistics_collector
{
private:
	weapon_collection* m_wpn_collection;
	typedef xr_vector<shared_str>	params_collection;
	typedef associative_vector<shared_str, params_collection*>	csv_files;
	csv_files	m_all_params;
	void save_file(csv_files::value_type const & val);
	csv_files::const_iterator get_most_acceptable_group(shared_str const & section);
public:
	explicit statistics_collector(weapon_collection* wpn_collection);
	~statistics_collector();
	void save_files();
	void load_settings();
};

#endif //#ifndef STATISTICS_COLLECTOR
