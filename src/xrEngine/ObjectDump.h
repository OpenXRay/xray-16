#pragma once
#ifdef DEBUG

ENGINE_API std::string dbg_object_base_dump_string( const CObject *obj );
ENGINE_API std::string dbg_object_poses_dump_string( const CObject *obj );
ENGINE_API std::string dbg_object_visual_geom_dump_string( const CObject *obj );
ENGINE_API std::string dbg_object_props_dump_string( const CObject *obj );
ENGINE_API std::string dbg_object_full_dump_string( const CObject *obj );
ENGINE_API std::string dbg_object_full_capped_dump_string( const CObject *obj );
#endif