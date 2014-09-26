//---------------------------------------------------------------------------
#ifndef ESceneGroupToolsH
#define ESceneGroupToolsH

#include "ESceneCustomOTools.h"

class ESceneGroupTool: public ESceneCustomOTool
{
	typedef ESceneCustomOTool inherited;
    xr_string			m_CurrentObject;
    xr_vector<bool>		m_stored_state;
protected:
    // controls
    virtual void 		CreateControls			();
	virtual void 		RemoveControls			();
public:
				ESceneGroupTool		        ():ESceneCustomOTool(OBJCLASS_GROUP){;}
	// definition
    IC LPCSTR			ClassName			(){return "group";}
    IC LPCSTR			ClassDesc			(){return "Group";}
    IC int			RenderPriority			(){return 1;}

    virtual void		Clear				(bool bSpecific=false){inherited::Clear(bSpecific);}
    // IO
    virtual int			SaveFileCount		   () const {return 2;}
    virtual bool   		IsNeedSave			(){return inherited::IsNeedSave();}
    virtual bool   		LoadStream           (IReader&);
    virtual bool   		LoadLTX            	 (CInifile&);
    virtual void   		SaveStream           (IWriter&);
	virtual void   		SaveLTX            	 (CInifile&, int id);
    virtual bool		LoadSelection      	 (IReader&);
    virtual void		SaveSelection      	 (IWriter&);

    virtual void 		OnActivate			 ();

    // group function
    void			 UngroupObjects			(bool bUndo=true);
    void 			 GroupObjects			(bool bUndo=true);

    void 			 CenterToGroup			();
    void 			 AlignToObject			();

    void 			 MakeThumbnail			();

    void 			 SaveSelectedObject		();
    void 			 ReloadRefsSelectedObject();
    void 			 SetCurrentObject		(LPCSTR nm);
    LPCSTR			 GetCurrentObject		(){return m_CurrentObject.c_str();}

    virtual CCustomObject*      CreateObject			(LPVOID data, LPCSTR name);
    virtual BOOL 		_RemoveObject			(CCustomObject* object);

};
//---------------------------------------------------------------------------
#endif
