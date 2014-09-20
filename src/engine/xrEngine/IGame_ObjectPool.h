#pragma once

// refs
class ENGINE_API				CObject;

//-----------------------------------------------------------------------------------------------------------
class ENGINE_API 				IGame_ObjectPool
{
/*
private:
	struct str_pred : public std::binary_function<shared_str&, shared_str&, bool> 
	{	
		IC bool operator()(const shared_str& x, const shared_str& y) const
		{	return xr_strcmp(x,y)<0;	}
	};
	typedef xr_multimap<shared_str,CObject*,str_pred>	POOL;
	typedef POOL::iterator							POOL_IT;
private:
	POOL						map_POOL;
*/
	typedef xr_vector<CObject*>	ObjectVec;
	typedef ObjectVec::iterator	ObjectVecIt;
	ObjectVec					m_PrefetchObjects;
public:
	void						prefetch			( );
	void						clear				( );

	CObject*					create				( LPCSTR	name	);
	void						destroy				( CObject*	O		);

	IGame_ObjectPool			();
	virtual ~IGame_ObjectPool	();
};
//-----------------------------------------------------------------------------------------------------------
