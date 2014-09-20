#ifndef _DEFINES_H_
#define _DEFINES_H_

#define _RELEASE(x)			{ if(x) { (x)->Release();       (x)=NULL; } }
#define _SHOW_REF(msg, x)   { VERIFY(x); x->AddRef(); DWORDToFile(msg,x->Release()); }
#define THROW				throw 123456789

// game path definition
#define _game_data_				"$game_data$"
#define _game_textures_			"$game_textures$"
#define _game_levels_			"$game_levels$"
#define _game_sounds_			"$game_sounds$"
#define _game_meshes_			"$game_meshes$"
#define _game_shaders_			"$game_shaders$"

// editor path definition
#define _server_root_		    "$server_root$"
#define _server_data_root_	    "$server_data_root$"
#define _local_root_		    "$local_root$"
#define _import_			    "$import$"
#define _sounds_			    "$sounds$"
#define _textures_			    "$textures$"
#define _objects_			    "$objects$"
#define _maps_				    "$maps$"
#define _temp_				    "$temp$"
#define _omotion_			    "$omotion$"
#define _omotions_			    "$omotions$"
#define _smotion_			    "$smotion$"
#define _detail_objects_	    "$detail_objects$"

#endif
