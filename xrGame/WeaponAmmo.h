#pragma once
#include "inventory_item_object.h"
#include "anticheat_dumpable_object.h"

struct SCartridgeParam
{
	float	kDist, kDisp, kHit/*, kCritical*/, kImpulse, kAP, kAirRes;
	int		buckShot;
	float	impair;
	float	fWallmarkSize;
	u8		u8ColorID;

	IC void Init()
	{
		kDist = kDisp = kHit = kImpulse = 1.0f;
//		kCritical = 0.0f;
		kAP       = 0.0f;
		kAirRes   = 0.0f;
		buckShot  = 1;
		impair    = 1.0f;
		fWallmarkSize = 0.0f;
		u8ColorID     = 0;
	}
};

class CCartridge : public IAnticheatDumpable
{
public:
	CCartridge();
	void Load(LPCSTR section, u8 LocalAmmoType);

	shared_str	m_ammoSect;
	enum{
		cfTracer				= (1<<0),
		cfRicochet				= (1<<1),
		cfCanBeUnlimited		= (1<<2),
		cfExplosive				= (1<<3),
		cfMagneticBeam			= (1<<4),
	};
	SCartridgeParam param_s;

	u8		m_LocalAmmoType;

	u16		bullet_material_idx;
	Flags8	m_flags;

	shared_str	m_InvShortName;
	virtual void				DumpActiveParams		(shared_str const & section_name, CInifile & dst_ini) const;
	virtual shared_str const 	GetAnticheatSectionName	() const { return m_ammoSect; };
};

class CWeaponAmmo :	
	public CInventoryItemObject {
	typedef CInventoryItemObject		inherited;
public:
									CWeaponAmmo			(void);
	virtual							~CWeaponAmmo		(void);

	virtual CWeaponAmmo				*cast_weapon_ammo	()	{return this;}
	virtual void					Load				(LPCSTR section);
	virtual BOOL					net_Spawn			(CSE_Abstract* DC);
	virtual void					net_Destroy			();
	virtual void					net_Export			(NET_Packet& P);
	virtual void					net_Import			(NET_Packet& P);
	virtual void					OnH_B_Chield		();
	virtual void					OnH_B_Independent	(bool just_before_destroy);
	virtual void					UpdateCL			();
	virtual void					renderable_Render	();

	virtual bool					Useful				() const;
	virtual float					Weight				() const;
	virtual	u32						Cost				() const;

	bool							Get					(CCartridge &cartridge);

	SCartridgeParam cartridge_param;

	u16			m_boxSize;
	u16			m_boxCurr;
	bool		m_tracer;

public:
	virtual CInventoryItem *can_make_killing	(const CInventory *inventory) const;
};
