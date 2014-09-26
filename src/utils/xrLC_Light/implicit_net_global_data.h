#ifndef	_NET_IMPLICIT_GLOBAL_DATA_H_
#define	_NET_IMPLICIT_GLOBAL_DATA_H_
#include "net_global_data.h"
class ImplicitCalcGlobs;
namespace lc_net
{

template<>
class net_global_data_impl<gl_implicit_cl_data>
{
		//ImplicitCalcGlobs *data;
public:
		net_global_data_impl<gl_implicit_cl_data>(){}

		void				init(  );
		void				cleanup( ) { data_cleanup( ) ;}
protected:
		void				create_data_file(LPCSTR path);
		bool				create_data(LPCSTR path);
		void				destroy_data( );
		LPCSTR				file_name( ){	return "gl_implicit_cl_data";	}
		virtual	void		data_init( )=0 {};
		virtual	void		data_cleanup( )= 0{ } ;
};


template<> struct global_add_global<gl_implicit_cl_data, gl_lm_data>{};
//due to vertices gl_cl_data -> gl_lm_data - todo separate vertex data!

}
#endif
