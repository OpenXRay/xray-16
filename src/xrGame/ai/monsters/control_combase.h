#pragma once
#include "control_com_defs.h"

class CControl_Manager;
class CBaseMonster;

class CControl_ComControlled;
class CControl_ComControlling;

//////////////////////////////////////////////////////////////////////////
// Base
class CControl_Com {
public:
					CControl_Com			()											{m_inited = false;}
	virtual			~CControl_Com			()											{}
	// common routines
			void	init_external			(CControl_Manager *cm, CBaseMonster *obj)	{m_man = cm; m_object = obj;}
	virtual void	load					(LPCSTR section)							{}
	virtual void	reinit					()											{m_active = false; m_inited = true;}
	virtual void	reload					(LPCSTR section)							{}

	// update
	virtual void	update_schedule			()											{}
	virtual void	update_frame			()											{}

	virtual CControl_ComControlled	*ced	()											{return 0;}
	virtual CControl_ComControlling *cing	()											{return 0;}

			void	set_active				(bool val = true)							{m_active	= val; val ? activate() : deactivate();}
			bool	is_active				()											{return m_active;}
			bool	is_inited				()											{return m_inited;}

	virtual bool check_start_conditions		() {return true;}

protected:
	virtual void	activate				()											{}
	virtual	void	deactivate				()											{}

protected:
	CControl_Manager	*m_man;
	CBaseMonster		*m_object;
	
private:
	bool				m_active;
	bool				m_inited;
};

//////////////////////////////////////////////////////////////////////////
// Controlled with data
class CControl_ComControlled {
public:
	virtual	void					reinit		() {m_locked = false; m_capturer = 0; reset_data();}
	virtual void					reset_data	(){}
	
	virtual	ControlCom::IComData	*data		()	{return 0;}
	
	// init/deinit current work
	virtual void					on_capture	() {reset_data();}
	virtual void					on_release	() {}
	
			bool					is_locked	() {return m_locked;}
			void					set_locked	(bool val = true) {m_locked = val;}
			CControl_Com			*capturer	() {return m_capturer;}
			void					set_capturer(CControl_Com *com){m_capturer = com;}
private:
	CControl_Com					*m_capturer;
	bool							m_locked;
};

//////////////////////////////////////////////////////////////////////////
// Controlling
class CControl_ComControlling {
public:
	virtual ~CControl_ComControlling		() {}
	virtual	void	reinit					() {}

	// initialize/finalize controlling com
	virtual void	on_start_control		(ControlCom::EControlType type) {}
	virtual void	on_stop_control			(ControlCom::EControlType type) {}

	// event handling
	virtual void	on_event				(ControlCom::EEventType, ControlCom::IEventData*)	{}

protected:
	DEFINE_VECTOR			(CControl_Com*, CONTROLLERS_VECTOR, CONTROLLERS_VECTOR_IT);
	CONTROLLERS_VECTOR		m_controlled;
};

//////////////////////////////////////////////////////////////////////////
// Data Storage
template<class T>
class CControl_ComControlledStorage : public CControl_ComControlled {
public:
	virtual	ControlCom::IComData *data		()	{return &m_data;}
protected:
	T										m_data;
};

//////////////////////////////////////////////////////////////////////////
// Pure
template<class T>
class CControl_ComPure : 
	public CControl_Com, 
	public CControl_ComControlledStorage<T>
{
public:
	virtual CControl_ComControlled	*ced	() {return this;}
	virtual	void					reinit	() 
	{
		CControl_Com::reinit						();
		CControl_ComControlledStorage<T>::reinit	();
		set_active									();
	}
};
//////////////////////////////////////////////////////////////////////////
// Base
class CControl_ComBase : 
	public CControl_Com,
	public CControl_ComControlling 
{
public:
	virtual CControl_ComControlling *cing	() {return this;}
	virtual	void					reinit	() 
	{
		CControl_Com::reinit			();
		CControl_ComControlling::reinit	();
		set_active						();
	}
};
//////////////////////////////////////////////////////////////////////////
// Custom
template<class T = ControlCom::IComData>
class CControl_ComCustom : 
	public CControl_Com,
	public CControl_ComControlledStorage<T>,
	public CControl_ComControlling
{
public:
	virtual CControl_ComControlled	*ced	() {return this;}
	virtual CControl_ComControlling *cing	() {return this;}
	virtual	void					reinit	() 
	{
		CControl_Com::reinit						();
		CControl_ComControlledStorage<T>::reinit	();
		CControl_ComControlling::reinit				();
	}

	virtual bool check_start_conditions		() {return false;}
};

