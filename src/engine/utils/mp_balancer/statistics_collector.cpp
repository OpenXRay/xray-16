#include "statistics_collector.hpp"
#include "wpn_collection.hpp"

statistics_collector::statistics_collector(weapon_collection* wpn_collection)
{
	m_wpn_collection = wpn_collection;
}

statistics_collector::~statistics_collector()
{
	delete_data(m_all_params);
}
#define CSV_SETTINGS "csv_settings"
void statistics_collector::load_settings()
{
	u32 file_count = m_wpn_collection->settings->line_count(CSV_SETTINGS);
	for (u32 i = 0; i != file_count; ++i)
	{
		LPCSTR key = NULL;
		LPCSTR value = NULL;
		if (m_wpn_collection->settings->r_line(CSV_SETTINGS, i, &key, &value) && key)
		{
			csv_files::iterator new_file_iter = m_all_params.insert(
				std::make_pair(shared_str(key), xr_new<params_collection>())).first;
			if (value)
			{
				get_string_collection(value, *new_file_iter->second);
			}
		}
	}
}

void statistics_collector::save_files()
{
	std::for_each(m_all_params.begin(), m_all_params.end(), std::bind1st(
		std::mem_fun<void, statistics_collector>(&statistics_collector::save_file), this)
	);
}

statistics_collector::csv_files::const_iterator 
	statistics_collector::get_most_acceptable_group(shared_str const & section)
{
	u32 max_size = 0;
	csv_files::const_iterator ret_iter = m_all_params.end();
	for (csv_files::const_iterator i = m_all_params.begin(),
		ie = m_all_params.end(); i != ie; ++i)
	{
		if (strncmp(i->first.c_str(), section.c_str(), i->first.size()))
			continue;
		
		if (ret_iter == ie)
		{
			ret_iter = i;
			max_size = i->first.size();
		} else if (max_size < i->first.size())
		{
			ret_iter = i;
			max_size = i->first.size();
		}
	}
	return ret_iter;
}

void statistics_collector::save_file(csv_files::value_type const & val)
{
	//LPCSTR new_file_name;
	//STRCONCAT(new_file_name, val.first.c_str(), ".csv");
	string_path new_file_name;
	strconcat(sizeof(new_file_name), new_file_name, val.first.c_str(), ".csv");

	char temp_string[1024];
	xr_string dest_string;
	dest_string.reserve(4096);
	
	dest_string.append("\"section_name\",");
	for (params_collection::const_iterator i = val.second->begin(),
		ie = val.second->end(); i != ie; ++i)
	{
		sprintf_s(temp_string, "\"%s\",", i->c_str());
		dest_string.append(temp_string);
	}
	dest_string.erase(dest_string.end() - 1);
	dest_string.append("\r\n");
	
	for (xr_vector<shared_str>::const_iterator i = m_wpn_collection->all_weapons.begin(),
		ie = m_wpn_collection->all_weapons.end(); i != ie; ++i)
	{
		csv_files::const_iterator temp_cit = get_most_acceptable_group(*i);
		if (temp_cit == m_all_params.end())
			continue;
		if (temp_cit->first != val.first)
			continue;

		sprintf_s(temp_string, "\"%s\",", i->c_str());
		dest_string.append(temp_string);
		for (params_collection::const_iterator param_i = val.second->begin(),
			param_ie = val.second->end(); param_i != param_ie; ++param_i)
		{
			LPCSTR val = m_wpn_collection->priquel_config->r_string(i->c_str(), param_i->c_str());
			if (!val)
				val = "";
			sprintf_s(temp_string, "\"%s\",", val);
			dest_string.append(temp_string);
		}
		dest_string.erase(dest_string.end() - 1);
		dest_string.append("\r\n");
	}
	
	IWriter* new_file = FS.w_open(new_file_name);
	R_ASSERT(new_file);
	new_file->w_string(dest_string.c_str());
	FS.w_close(new_file);
}