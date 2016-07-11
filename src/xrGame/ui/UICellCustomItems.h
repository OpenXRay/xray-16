#pragma once
#include "UICellItem.h"
#include "../Weapon.h"


struct SIconLayer
{
	LPCSTR m_name;
	CUIStatic* m_icon;
	Fvector2 offset;
	//u32 m_color;
	float m_scale;
};

class CUIInventoryCellItem :public CUICellItem
{
	typedef  CUICellItem	inherited;
public:
											CUIInventoryCellItem		(CInventoryItem* itm);
	virtual		bool						EqualTo						(CUICellItem* itm);
	virtual		void						UpdateItemText				();
				CUIDragItem*				CreateDragItem				();
	virtual		bool						IsHelper					();
	virtual		void						SetIsHelper					(bool is_helper);
				bool						IsHelperOrHasHelperChild	();
				void						Update						();
				CInventoryItem*				object						() {return (CInventoryItem*)m_pData;}
				//Alundaio
				virtual		void			OnAfterChild(CUIDragDropListEx* parent_list);
				virtual		void			SetTextureColor(u32 color);

				xr_vector<SIconLayer*>		m_layers;
				void						RemoveLayer					(SIconLayer* layer);
				void						CreateLayer					(LPCSTR name, Fvector2 offset, float scale);
				CUIStatic*					InitLayer					(CUIStatic* s, LPCSTR section, Fvector2 addon_offset, bool b_rotate, float scale);
				//-Alundaio
};

class CUIAmmoCellItem :public CUIInventoryCellItem
{
	typedef  CUIInventoryCellItem	inherited;
protected:
	virtual		void			 UpdateItemText				();
public:
								 CUIAmmoCellItem			(CWeaponAmmo* itm);

				u32				 CalculateAmmoCount			();
	virtual		bool			 EqualTo						(CUICellItem* itm);
	virtual		CUIDragItem*	 CreateDragItem				();
				CWeaponAmmo*	 object						() {return (CWeaponAmmo*)m_pData;}
};

class CUIWeaponCellItem :public CUIInventoryCellItem
{
	typedef  CUIInventoryCellItem	inherited;
public:
	enum eAddonType{	eSilencer=0, eScope, eLauncher, eMaxAddon};
protected:
	CUIStatic*					m_addons					[eMaxAddon];
	Fvector2					m_addon_offset				[eMaxAddon];
	void						CreateIcon					(eAddonType);
	void						DestroyIcon					(eAddonType);
	void						RefreshOffset				();
	CUIStatic*					GetIcon						(eAddonType);
	void						InitAddon					(CUIStatic* s, LPCSTR section, Fvector2 offset, bool use_heading);
	bool						is_scope					();
	bool						is_silencer					();
	bool						is_launcher					();
public:
								CUIWeaponCellItem			(CWeapon* itm);
				virtual			~CUIWeaponCellItem			();
	virtual		void			Update						();
	virtual		void			Draw						();
	virtual		void			SetTextureColor				(u32 color);

				CWeapon*		object						() {return (CWeapon*)m_pData;}
	virtual		void			OnAfterChild				(CUIDragDropListEx* parent_list);
	virtual		CUIDragItem*	CreateDragItem				();
	virtual		bool			EqualTo						(CUICellItem* itm);
	CUIStatic*					get_addon_static			(u32 idx)				{return m_addons[idx];}
};

class CBuyItemCustomDrawCell :public ICustomDrawCellItem
{
	CGameFont*			m_pFont;
	string16			m_string;
public:
						CBuyItemCustomDrawCell	(LPCSTR str, CGameFont* pFont);
	virtual void		OnDraw					(CUICellItem* cell);

};
