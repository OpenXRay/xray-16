#pragma once
#ifdef DEBUG
std::string get_string( bool v );
std::string get_string( const Fvector& v );
std::string get_string( const Fmatrix& dop );
std::string get_string(const Fbox &box);
std::string dbg_object_base_dump_string( const CObject *obj );
std::string dbg_object_poses_dump_string( const CObject *obj );
std::string dbg_object_visual_geom_dump_string( const CObject *obj );
std::string dbg_object_props_dump_string( const CObject *obj );
std::string dbg_object_full_dump_string( const CObject *obj );
std::string dbg_object_full_capped_dump_string( const CObject *obj );
#endif