#ifndef	_NET_LC_GLOBAL_DATA_H_
#define	_NET_LC_GLOBAL_DATA_H_

#include "net_global_data.h"

namespace lc_net
{

template<>
class net_global_data_impl<gl_cl_data>
{
	
public:
		void				init()		{ data_init( ); }
protected:
		void				create_data_file	(LPCSTR path);
		bool				create_data			(LPCSTR path);
		void				destroy_data		( ){};
		LPCSTR				file_name			( ){	return "gl_cl_data";	}
		virtual	void		data_init			( )=0 {};
		virtual	void		data_cleanup		( )=0 {};
};

template<> struct global_add_global<gl_cl_data, gl_base_cl_data>{};

}
#endif