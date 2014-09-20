//---------------------------------------------------------------------------
#ifndef SHGameMtlPairToolsH
#define SHGameMtlPairToolsH

#include "SHToolsInterface.h"
#include "GameMtlLib.h"

// refs
class PropValue;

class CSHGameMtlPairTools: public ISHTools
{
	ISHTools*				m_GameMtlTools;
    u32						m_StoreFlags;
public:
    SGameMtlPair*			m_MtlPair;                                
    virtual LPCSTR			AppendItem			(LPCSTR folder_name, LPCSTR parent_name=0){return 0;}
	virtual void 			FillItemList		();
public:
							CSHGameMtlPairTools (ISHInit& init);
    virtual 				~CSHGameMtlPairTools();

    virtual LPCSTR			ToolsName			(){return "Game Material Pairs";}

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
    virtual void			OnRender			(){;}

    virtual void			OnDeviceCreate		(){;}
    virtual void			OnDeviceDestroy		(){;}
};
//---------------------------------------------------------------------------
#endif // SHGameMaterialToolsH
