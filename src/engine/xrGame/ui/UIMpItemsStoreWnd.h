#pragma once
#include "../../xrServerEntities/object_interfaces.h"
#include "UIBuyWndShared.h"

class CUIXml;
class CUITabButtonMP;
class CUICellItem;

class CStoreHierarchy
{
public:
	struct item :public IPureDestroyableObject
	{
								item				():m_parent(NULL),m_button(NULL){}
	virtual void				destroy				();

		shared_str				m_name;
		shared_str				m_btn_xml_name; //debug
		item*					m_parent;
		xr_vector<item*>		m_childs;
		xr_vector<shared_str>	m_items_in_group;
		CUITabButtonMP*			m_button;
		IC u32					ChildCount			()								const		{return m_childs.size();}
		IC const item&			Child				(const shared_str& id)			const;
		IC const item&			ChildAtIdx			(u32 idx)						const		{VERIFY(idx<=ChildCount()); return *m_childs[idx];};
		IC bool					HasSubLevels		()								const		{return ChildCount()!=0;}
		bool					HasItem				(const shared_str& name_sect)	const;
		int						GetItemIdx			(const shared_str& name_sect)	const;
	};

private:
	const item*				m_current_level;
	item*					m_root;
	int						m_team_idx;
	void					LoadLevel			(CUIXml& xml, int index, item* _itm, int depth_level);
public:
							CStoreHierarchy		();
							~CStoreHierarchy	();
					
	void					Init				(CUIXml& xml, LPCSTR path);
	void					InitItemsInGroup	(const shared_str& sect, item* =NULL);
	const item&				GetRoot				()								{VERIFY(m_root); return *m_root;};
	void					Reset				()								{VERIFY(m_root); m_current_level = m_root;};
	IC bool					CurrentIsRoot		()								{return m_current_level == m_root;}

	const item&				CurrentLevel		()								{VERIFY(m_current_level); return *m_current_level;};
	bool					MoveUp				();
	bool					MoveDown			(const shared_str& name);
	item*					FindItem			(const shared_str& name_sect, item* recurse_from=NULL);
	int						TeamIdx				() const						{return m_team_idx;}
};
