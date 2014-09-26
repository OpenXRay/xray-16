////////////////////////////////////////////////////////////////////////////
//	Module 		: object_actions.h
//	Created 	: 12.03.2004
//  Modified 	: 26.03.2004
//	Author		: Dmitriy Iassenev
//	Description : Object actions
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "action_base.h"

class CAI_Stalker;
class CInventoryItem;
class CFoodItem;
class CWeaponMagazined;
class CWeapon;
//////////////////////////////////////////////////////////////////////////
// CObjectActionBase
//////////////////////////////////////////////////////////////////////////

template <typename _item_type>
class CObjectActionBase : public CActionBase<CAI_Stalker> {
protected:
	typedef CActionBase<CAI_Stalker>					inherited;
	typedef GraphEngineSpace::_solver_condition_type	_condition_type;
	typedef GraphEngineSpace::_solver_value_type		_value_type;

protected:
	_item_type			*m_item;

public:
	IC					CObjectActionBase	(_item_type *item, CAI_Stalker *owner, CPropertyStorage *storage, LPCSTR action_name = "");
	virtual void		initialize			();
	IC		void		set_property		(_condition_type condition_id, _value_type value);
	IC		CAI_Stalker &object				() const;
			void		prevent_weapon_state_switch_ugly	( );
			void		stop_hiding_operation_if_any		( ) const;
};

class CGameObject;

typedef CObjectActionBase<CGameObject> CSObjectActionBase;

//////////////////////////////////////////////////////////////////////////
// CObjectActionMember
//////////////////////////////////////////////////////////////////////////

template <typename _item_type>
class CObjectActionMember : public CObjectActionBase<_item_type> {
protected:
	typedef CObjectActionBase<_item_type>			inherited;

protected:
	_condition_type		m_condition_id;
	_value_type			m_value;

public:
	IC					CObjectActionMember	(_item_type *item, CAI_Stalker *owner, CPropertyStorage *storage, _condition_type condition_id, _value_type value, LPCSTR action_name = "");
	virtual void		execute				();
};

//////////////////////////////////////////////////////////////////////////
// CObjectActionCommand
//////////////////////////////////////////////////////////////////////////

class CObjectActionCommand : public CObjectActionBase<CInventoryItem> {
protected:
	typedef CObjectActionBase<CInventoryItem> inherited;

protected:
	u32					m_command;

public:
						CObjectActionCommand(CInventoryItem *item, CAI_Stalker *owner, CPropertyStorage *storage, u32 command, LPCSTR action_name = "");
	virtual void		initialize			();
};

//////////////////////////////////////////////////////////////////////////
// CObjectActionShow
//////////////////////////////////////////////////////////////////////////

class CObjectActionShow : public CObjectActionBase<CInventoryItem> {
protected:
	typedef CObjectActionBase<CInventoryItem> inherited;

private:
	CWeapon				*m_weapon;

public:
						CObjectActionShow	(CInventoryItem *item, CAI_Stalker *owner, CPropertyStorage *storage, LPCSTR action_name = "");
	virtual void		initialize			();
	virtual void		execute				();
};

//////////////////////////////////////////////////////////////////////////
// CObjectActionHide
//////////////////////////////////////////////////////////////////////////

class CObjectActionHide : public CObjectActionBase<CInventoryItem> {
protected:
	typedef CObjectActionBase<CInventoryItem> inherited;

public:
						CObjectActionHide	(CInventoryItem *item, CAI_Stalker *owner, CPropertyStorage *storage, LPCSTR action_name = "");
	virtual void		execute				();
	virtual void		finalize			();
};

//////////////////////////////////////////////////////////////////////////
// CObjectActionReload
//////////////////////////////////////////////////////////////////////////

class CObjectActionReload : public CObjectActionBase<CInventoryItem> {
protected:
	typedef CObjectActionBase<CInventoryItem> inherited;

protected:
	_condition_type		m_type;

public:
						CObjectActionReload	(CInventoryItem *item, CAI_Stalker *owner, CPropertyStorage *storage, _condition_type type, LPCSTR action_name = "");
	virtual void		initialize			();
	virtual void		execute				();
};

//////////////////////////////////////////////////////////////////////////
// CObjectActionFire
//////////////////////////////////////////////////////////////////////////

class CObjectActionFire : public CObjectActionBase<CInventoryItem> {
protected:
	typedef CObjectActionBase<CInventoryItem> inherited;

protected:
	_condition_type		m_type;

public:
						CObjectActionFire	(CInventoryItem *item, CAI_Stalker *owner, CPropertyStorage *storage, _condition_type type, LPCSTR action_name = "");
	virtual void		initialize			();
	virtual void		execute				();
	virtual void		finalize			();
};

//////////////////////////////////////////////////////////////////////////
// CObjectActionFireNoReload
//////////////////////////////////////////////////////////////////////////

class CObjectActionFireNoReload : public CObjectActionBase<CInventoryItem> {
protected:
	typedef CObjectActionBase<CInventoryItem> inherited;

protected:
	_condition_type		m_type;
	bool				m_fired;

public:
						CObjectActionFireNoReload	(CInventoryItem *item, CAI_Stalker *owner, CPropertyStorage *storage, _condition_type type, LPCSTR action_name = "");
	virtual void		initialize					();
	virtual void		execute						();
	virtual void		finalize					();
};

//////////////////////////////////////////////////////////////////////////
// CObjectActionStrapping
//////////////////////////////////////////////////////////////////////////

class CObjectActionStrapping : public CObjectActionBase<CInventoryItem> {
protected:
	typedef CObjectActionBase<CInventoryItem> inherited;

private:
	bool				m_callback_removed;

private:
		void xr_stdcall	on_animation_end		();

public:
						CObjectActionStrapping	(CInventoryItem *item, CAI_Stalker *owner, CPropertyStorage *storage, LPCSTR action_name = "");
	virtual				~CObjectActionStrapping	();
	virtual void		initialize				();
	virtual void		execute					();
	virtual void		finalize				();
};

//////////////////////////////////////////////////////////////////////////
// CObjectActionStrappingToIdle
//////////////////////////////////////////////////////////////////////////

class CObjectActionStrappingToIdle : public CObjectActionBase<CInventoryItem> {
protected:
	typedef CObjectActionBase<CInventoryItem> inherited;

private:
	bool				m_callback_removed;

private:
		void xr_stdcall	on_animation_end		();

public:
						CObjectActionStrappingToIdle	(CInventoryItem *item, CAI_Stalker *owner, CPropertyStorage *storage, LPCSTR action_name = "");
	virtual				~CObjectActionStrappingToIdle	();
	virtual void		initialize						();
	virtual void		execute							();
	virtual void		finalize						();
};

//////////////////////////////////////////////////////////////////////////
// CObjectActionUnstrapping
//////////////////////////////////////////////////////////////////////////

class CObjectActionUnstrapping : public CObjectActionBase<CInventoryItem> {
protected:
	typedef CObjectActionBase<CInventoryItem> inherited;

private:
	bool				m_callback_removed;

private:
		void xr_stdcall	on_animation_end			();

public:
						CObjectActionUnstrapping	(CInventoryItem *item, CAI_Stalker *owner, CPropertyStorage *storage, LPCSTR action_name = "");
	virtual				~CObjectActionUnstrapping	();
	virtual void		initialize					();
	virtual void		execute						();
	virtual void		finalize					();
};

//////////////////////////////////////////////////////////////////////////
// CObjectActionUnstrappingToIdle
//////////////////////////////////////////////////////////////////////////

class CObjectActionUnstrappingToIdle : public CObjectActionBase<CInventoryItem> {
protected:
	typedef CObjectActionBase<CInventoryItem> inherited;

private:
	bool				m_callback_removed;

private:
		void xr_stdcall	on_animation_end		();

public:
						CObjectActionUnstrappingToIdle	(CInventoryItem *item, CAI_Stalker *owner, CPropertyStorage *storage, LPCSTR action_name = "");
	virtual				~CObjectActionUnstrappingToIdle	();
	virtual void		initialize						();
	virtual void		execute							();
	virtual void		finalize						();
};

//////////////////////////////////////////////////////////////////////////
// CObjectActionQueueWait
//////////////////////////////////////////////////////////////////////////

class CObjectActionQueueWait : public CObjectActionBase<CInventoryItem> {
protected:
	typedef CObjectActionBase<CInventoryItem> inherited;

protected:
	_condition_type		m_type;
	CWeaponMagazined	*m_magazined;

public:
						CObjectActionQueueWait	(CInventoryItem *item, CAI_Stalker *owner, CPropertyStorage *storage, _condition_type type, LPCSTR action_name = "");
	virtual void		initialize				();
	virtual void		execute					();
	virtual void		finalize				();
};

//////////////////////////////////////////////////////////////////////////
// CObjectActionSwitch
//////////////////////////////////////////////////////////////////////////

class CObjectActionSwitch : public CObjectActionBase<CInventoryItem> {
protected:
	typedef CObjectActionBase<CInventoryItem> inherited;

protected:
	_condition_type		m_type;

public:
						CObjectActionSwitch	(CInventoryItem *item, CAI_Stalker *owner, CPropertyStorage *storage, _condition_type type, LPCSTR action_name = "");
	virtual void		initialize			();
	virtual void		execute				();
	virtual void		finalize			();
};

//////////////////////////////////////////////////////////////////////////
// CObjectActionDrop
//////////////////////////////////////////////////////////////////////////

class CObjectActionDrop : public CObjectActionBase<CInventoryItem> {
protected:
	typedef CObjectActionBase<CInventoryItem> inherited;

public:
						CObjectActionDrop	(CInventoryItem *item, CAI_Stalker *owner, CPropertyStorage *storage, LPCSTR action_name = "");
	virtual void		initialize			();
};

//////////////////////////////////////////////////////////////////////////
// CObjectActionAim
//////////////////////////////////////////////////////////////////////////

class CObjectActionAim : public CObjectActionMember<CInventoryItem> {
protected:
	typedef CObjectActionMember<CInventoryItem> inherited;

private:
	CWeaponMagazined	*m_weapon;

public:
						CObjectActionAim		(CInventoryItem *item, CAI_Stalker *owner, CPropertyStorage *storage, _condition_type condition_id, _value_type value, LPCSTR action_name = "");
	virtual void		initialize				();
	virtual void		execute					();
};

//////////////////////////////////////////////////////////////////////////
// CObjectActionIdle
//////////////////////////////////////////////////////////////////////////

class CObjectActionIdle : public CObjectActionBase<CInventoryItem> {
protected:
	typedef CObjectActionBase<CInventoryItem> inherited;

public:
						CObjectActionIdle		(CInventoryItem *item, CAI_Stalker *owner, CPropertyStorage *storage, LPCSTR action_name = "");
	virtual void		initialize				();
};

//////////////////////////////////////////////////////////////////////////
// CObjectActionIdleMissile
//////////////////////////////////////////////////////////////////////////

class CObjectActionIdleMissile : public CObjectActionBase<CInventoryItem> {
protected:
	typedef CObjectActionBase<CInventoryItem> inherited;

public:
						CObjectActionIdleMissile(CInventoryItem *item, CAI_Stalker *owner, CPropertyStorage *storage, LPCSTR action_name = "");
	virtual void		initialize				();
};

//////////////////////////////////////////////////////////////////////////
// CObjectActionThrowMissile
//////////////////////////////////////////////////////////////////////////

class CObjectActionThrowMissile : public CObjectActionBase<CInventoryItem> {
protected:
	typedef CObjectActionBase<CInventoryItem> inherited;

public:
						CObjectActionThrowMissile	(CInventoryItem *item, CAI_Stalker *owner, CPropertyStorage *storage, LPCSTR action_name = "");
	virtual void		initialize				();
	virtual void		execute					();
};
#include "object_actions_inline.h"