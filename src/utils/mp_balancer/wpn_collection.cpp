#include "wpn_collection.hpp"

weapon_collection::weapon_collection()
{
}

weapon_collection::~weapon_collection()
{
	if (priquel_config)
		xr_delete	(priquel_config);
	if (patch_config)
		xr_delete	(patch_config);

	if (settings)
	{
		xr_delete	(settings);
	}

	delete_data		(extract_list);
}

void weapon_collection::load_all_mp_weapons()
{
	string_path		path_ltx;
	
	FS.update_path			(path_ltx, "$patch_config$", "system.ltx");
	patch_config			= xr_new<CInifileEx>(path_ltx, TRUE, TRUE, FALSE);
	
	FS.update_path			(path_ltx, "$game_config$", "system.ltx");
	priquel_config			= xr_new<CInifileEx>(path_ltx, TRUE, TRUE, FALSE);

	/*FS.update_path			(path_ltx, "$game_config$", "mp\\weapons_mp\\weapons_mp_for_work.ltx");
	work_mp_weapons			= xr_new<CInifileEx>(path_ltx, TRUE, TRUE, FALSE);

	FS.update_path			(path_ltx, "$game_config$", "mp\\weapons_mp\\ammo_mp_for_work.ltx");
	work_mp_ammo				= xr_new<CInifileEx>(path_ltx, TRUE, TRUE, FALSE);

	FS.update_path			(path_ltx, "$game_config$", "mp\\weapons_mp\\items_mp_for_work.ltx");
	work_mp_items			= xr_new<CInifileEx>(path_ltx, TRUE, TRUE, FALSE);
	
	FS.update_path			(path_ltx, "$game_config$", "mp\\weapons_mp\\outfit_mp_for_work.ltx");
	work_mp_outfits			= xr_new<CInifileEx>(path_ltx, TRUE, TRUE, FALSE);*/

	FS.update_path			(path_ltx, "$app_data_root$", "export_settings.ltx");
	settings				= xr_new<CInifileEx>(path_ltx, TRUE, TRUE, FALSE);
	load_settings			();
		
	
	
	/*new_mp_weapons			= xr_new<CInifileEx>("new_weapons_mp.ltx", FALSE, FALSE, FALSE);
	new_mp_ammo				= xr_new<CInifileEx>("new_ammo_mp.ltx", FALSE, FALSE, FALSE);
	new_mp_items			= xr_new<CInifileEx>("new_items_mp.ltx", FALSE, FALSE, FALSE);
	new_mp_outfits			= xr_new<CInifileEx>("new_outfit_mp.ltx", FALSE, FALSE, FALSE);*/

	
	CInifileEx::Sect & dm_base_cost = priquel_config->r_section("deathmatch_base_cost");
	CInifileEx::SectIt_	ie = dm_base_cost.Data.end();
	std::cout << "Found next weapons and ammo:" << std::endl;
	for (CInifileEx::SectIt_ i = dm_base_cost.Data.begin(); i != ie; ++i)
	{
		std::cout << i->first.c_str() << std::endl;
		all_weapons.push_back(i->first);
	}
}

struct SymbolCountComparator : public std::binary_function<
	weapon_collection::tentity_extract_keys, 
	weapon_collection::tentity_extract_keys, bool>
{
	bool operator () (weapon_collection::tentity_extract_keys const * left,
		weapon_collection::tentity_extract_keys const * right) const
	{
		return (left->first.size() >= right->first.size());
	};
};

void read_arguments_to_set(xr_set<shared_str> & dest, char const * source_string)
{
	xr_string	tmp;
	
	for (int k = 0,
		cnt	= _GetItemCount(source_string); k < cnt; ++k)
	{
		_GetItem	(source_string, k,tmp);
		dest.insert(tmp.c_str());
	}
}

#define EXPORT_SETTINGS_SECT "export_settings"
void weapon_collection::load_settings()
{
	u32 params_count = settings->line_count(EXPORT_SETTINGS_SECT);
	extract_list.reserve	(params_count);
	for (u32 param = 0; param < params_count; ++param)
	{
		const char * line = NULL;
		const char * larg = NULL;
		settings->r_line(EXPORT_SETTINGS_SECT, param, &line, &larg);
		R_ASSERT(line);
		extract_list.push_back(xr_new<tentity_extract_keys>());
		extract_list.back()->first = line;
		new_config.insert(std::make_pair(shared_str(line), xr_vector<CInifileEx::Sect>()));
		if (larg)
		{
			read_arguments_to_set(extract_list.back()->second, larg);
		}
	}
	std::sort(extract_list.begin(), extract_list.end(), SymbolCountComparator());
}

weapon_collection::textract_list::const_iterator weapon_collection::get_extract_keys(char const * section_name)
{
	for (weapon_collection::textract_list::const_iterator i = extract_list.begin(),
		ie = extract_list.end(); i != ie; ++i)
	{
		if (!strncmp((*i)->first.c_str(), section_name, (*i)->first.size()))
			return i;
	}

	return			extract_list.end();
}

//save all items using existing structure..
void weapon_collection::extract_all_params()
{
	std::cout << "----------------- started to save values -----------------" << std::endl;
	xr_set<shared_str> no_extract_params;
	xr_vector<shared_str>::const_iterator ie = all_weapons.end();
	for (xr_vector<shared_str>::const_iterator i = all_weapons.begin();
		i != ie; ++i)
	{
		textract_list::const_iterator extr_iter = get_extract_keys(i->c_str());
		if (extr_iter == extract_list.end())
		{
			std::cerr << "Not found extract list for section: " << i->c_str() << std::endl;
			continue;
		}
		std::cout << "\r\n\r\nProcessing section: " << i->c_str() << std::endl;
		

		CInifileEx::Sect new_sect;
		CInifileEx::Sect & temp_sect = priquel_config->r_section(*i);
		build_section(new_sect, temp_sect, (*extr_iter)->second);

		new_config[(*extr_iter)->first].push_back(new_sect);



		/*u32 params_count = priquel_config->line_count(*i);
		for (u32 param = 0; param < params_count; ++param)
		{
			const char * line = NULL;
			const char * larg = NULL;
			const char * patch_arg = NULL;
			const char * base_line = NULL;
			priquel_config->r_line(i->c_str(), param, &line, &larg);
			R_ASSERT(line);
			if (!larg) larg = "";
			std::cout << "Key: " << line << ", value: " << larg << std::endl;
			write_store->remove_line(i->c_str(), line);
			if (mp_base->line_exist(base_section, line))
			{
				std::cout << "This key present in base section ...\r\n";
				continue;
			}
			patch_arg = try_extract_from_patch(i->c_str(), line);
			if (patch_arg)
			{
				if (strcmp(larg, patch_arg))
				{
					std::cout << "Patch value is not equal: " << patch_arg << std::endl;
					if (remember_yes_keys.find(shared_str(line)) != remember_yes_keys.end())
					{
						larg = patch_arg;
						std::cout << "Processing YES action...\r\n";
					} else if (remember_no_keys.find(shared_str(line)) != remember_no_keys.end())
					{
						std::cout << "Processing NO action...\r\n";
					} else
					{
						std::cout << "Do you want to save patch value ? (y - single yes, Y - multiple yes, n - single no, N - multiple no):";
						int ch = _getch();
						if (ch == 'y')
						{
							larg = patch_arg;
						} else if (ch == 'Y')
						{
							remember_yes_keys.insert(shared_str(line));
							larg = patch_arg;
						} else if (ch == 'N')
						{
							remember_no_keys.insert(shared_str(line));
						}
					}
				}
			}
			std::cout << "Saving key: " << line << ", value: " << larg << std::endl << std::endl;
			write_store->w_string(temp_section, line, larg);
		}*/
	}
	
}

char const * weapon_collection::try_extract_from_patch(char const * sect, char const * line)
{
	R_ASSERT(sect && line);
	if (patch_config->line_exist(sect, line))
	{
		return patch_config->r_string(sect, line);
	}
	return NULL;
}

void weapon_collection::copy_params_ex(CInifileEx::Sect & dest, CInifileEx::Sect const & from, xr_set<shared_str> const & copy_keys)
{
	std::cout << "Processing section: " << from.Name.c_str() << std::endl;
	for (CInifileEx::SectCIt i = from.Data.begin(),
		ie = from.Data.end(); i != ie; ++i)
	{
		if (copy_keys.find(i->first) != copy_keys.end())
		{
			char const * larg = try_extract_from_patch(from.Name.c_str(), i->first.c_str());
			CInifileEx::Item temp_item;
			temp_item.first = i->first;
			temp_item.second = priquel_config->r_string(dest.Name.c_str(), temp_item.first.c_str());
			temp_item.comment = i->comment;
			if (larg)
			{
				if (temp_item.second != larg)
				{
//--------------------------------
					std::cout << "Key: " << temp_item.first.c_str() << "\r\n";
					if (temp_item.comment.c_str())
					{
						std::cout << "Comment: " << temp_item.comment.c_str() << std::endl;
					}
					
					std::cout << "Values not EQUAL:\r\n"
						<< "Priquel value is: " << temp_item.second.c_str()
						<< ", patch value is: " << larg << std::endl;
					if (remember_yes_keys.find(shared_str(temp_item.first)) != remember_yes_keys.end())
					{
						temp_item.second = larg;
						std::cout << "Processing YES action...\r\n";
					} else if (remember_no_keys.find(shared_str(temp_item.first)) != remember_no_keys.end())
					{
						std::cout << "Processing NO action...\r\n";
					} else
					{
						std::cout << "Do you want to save patch value ? (y - single yes, Y - multiple yes, n - single no, N - multiple no):";
						int ch = _getch();
						if (ch == 'y')
						{
							temp_item.second = larg;
						} else if (ch == 'Y')
						{
							remember_yes_keys.insert(shared_str(temp_item.first));
							temp_item.second = larg;
						} else if (ch == 'N')
						{
							remember_no_keys.insert(shared_str(temp_item.first));
						}
						std::cout << std::endl;
					}
//--------------------------------
				}
			}
			dest.Data.push_back(temp_item);
		}
		
	}
}

void weapon_collection::build_section(CInifileEx::Sect & dest, 
									CInifileEx::Sect const & orig,
									xr_set<shared_str> const & extract_from_base_keys)
{
	dest.Name = orig.Name;
	dest.base_sections = orig.base_sections;

	for (xr_vector<shared_str>::const_iterator bi = orig.base_sections.begin(),
		bie = orig.base_sections.end();	bi != bie; ++bi)
	{
		R_ASSERT2(priquel_config->section_exist(bi->c_str()), "base section not exist");
		CInifileEx::Sect & base_sect = priquel_config->r_section(bi->c_str());
		copy_params_ex(dest, base_sect, extract_from_base_keys);
	}

	for (CInifileEx::SectCIt i = orig.Data.begin(), ie = orig.Data.end(); i != ie; ++i)
	{
		bool found_key = false;
		
		for (xr_vector<shared_str>::const_iterator bi = orig.base_sections.begin(),
			bie = orig.base_sections.end(); bi != bie; ++bi)
		{
			if (priquel_config->line_exist(bi->c_str(), i->first.c_str()))
			{
				//char const * larg = config->r_string(bi->c_str(), i->first.c_str());
				//if (i->second == larg)
				//{
				found_key = true;
				break;
				//}
			}
		}
		if (!found_key)
		{
			dest.Data.push_back(*i);
		}
	}
}

void weapon_collection::save_config_to_file(tnew_config_map::const_iterator cfg_iter)
{
	xr_set<shared_str> comments_set;
	IWriter* new_file = FS.w_open(cfg_iter->first.c_str());
	tnew_config_map::mapped_type const & sect_collection = cfg_iter->second;
	xr_string temp_string;
	temp_string.reserve(1024 * 5);
	char temp_buffer[2096];
	for (tnew_config_map::mapped_type::const_iterator i = sect_collection.begin(),
		ie = sect_collection.end(); i != ie; ++i)
	{
		temp_string.clear();
		sprintf_s(temp_buffer, "[%s]", i->Name.c_str());
		
		temp_string.append(temp_buffer);
		if (i->base_sections.size())
		{
			temp_string.append(":");
			for (xr_vector<shared_str>::const_iterator bi = i->base_sections.begin(),
				bie = i->base_sections.end(); bi != bie; ++bi)
			{
				temp_string.append(bi->c_str());
				if ((bi + 1) != bie)
					temp_string.append(", ");
			}
		}
		temp_string.append("\r\n");

		for (CInifileEx::SectCIt si = i->Data.begin(),
			sie = i->Data.end(); si != sie; ++si)
		{
			sprintf_s(temp_buffer, "%4s%-32s = %-8s", " ", si->first.c_str(), si->second.c_str() ? si->second.c_str() : "");
			if (si->comment.c_str())
			{
				if (comments_set.find(si->first) == comments_set.end())
				{
					strcat_s(temp_buffer, ";");
					strcat_s(temp_buffer, si->comment.c_str());
					comments_set.insert(si->first);
				}
			}
			strcat_s(temp_buffer, "\r\n");
			temp_string.append(temp_buffer);
		}
		temp_string.append("\r\n");
		new_file->w_string(temp_string.c_str());
	}
	FS.w_close(new_file);
}

void weapon_collection::save_new_configs()
{
	for (tnew_config_map::const_iterator i = new_config.begin(),
		ie = new_config.end(); i != ie; ++i)
	{
		save_config_to_file(i);
	}
}