#include "stdafx.h"

#include "net_execution_globals.h"

////NEVER REMOVE THESE INCLUDES////
#include "net_all_executions.h"
#include "net_all_globals.h"
////NEVER REMOVE THESE INCLUDES////
namespace lc_net
{
	
	
	

	static execution_globals_reg exe_reg;
	static globals_globals_reg gl_reg;

	const execution_globals_reg &exe_gl_reg()
	{
		return exe_reg;
	}
	
	const globals_globals_reg &gl_gl_reg	()
	{
		return gl_reg;
	}
	
	template <typename e>
	static xr_vector< xr_vector <e_net_globals>* >	&inter_get_table( enum_table<e> &table )
	{
		return table.gl_table;
	}



	template < execution_types  ie, e_net_globals ig >
	struct add_global
	{
		
		add_global( )
		{
			xr_vector<e_net_globals>& v = *inter_get_table<execution_types>(exe_reg)[ ie ] ;
			v.erase( std::find( v.begin(), v.end(), ig ) );
		}
	};

	template < e_net_globals  ie, e_net_globals ig >
	struct global_add_global
	{
		
		global_add_global( )
		{
			xr_vector<e_net_globals>& v = *inter_get_table<e_net_globals>(gl_reg)[ ie ] ;
			v.erase( std::find( v.begin(), v.end(), ig ) );
		}
	};
	
	
	template < class  ee, e_net_globals ig >
		struct add_enum;


	template < e_net_globals ie, e_net_globals ig >
		struct add_enum< type_net_globals<ie>, ig >
	{
		
		global_add_global<ie,ig>  v; 
	};

	template < execution_types ie, e_net_globals ig >
		struct add_enum< type_execution<ie>, ig >
	{
		
		add_global<ie,ig>  v; 
	};



	template< e_net_globals i, class e  >
	struct list_glob
	{
		static const e_net_globals ii =	     (e_net_globals)(i);
		static const e_net_globals next_ii = (e_net_globals)(i+1);
		typedef	list_glob<next_ii, e> next;
		next	n;
		add_enum<e,i> remove; // 
	};



	template<class e>
	struct list_glob<gl_last,e>
	{};


	template <class e>
	class it
	{

	};

	/*template< class e >
	struct list_exec;

	template< class e >
	struct list_exec*/
	template < class T >
	class n_type;

	template < e_net_globals v >
	class n_type < type_net_globals< v > >
	{
	public:	
		typedef	type_net_globals< e_net_globals(v+1) > next;
	};
	
	template < execution_types v >
	class n_type < type_execution< v > >
	{
	public:	
		typedef	type_execution< execution_types(v+1) > next;
	};
	
	template <typename e>
		struct v_type;
	template <>
	struct v_type< execution_types >
	{
		
		static const execution_types				first = et_lightmaps	;
		static const execution_types				last =	et_last			;
		typedef	type_execution< first >				enum_type;
	};
	template <>
	struct v_type< e_net_globals >
	{
		static const e_net_globals					first = gl_cl_data	;
		static const e_net_globals					last =	gl_last		;
		typedef	type_net_globals< first >			enum_type;
	};


	template<  typename e >
	struct list_exec
	{
		//typedef	typename e::type e_type;
		//static const  e_type		ii			=	( e_type )(e::value);
		//static const  e_type	next_ii		=	( e_type )(i+1);
		typedef typename n_type< e >::next n;
		typedef	list_exec< n > next;
		typedef	list_glob< gl_cl_data, e > globs;
		next in;
		globs gl;
	};

	template<>
	struct list_exec< type_execution<et_last> >
	{};

	template<>
	struct list_exec< type_net_globals<gl_last> >
	{};


	template<typename e>
	enum_table<e>::enum_table()
	{
		gl_table.resize(v_type<e>::last,0);

		for(u32 i = v_type<e>::first ; v_type<e>::last!=i ;++i)
		{
			gl_table[i] = xr_new< xr_vector <e_net_globals> >();
			for(u32 j = gl_cl_data ; gl_last!=j ;++j)
				gl_table[i]->push_back(e_net_globals(j));
		}
		list_exec< v_type <e>::enum_type > remove_globs;
	}






}