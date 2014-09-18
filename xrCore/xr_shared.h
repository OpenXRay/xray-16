#ifndef xr_sharedH
#define xr_sharedH
#pragma once

class XRCORE_API shared_value
{
public:
	int						m_ref_cnt;
	virtual					~shared_value		(){}
};

template <class T>
class shared_container
{
protected:
	typedef xr_map< shared_str,T* >			SharedMap;
	typedef typename SharedMap::iterator	SharedMapIt;
	SharedMap				container;
public:
							shared_container	(){}
	virtual					~shared_container	(){VERIFY(container.empty());}
	template <typename _on_new>
	T*						dock				(shared_str key, const _on_new& p)
	{
		T*	result				= 0	;
		SharedMapIt	I			= container.find	(key);
		if (I!=container.end())	result = I->second;
		if (0==result)			{
			result				= xr_new<T>();
			result->m_ref_cnt	= 0;
			if (p(key,result))	container.insert(mk_pair(key,result));
			else				xr_delete		(result);
		}
		return				result;
	}
	virtual void			clean				(bool force_destroy)
	{
		SharedMapIt it			= container.begin();
		SharedMapIt _E			= container.end();
		if (force_destroy){
			for (; it!=_E; it++){
				T*	sv			= it->second;
				xr_delete		(sv);
			}
			container.clear		();
		}else{
			for (; it!=_E; )	{
				T*	sv			= it->second;
				if (0==sv->m_ref_cnt){
					SharedMapIt	i_current	= it;
					SharedMapIt	i_next		= ++it;
					xr_delete			(sv);
					container.erase		(i_current);
					it					= i_next;
				}else{
					it++;
				}
			}
		}
	}
};

template <class T>
class shared_item
{
protected:
	T*						p_;
protected:
	// ref-counting
	void					destroy				()							{	if (0==p_) return;	p_->m_ref_cnt--; 	if (0==p_->m_ref_cnt)	p_=0;	}
	void					create				(shared_item const &rhs)	{	T* v = rhs.p_; if (0!=v) v->m_ref_cnt++; destroy(); p_ = v;	}
public:
	// construction
							shared_item			()							{	p_ = 0;											}
							shared_item			(shared_item const &rhs)	{	p_ = 0;	create(rhs);							}
							~shared_item		()							{	destroy();										}
	// assignment & accessors
	shared_item<T>&			operator=			(shared_item const &rhs)	{	create(rhs);return *this;	}
	const T*				get_value			()							{	return p_;					}
	// creating
	template <typename _on_new>
	void					create				(shared_str key, shared_container<T>* container, const _on_new& p){	T* v = container->dock(key,p); if (0!=v) v->m_ref_cnt++; destroy(); p_ = v;	}
};

#endif
