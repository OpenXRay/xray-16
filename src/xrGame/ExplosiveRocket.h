//////////////////////////////////////////////////////////////////////
// ExplosiveRocket.h:	ракета, которой стреляет RocketLauncher 
//						взрывается при столкновении
//////////////////////////////////////////////////////////////////////

#pragma once

#include "CustomRocket.h"
#include "Explosive.h"
#include "inventory_item.h"

class CExplosiveRocket : 
			public CCustomRocket,
			public CInventoryItem,
			public CExplosive
{
private:
	typedef CCustomRocket inherited;
	friend CRocketLauncher;
public:
	CExplosiveRocket(void);
	virtual ~CExplosiveRocket(void);
	virtual DLL_Pure	*_construct	();
public:
	virtual CExplosive					*cast_explosive			()						{return this;}
	virtual CInventoryItem				*cast_inventory_item	()						{return this;}
	virtual CAttachableItem				*cast_attachable_item	()						{return this;}
	virtual CWeapon						*cast_weapon			()						{return NULL;}
	virtual CGameObject					*cast_game_object		()						{return this;}
	virtual IDamageSource*				cast_IDamageSource()							{return CExplosive::cast_IDamageSource();}
	virtual void						on_activate_physic_shell();
public:

	virtual void Load				(LPCSTR section);
	virtual BOOL net_Spawn			(CSE_Abstract* DC);
	virtual void net_Destroy		();
	virtual	void net_Relcase		(CObject* O );
	virtual void OnH_A_Independent	();
	virtual void OnH_B_Independent	(bool just_before_destroy);
	virtual void UpdateCL			();

	virtual void Contact(const Fvector &pos, const Fvector &normal);

	virtual void OnEvent (NET_Packet& P, u16 type) ;

	virtual	void Hit	(SHit* pHDS)
						{ inherited::Hit(pHDS); };

public:
	virtual BOOL			UsedAI_Locations	()				{return inherited::UsedAI_Locations();}
	virtual void			net_Import			(NET_Packet& P)	{inherited::net_Import(P);}
	virtual void			net_Export			(NET_Packet& P) {inherited::net_Export(P);}
	
	virtual void			save				(NET_Packet &output_packet) {inherited::save(output_packet);}
	virtual void			load				(IReader &input_packet)		{inherited::load(input_packet);}
	virtual BOOL			net_SaveRelevant	()							{return inherited::net_SaveRelevant();}

	virtual void			OnH_A_Chield		()				{inherited::OnH_A_Chield();}
	virtual void			OnH_B_Chield		()				{inherited::OnH_B_Chield();}
	virtual void			renderable_Render	()				{inherited::renderable_Render();}
	virtual void			make_Interpolation	();
	virtual void			PH_B_CrPr			(); // actions & operations before physic correction-prediction steps
	virtual void			PH_I_CrPr			(); // actions & operations after correction before prediction steps
#ifdef DEBUG
	virtual void			PH_Ch_CrPr			(); // 
	virtual void			OnRender			();
#endif
	virtual void			PH_A_CrPr			(); // actions & operations after phisic correction-prediction steps
	virtual void			reinit				();
	virtual void			reload				(LPCSTR section);
	virtual void			activate_physic_shell	();
	virtual void			setup_physic_shell		();
	virtual void			create_physic_shell		();

public:
	virtual bool			Useful				() const;
protected:
	virtual bool	use_parent_ai_locations	() const
	{
		return CAttachableItem::use_parent_ai_locations();
	}
};