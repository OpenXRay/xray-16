//---------------------------------------------------------------------------
#ifndef SHGameMtlToolsH
#define SHGameMtlToolsH

#include "SHToolsInterface.h"
#include "GameMtlLib.h"

// refs
class PropValue;

class CSHGameMtlTools : public ISHTools
{
	BOOL m_CreatingMtl;
	xr_string m_CreatingMtlPath;
	ISHTools *m_GameMtlPairTools;
	void ItemExist(LPCSTR name, bool &res) { res = !!FindItem(name); }
	SGameMtl *FindItem(LPCSTR name);

public:
	SGameMtl *m_Mtl;
	virtual void AppendItem(LPCSTR path, LPCSTR parent_name = 0);
	virtual void AppendItem(LPCSTR path, bool dynamic = false, SGameMtl *parent = 0);
	virtual void OnRemoveItem(LPCSTR name, EItemType type);
	virtual void OnRenameItem(LPCSTR old_full_name, LPCSTR new_full_name, EItemType type);
	void FillChooseMtlType(ChooseItemVec &items, void *param);

	virtual void FillItemList();

public:
	CSHGameMtlTools(const ISHInit &init);
	virtual ~CSHGameMtlTools();

	virtual LPCSTR ToolsName() { return "Game Materials"; }

	virtual void Reload();
	virtual void Load();
	virtual bool Save();
	virtual void ApplyChanges(bool bForced = false);

	virtual bool OnCreate();
	virtual void OnDestroy();
	virtual void OnActivate();
	virtual void OnDeactivate();

	// misc
	virtual void ResetCurrentItem();
	virtual void SetCurrentItem(LPCSTR name, bool bView);

	virtual void RealUpdateProperties();
	virtual void RealUpdateList();

	virtual void OnFrame();
	virtual void OnRender() { ; }

	virtual void OnDeviceCreate() { ; }
	virtual void OnDeviceDestroy() { ; }

	virtual void OnDrawUI();
};
//---------------------------------------------------------------------------
#endif // SHGameMtlToolsH
