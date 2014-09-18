#ifndef	_NET_EXECUTION_FACTORY_
#define	_NET_EXECUTION_FACTORY_

#include "net_execution.h"

namespace lc_net{

	class base_execution_type_creator
	{
		public:
		virtual net_execution*	create			( u32 _net_id ) = 0;
		virtual	net_execution*  pool_create		( ) = 0;
		virtual	void			pool_destroy	( net_execution* &e ) = 0;
		virtual	u32				type			( ) = 0;
		virtual	void			set_pool_size	( u32 size ) = 0;
 		virtual	void			free_pool		( ) = 0;

	};




	class factory
	{
		
	public:
		//typedef std::pair<u32, base_execution_type_creator*> type_reg;
		typedef  base_execution_type_creator* type_reg;
	private:
		xr_vector< type_reg > vec_types;
	private:
			net_execution			*create_in_pool( u32 type_id );
			void					destroy_in_pool( net_execution* &e );
	public:
			net_execution			*create( u32 type_id, u32 _net_id );

			template < execution_types etype >
			tnet_execution_base< etype >	*create(  )
			{
				//	return (tnet_execution_base< etype >*) create( etype, u32(-1) );
					return (tnet_execution_base< etype >*) create_in_pool( etype );

			}
			virtual	void			destroy( net_execution* &e );
		
			void free_pool			( u32 type_id )	
			{
				R_ASSERT(type_id<et_last);
				vec_types[type_id]->free_pool		( );
			}
			
			factory					();
			~factory				();
	


		public:
			void							register_type	( base_execution_type_creator* creator );
		private:
		//	xr_vector< type_reg >::iterator find_type		( u32 id  );
			void							register_all	();
			void							clear			();

	};

extern factory execution_factory;
};
#endif