////////////////////////////////////////////////////////////////////////////
//	Module 		: state_adapter.h
//	Created 	: 23.04.2008
//  Modified 	: 23.04.2008
//	Author		: Lain
//	Description : encircle state
////////////////////////////////////////////////////////////////////////////

#ifndef GROUP_STATE_ADAPTER_H_INCLUDED
#define GROUP_STATE_ADAPTER_H_INCLUDED

#include "../state.h"
#include "../basemonster/base_monster.h"

class CBaseMonster;

class CMonsterStateInterface
{
public:
	CMonsterStateInterface(CBaseMonster* p_object) : m_object(p_object) {}
	virtual ~CMonsterStateInterface  () {}

	virtual void*   get_data         () = 0 {}
	virtual void    initialize		 () { time_state_started = Device.dwTimeGlobal; }
	virtual	void    execute			 () {}
	virtual bool    check_completion () { return true; }

protected:
	CBaseMonster*   m_object;
	u32             time_state_started;
};

template<typename _Object>
class CMonsterStateAdapter : public CState<_Object> 
{	
	typedef CState<_Object> inherited;

public:
	template <class Impl>
	CMonsterStateAdapter (Impl* p_impl, _Object* obj) 
		                  :
	                      m_impl(p_impl), inherited(obj, p_impl->get_data()) {}

	virtual ~CMonsterStateAdapter () { xr_delete(m_impl); }

	virtual void	initialize		 () { m_impl->initialize(); }
	virtual	void	execute			 () { m_impl->execute(); }
	virtual bool	check_completion () { return m_impl->check_completion(); }

	virtual void	remove_links	 (CObject* object) { inherited::remove_links(object);}

private:
	CMonsterStateInterface* m_impl;
};

#endif // GROUP_STATE_ADAPTER_H_INCLUDED