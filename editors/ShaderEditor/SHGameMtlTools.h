//---------------------------------------------------------------------------
#ifndef SHGameMtlToolsH
#define SHGameMtlToolsH

#include "SHToolsInterface.h"
#include "GameMtlLib.h"

// refs
class PropValue;

class CSHGameMtlTools: public ISHTools
{
	ISHTools*				m_GameMtlPairTools;
	void __stdcall 			ItemExist			(LPCSTR name, bool& res){res = !!FindItem(name);}
	SGameMtl*				FindItem			(LPCSTR name);
public:
    SGameMtl*				m_Mtl;
    virtual LPCSTR			AppendItem			(LPCSTR folder_name, LPCSTR parent_name=0);  
    virtual void __stdcall  OnRemoveItem		(LPCSTR name, EItemType type, bool& res); 
	virtual void __stdcall  OnRenameItem		(LPCSTR old_full_name, LPCSTR new_full_name, EItemType type);
	void __stdcall  		FillChooseMtlType	(ChooseItemVec& items, void* param);

    virtual void			FillItemList		();
public:
							CSHGameMtlTools 	(ISHInit& init);
    virtual 				~CSHGameMtlTools	();

    virtual LPCSTR			ToolsName			(){return "Game Materials";}

	virtual void			Reload				();
	virtual void			Load				();
	virtual bool			Save				();
    virtual void			ApplyChanges		(bool bForced=false);

    virtual bool			OnCreate			();
    virtual void			OnDestroy			();
	virtual void 			OnActivate			();
	virtual void 			OnDeactivate		();

    // misc
    virtual void			ResetCurrentItem	();
    virtual void			SetCurrentItem		(LPCSTR name, bool bView);

	virtual void 			RealUpdateProperties();
	virtual void 			RealUpdateList		();

	virtual void 			OnFrame				();
	virtual void 			OnRender			(){;}

    virtual void			OnDeviceCreate		(){;}
    virtual void			OnDeviceDestroy		(){;}
};
//---------------------------------------------------------------------------
#endif // SHGameMtlToolsH
