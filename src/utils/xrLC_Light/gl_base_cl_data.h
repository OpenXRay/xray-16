////////////////////////////////////////////////////////////////////////////
//	Created		: 19.05.2009
//	Author		: Konstantin Slipchenko
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#ifndef GL_BASE_CL_DATA_H_INCLUDED
#define GL_BASE_CL_DATA_H_INCLUDED

#include "net_global_data.h"

namespace lc_net
{

template<>
class net_global_data_impl<gl_base_cl_data>
{
	
public:
		void				init()		{ data_init( ); }
protected:
		void				create_data_file	(LPCSTR path);
		bool				create_data			(LPCSTR path);
		void				destroy_data		( ){};
		LPCSTR				file_name			( ){	return "gl_base_cl_data";	}
		virtual	void		data_init			( )=0 {};
		virtual	void		data_cleanup		( )=0 {};
};

}

#endif // #ifndef GL_BASE_CL_DATA_H_INCLUDED