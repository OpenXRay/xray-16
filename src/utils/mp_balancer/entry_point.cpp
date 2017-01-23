#include "pch.h"
#include "wpn_collection.hpp"
#include "statistics_collector.hpp"

#pragma comment(lib,"xrCore.LIB")

void main(int argc, char* argv[])
{
	Core._initialize("mp_ballancer", NULL, TRUE, "fsgame4mpu.ltx");
	
	SetConsoleOutputCP(1251);

	weapon_collection	wpn_collection;

	wpn_collection.load_all_mp_weapons();

	if (argc == 2)
	{
		if (!strcmp(argv[1], "export_configs"))
		{
			wpn_collection.extract_all_params	();
			wpn_collection.save_new_configs		();
		} else if (!strcmp(argv[1], "made_csv"))
		{
			statistics_collector stat_collector(&wpn_collection);
			stat_collector.load_settings();
			stat_collector.save_files();
		}
	} 
			
	Core._destroy	();
}