#ifndef DEMO_INFO_LOADER
#define DEMO_INFO_LOADER

#include "DemoInfo.h"
#include "../xrServerEntities/associative_vector.h"

class demo_info_loader
{
private:
	typedef associative_vector<shared_str, demo_info*> demo_info_cache_t;
	demo_info_cache_t	m_demo_info_cache;

	demo_info*				load_demofile		(LPCSTR demo_file_name);
public:
							demo_info_loader	();
							~demo_info_loader	();

	demo_info const *		get_demofile_info	(LPCSTR demo_file_name);
}; //class demo_info_loader

#endif //#ifndef DEMO_INFO_LOADER