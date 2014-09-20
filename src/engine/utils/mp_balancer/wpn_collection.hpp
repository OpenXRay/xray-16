#include "pch.h"

#ifndef WEAPON_COLLECTION
#define WEAPON_COLLECTION

class weapon_collection
{
	friend class statistics_collector;
protected:
	CInifileEx*		priquel_config;
	CInifileEx*		patch_config;
	
	/*CInifileEx*		work_mp_weapons;
	CInifileEx*		work_mp_ammo;
	CInifileEx*		work_mp_items;
	CInifileEx*		work_mp_outfits;*/

	CInifileEx*		settings;


	xr_vector<shared_str> all_weapons;
	xr_set<shared_str>	remember_yes_keys;
	xr_set<shared_str>	remember_no_keys;

public:
	typedef std::pair<shared_str, xr_set<shared_str> >			tentity_extract_keys;
	typedef std::map<shared_str, xr_vector<CInifileEx::Sect> >	tnew_config_map;
	typedef std::vector<tentity_extract_keys*>					textract_list;

		
	void load_all_mp_weapons();
	void load_settings();

	textract_list::const_iterator	get_extract_keys(char const * section_name);
	
	void extract_all_params();

	char const *	try_extract_from_patch(char const * sect, char const * key);
	void			copy_params_ex(CInifileEx::Sect & dest, CInifileEx::Sect const & from, xr_set<shared_str> const & copy_keys);
	void			build_section(CInifileEx::Sect & dest, 
									CInifileEx::Sect const & orig,
									xr_set<shared_str> const & extract);
	
	void			save_config_to_file(tnew_config_map::const_iterator cfg_iter);
	void			save_new_configs();
	
	weapon_collection();
	~weapon_collection();

private:
	textract_list		extract_list;
	tnew_config_map		new_config;
}; //class weapon_collection


#endif //#ifndef WEAPON_COLLECTION