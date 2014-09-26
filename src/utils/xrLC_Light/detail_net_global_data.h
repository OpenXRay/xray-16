////////////////////////////////////////////////////////////////////////////
//	Created		: 27.03.2009
//	Author		: Konstantin Slipchenko
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#ifndef DETAIL_NET_GLOBAL_DATA_H_INCLUDED
#define DETAIL_NET_GLOBAL_DATA_H_INCLUDED

 // class detail_net_global_data


#include "net_global_data.h"

namespace lc_net
{

template<>
class net_global_data_impl<gl_detail_cl_data>
{
		
public:
		net_global_data_impl<gl_detail_cl_data>(){}

		void				init(  );
		void				cleanup( ) { data_cleanup( ) ;}
protected:
		void				create_data_file(LPCSTR path);
		bool				create_data(LPCSTR path);
		void				destroy_data( );
		LPCSTR				file_name( ){	return "gl_detail_cl_data";	}
		virtual	void		data_init( )=0 {};
		virtual	void		data_cleanup( )= 0{ } ;
};


//template<> struct global_add_global<gl_detail_cl_data, gl_lm_data>{};


}


#endif // #ifndef DETAIL_NET_GLOBAL_DATA_H_INCLUDED