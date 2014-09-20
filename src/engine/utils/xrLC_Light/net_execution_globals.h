#ifndef	_NET_EXECUTION_GLOBALS_REG_
#define	_NET_EXECUTION_GLOBALS_REG_
#include "net_global_data.h"
#include "net_execution.h"
namespace lc_net
{
	
	template <e_net_globals e> 
	class type_net_globals
	{
		public:
		typedef		 e_net_globals		type				;
		static const type				value = e			;
		//static const type				first = gl_cl_data	;
		//static const type				last =	gl_last		;
	};

	template <execution_types e> 
	class type_execution
	{
		public:
		typedef		 execution_types	type					;
		static const type				value = e				;
		//static const type				first = et_lightmaps	;
		//static const type				last =	et_last			;
	};

	template<typename e>
		class enum_table;
	template <typename e>
	static xr_vector< xr_vector <e_net_globals>* >	&inter_get_table( enum_table<e> &table );

	template<typename e>
	class enum_table
	{
	public:
		typedef	xr_vector< xr_vector <e_net_globals>* >			type_table;
	private:
		friend	  type_table	&inter_get_table <e>( enum_table &table );
		type_table				gl_table;
	public:
		enum_table			();
		const xr_vector<e_net_globals>&	get_globals	( e et ) const
		{
			return *gl_table[ et ];
		}
	private:

	};
	typedef enum_table<execution_types> execution_globals_reg;
	typedef enum_table<e_net_globals> globals_globals_reg;

	const execution_globals_reg &exe_gl_reg	();
	const globals_globals_reg &gl_gl_reg	();


}
#endif