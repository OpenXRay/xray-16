#include	"stdafx.h"

#include	"net_execution_factory.h"

namespace lc_net{




	lc_net::factory execution_factory;

	
	factory::factory					()
	{
		register_all();
	}
	factory::~factory				()
	{
		clear();
	}

	//struct sfind_type
	//{
	//	u32 _id;
	//	sfind_type(  u32 id  ): _id( id ){}
	//	bool operator () ( const factory::type_reg &tr )
	//	{
	//		return tr->type() == _id;
	//	}
	//};

	//xr_vector< factory::type_reg >::iterator factory::find_type(  u32 id  ) 
	//{
	//	return  std::find_if( vec_types.begin(), vec_types.end(), sfind_type( id )  );
	//}

	void	lc_net::factory::register_type( base_execution_type_creator* creator )
	{
		//R_ASSERT2( vec_types.end() == find_type( creator->type() ),"type already regestred!" );
		VERIFY( creator );
		R_ASSERT(vec_types.size()==et_last);
		R_ASSERT( vec_types[creator->type()] == 0 );
		vec_types[ creator->type() ] =   creator;
	}
	


	net_execution*	factory::create( u32 type_id, u32 _net_id )
	{
		//xr_vector< factory::type_reg >::iterator i= find_type(type_id);
		//R_ASSERT2( vec_types.end() != i,"type has not regestred!" );
		//VERIFY((*i));
		R_ASSERT(type_id<et_last);
		return vec_types[type_id]->create(_net_id);

	}
	net_execution			*factory::create_in_pool( u32 type_id )
	{
		
		//return create( type_id, u32(-1) );
		R_ASSERT(type_id<et_last);
		return vec_types[type_id]->pool_create( );

	}

	void factory::destroy( net_execution* &e )
	{
		destroy_in_pool( e );
	}
	void factory::destroy_in_pool( net_execution* &e )
	{
		if(!e)
			return;
		type_reg creator = vec_types[e->type()];
		creator->pool_destroy( e );
	}




	void factory::clear()
	{
		xr_vector< factory::type_reg >::iterator i = vec_types.begin(), e = vec_types.end();
		for(;e!=i;++i)
			xr_delete((*i));
		vec_types.clear();
	}
};