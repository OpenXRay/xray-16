//---------------------------------------------------------------------------
#ifndef SHToolsInterfaceH
#define SHToolsInterfaceH

#include "ElTree.hpp"
#include "MxMenus.hpp"
#include "ElPgCtl.hpp"
#include "../xrEprops/PropertiesList.h"

// refs

enum EToolsID{
	aeFirstTool = 0,
	aeEngine = 0,
    aeCompiler,
    aeMtl,
    aeMtlPair,
    aeSoundEnv,
    aeMaxTools
};

struct ISHInit
{
    EToolsID				tools_id;
	TItemList*				m_Items;
    TProperties*			m_ItemProps;
    TProperties*			m_PreviewProps;
    TElTabSheet*			m_Sheet;
							ISHInit(){ZeroMemory(this,sizeof(ISHInit));}
							ISHInit(EToolsID id, TItemList* il, TElTabSheet* sh, TProperties* ip, TProperties* pp)
    {
        tools_id			= id;
        m_Items				= il;
        m_ItemProps			= ip;
        m_PreviewProps		= pp;
        m_Sheet				= sh;
    }
};
class ISHTools
{
protected:
	typedef	ISHTools 		inherited;

	ISHInit					Ext;

	BOOL					m_bModified;

    BOOL					m_bLockUpdate;		// если менялся объект непосредственно  Update____From___()

    AnsiString				m_LastSelection;
public:
	ListItem* 				m_CurrentItem;
public:
    void					ViewSetCurrentItem	(LPCSTR full_name);
    AnsiString				ViewGetCurrentItem	(bool bFolderOnly);
    TElTreeItem*			ViewGetCurrentItem	();
public:
    virtual LPCSTR			AppendItem			(LPCSTR folder_name, LPCSTR parent=0)=0; 
	virtual void			FillItemList		()=0;
public:
							ISHTools 			(ISHInit& init);
    virtual 				~ISHTools			(){;}

    EToolsID				ID					(){return Ext.tools_id;}
    TElTabSheet*			Sheet				(){return Ext.m_Sheet;}
    AnsiString				SelectedName		();
    void					RemoveCurrent		();
    void					RenameCurrent		();

    virtual LPCSTR			ToolsName			()=0;
    
	virtual void			Reload				()=0;
	virtual void			Load				()=0;
	virtual bool			Save				()=0;

    bool					IsModified			(){return m_bModified;}
    virtual bool			IfModified			();
    virtual void __stdcall	Modified			();

    virtual bool			OnCreate			()=0;
    virtual void			OnDestroy			()=0;
	virtual void 			OnActivate			()=0;
	virtual void 			OnDeactivate		()=0;

    // misc
    virtual void			ResetCurrentItem	()=0;
    virtual void			SetCurrentItem		(LPCSTR name, bool bView)=0;
    virtual void			ApplyChanges		(bool bForced=false)=0;

	virtual void 			RealUpdateProperties()=0;
	virtual void 			RealUpdateList		()=0;

	virtual void 			OnFrame				()=0;
	virtual void 			OnRender			()=0;

    virtual void			OnDeviceCreate		()=0;
    virtual void			OnDeviceDestroy		()=0;

    virtual void			ZoomObject			(bool bOnlySel);
	virtual void 			OnShowHint			(AStringVec& ss){;}
};
//---------------------------------------------------------------------------
#endif
