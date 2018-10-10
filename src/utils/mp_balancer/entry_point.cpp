#include "pch.h"
#include "wpn_collection.hpp"
#include "statistics_collector.hpp"

void main(int argc, char* argv[])
{
    xrDebug::Initialize(false);
    Core.Initialize("mp_ballancer", nullptr, NULL, TRUE, "fsgame.ltx");

    SetConsoleOutputCP(1251);

    weapon_collection wpn_collection;

    wpn_collection.load_all_mp_weapons();

    if (argc == 2)
    {
        if (!strcmp(argv[1], "export_configs"))
        {
            wpn_collection.extract_all_params();
            wpn_collection.save_new_configs();
        }
        else if (!strcmp(argv[1], "made_csv"))
        {
            statistics_collector stat_collector(&wpn_collection);
            stat_collector.load_settings();
            stat_collector.save_files();
        }
    }

    Core._destroy();
}
