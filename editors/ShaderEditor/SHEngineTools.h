//---------------------------------------------------------------------------
#ifndef SHEngineToolsH    
#define SHEngineToolsH

#include "SHToolsInterface.h"
#include "../xrEProps/PropertiesList.h"

DEFINE_VECTOR(IBlender*,TemplateVec,TemplateIt);
DEFINE_MAP_PRED(LPSTR,CConstant*,ConstantMap,ConstantPairIt,str_pred);
DEFINE_MAP_PRED(LPSTR,CMatrix*,MatrixMap,MatrixPairIt,str_pred);
DEFINE_MAP_PRED(LPSTR,IBlender*,BlenderMap,BlenderPairIt,str_pred);

// refs
class CSHEngineTools;
class CEditableObject;

class CParseBlender{
public:
	virtual void Parse(CSHEngineTools* owner, DWORD type, LPCSTR key, LPVOID data)=0;
};

enum EPreviewObj{
	pvoNone,
	pvoPlane,
    pvoBox,
    pvoSphere,
    pvoTeapot,
    pvoCustom,
    pvo_force_dword = u32(-1)
};

class CSHEngineTools: public ISHTools
{
	SStringVec				MCString;

	u32						m_PreviewObjectType;
	CEditableObject*		m_PreviewObject;
    bool					m_bCustomEditObject;

	BOOL					m_bFreezeUpdate;
    BOOL					m_bNeedResetShaders;
    BOOL					m_RemoteRenBlender;
    AnsiString				m_RenBlenderOldName;
    AnsiString				m_RenBlenderNewName;

	TemplateVec				m_TemplatePalette;

	ConstantMap				m_OptConstants;
	MatrixMap				m_OptMatrices;
	ConstantMap				m_Constants;
	MatrixMap				m_Matrices;
	BlenderMap				m_Blenders;

	void __stdcall 			ItemExist			(LPCSTR name, bool& res){res = !!FindItem(name);}
	IBlender*				FindItem			(LPCSTR name);

	void 					AddMatrixRef		(LPSTR name);
	CMatrix*				FindMatrix			(LPCSTR name);
	CMatrix*				AppendMatrix		(LPSTR name);
    LPCSTR					GenerateMatrixName	(LPSTR name);
    LPCSTR					AppendMatrix		(CMatrix* src=0, CMatrix** dest=0);
    void					RemoveMatrix		(LPCSTR name);   

	void 					AddConstantRef		(LPSTR name);
	CConstant*				FindConstant		(LPCSTR name);
	CConstant*				AppendConstant		(LPSTR name);
    LPCSTR					GenerateConstantName(LPSTR name);
    LPCSTR					AppendConstant		(CConstant* src=0, CConstant** dest=0);
    void					RemoveConstant		(LPCSTR name);

friend class CCollapseBlender;
friend class CRefsBlender;
friend class CRemoveBlender;
friend class TfrmShaderProperties;
    void					CollapseMatrix		(LPSTR name);
    void					CollapseConstant	(LPSTR name);
    void					CollapseReferences	();
    void					UpdateMatrixRefs	(LPSTR name);
    void					UpdateConstantRefs	(LPSTR name);
    void					UpdateRefCounters	();

    void 					ParseBlender		(IBlender* B, CParseBlender& P);

	CMemoryWriter			m_BlenderStream;	// пользоваться функциями обновления стрима для синхронизации
    bool 					m_bUpdateCurrent;	// если менялся объект непосредственно  Update____From___()
    bool					m_bCurBlenderChanged;

    void 					Save				(CMemoryWriter& F);
    void 					PrepareRender		();

    // template
	void __stdcall  		FillChooseTemplate	(ChooseItemVec& items, void* param);
    // matrix props                                                
	bool __stdcall  		MatrixOnAfterEdit	(PropValue* sender, xr_string& edit_val);
	void __stdcall  		FillMatrixProps		(PropItemVec& items, LPCSTR pref, LPSTR name);
	void __stdcall  		MCOnDraw			(PropValue* sender, xr_string& draw_val);
    // constant props
	bool __stdcall  		ConstOnAfterEdit	(PropValue* sender, xr_string& edit_val);
	void __stdcall  		FillConstProps		(PropItemVec& items, LPCSTR pref, LPSTR name);
    // name                                 
	bool __stdcall  		NameOnAfterEdit		(PropValue* sender, xr_string& edit_val);

    void					RealResetShaders	();

	void __stdcall  		FillMatrix			(PropItemVec& values, LPCSTR pref, CMatrix* m);
	void __stdcall  		FillConst			(PropItemVec& values, LPCSTR pref, CConstant* c);
    void __stdcall 			RefreshProperties	();

    void					ResetShaders		(bool bForced=false){m_bNeedResetShaders=true; if (bForced) RealResetShaders(); }
    void					UpdateObjectShader	();

    bool __stdcall 			OnPreviewObjectRefChange(PropValue* sender, u32& edit_val); 
public:
	CMemoryWriter			m_RenderShaders;

    IBlender*				m_CurrentBlender;
	void					RemoteRenameBlender	(LPCSTR old_full_name, LPCSTR new_full_name){m_RemoteRenBlender=TRUE;m_RenBlenderOldName=old_full_name;m_RenBlenderNewName=new_full_name;}

    Shader_xrLC*			m_Shader;
    virtual LPCSTR			AppendItem			(LPCSTR folder_name, LPCSTR parent=0);        
	virtual void			RealRenameItem		(LPCSTR old_full_name, LPCSTR new_full_name);
    virtual void __stdcall  OnRemoveItem		(LPCSTR name, EItemType type, bool& res); 
	virtual void __stdcall  OnRenameItem		(LPCSTR old_full_name, LPCSTR new_full_name, EItemType type);
	virtual void			FillItemList		();

    void					UpdateStreamFromObject();
    void					UpdateObjectFromStream();

    void 					ClearData			();
public:
							CSHEngineTools		(ISHInit& init);
    virtual 				~CSHEngineTools		();

    virtual LPCSTR			ToolsName			(){return "Engine Shader";}

	virtual void			Reload				();
	virtual void			Load				();
	virtual bool			Save				();

    virtual bool			OnCreate			();
    virtual void			OnDestroy			();
    virtual void			OnActivate			();
    virtual void			OnDeactivate		();

    // misc
    virtual void			ResetCurrentItem	();
    virtual void			SetCurrentItem		(LPCSTR name, bool bView);
    virtual void			ApplyChanges		(bool bForced=false);

	virtual void 			RealUpdateProperties();
	virtual void 			RealUpdateList		();

    virtual void			OnFrame				();
	virtual void 			OnRender			();

    virtual void			OnDeviceCreate		();
    virtual void			OnDeviceDestroy		(){;}

    virtual void			ZoomObject			(bool bOnlySel);
	virtual void 			OnShowHint			(AStringVec& ss);
};
//---------------------------------------------------------------------------
#endif
