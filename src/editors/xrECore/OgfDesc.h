#pragma once

// OGF_DESC
struct ECORE_API ogf_desc
{
    shared_str source_file;
    shared_str build_name;
    time_t build_time;
    shared_str create_name;
    time_t create_time;
    shared_str modif_name;
    time_t modif_time;
    ogf_desc() : build_time(0), create_time(0), modif_time(0) {}
    void Load(IReader& F);
    void Save(IWriter& F);
};
