////////////////////////////////////////////////////////////////////////////
//	Created		: 09.04.2009
//	Author		: Konstantin Slipchenko
//	Copyright (C) GSC Game World - 2009
////////////////////////////////////////////////////////////////////////////

#ifndef VECTOR_CLEAR_H_INCLUDED
#define VECTOR_CLEAR_H_INCLUDED

template<typename T, typename TDestruct >
void vec_value_destruct( xr_vector<T*> &v, TDestruct &destruct )
{
	typename xr_vector<T*>::iterator i = v.begin(), e = v.end();
	for(;i!=e;++i)
		destruct.destruct(*i);
}
template<typename T, typename TDestruct >
void vec_clear( xr_vector<T*> &v, TDestruct &destruct )
{
	vec_value_destruct( v, destruct );
	v.clear();
}

#endif // #ifndef VECTOR_CLEAR_H_INCLUDED