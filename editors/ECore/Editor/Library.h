//----------------------------------------------------
// file: Library.h
//----------------------------------------------------

#ifndef LibraryH
#define LibraryH

#include "pure.h"
#include "../xrEProps/FolderLib.h"              
//----------------------------------------------------
class CEditableObject;

DEFINE_MAP_PRED(AnsiString,CEditableObject*,EditObjMap,EditObjPairIt,astr_pred);
//----------------------------------------------------
class ECORE_API ELibrary//:	public pureDeviceCreate, public pureDeviceDestroy
{
	bool				m_bReady;
	friend class TfrmChoseObject;
	EditObjMap			m_EditObjects;

    CEditableObject*	LoadEditObject		(LPCSTR full_name);
    void				UnloadEditObject	(LPCSTR full_name);
public:
						ELibrary			();
	virtual 			~ELibrary			();
                    
    void __stdcall 		RemoveObject		(LPCSTR fname, EItemType type, bool& res);
    void __stdcall 		RenameObject		(LPCSTR fn0, LPCSTR fn1, EItemType type);

	void 				OnCreate			();
	void 				OnDestroy			();
	void 				Save				(FS_FileSet* modif_map=0);

    void 				ReloadObjects		();
    void 				CleanLibrary		();
    void 				ReloadObject		(LPCSTR name);

    CEditableObject*	CreateEditObject	(LPCSTR name);
    void				RemoveEditObject	(CEditableObject*& object);

    int					GetObjects			(FS_FileSet& files);
    int					ObjectCount			(){return m_EditObjects.size();}

    void				EvictObjects		();

	virtual		void	OnDeviceCreate		();
	virtual		void	OnDeviceDestroy		();
};

extern ECORE_API ELibrary Lib;
//----------------------------------------------------
#endif /*_INCDEF_Library_H_*/

