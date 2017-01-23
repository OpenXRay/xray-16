// file: MeshExpUtility.h


#ifndef __MeshExpUtility__H__INCLUDED__
#define __MeshExpUtility__H__INCLUDED__

#include "NetDeviceLog.h"
#include "MeshExpUtility.rh"

#define EXPORTER_VERSION	2
#define EXPORTER_BUILD		03
//using namespace std;

// refs
class CEditableObject;

#define	EXP_UTILITY_CLASSID 0x507d29c0

class ExportItem {
public:
	INode *pNode;
	ExportItem(){ pNode = 0;};
	ExportItem( INode* _pNode ){ pNode = _pNode; };
	~ExportItem(){};
};

DEFINE_VECTOR(ExportItem,ExportItemVec,ExportItemIt);
class MeshExpUtility : public UtilityObj {
public:

	IUtil		*iu;
	Interface	*ip;

	HWND		hPanel;
	HWND		hItemList;

	INode*		GetExportNode	();
protected:
	ExportItemVec m_Items;

	void		RefreshExportList();
	void		UpdateSelectionListBox();

	BOOL		BuildObject		(CEditableObject*& obj, LPCSTR m_ExportName);
	BOOL		SaveAsObject	(const char* n);
	BOOL		SaveAsLWO		(const char* n);
	BOOL		SaveAsSkin		(const char* n);
	BOOL		SaveSkinKeys	(const char* n);
public:
	int			m_ObjectFlipFaces;
	int			m_SkinFlipFaces;
	int			m_SkinAllowDummy;
public:
				MeshExpUtility	();
	virtual		~MeshExpUtility	();

	void		BeginEditParams	(Interface *ip,IUtil *iu);
	void		EndEditParams	(Interface *ip,IUtil *iu);
	void		SelectionSetChanged(Interface *ip,IUtil *iu);
	void		DeleteThis		() {}

	void		Init			(HWND hWnd);
	void		Destroy			(HWND hWnd);
	void		ExportObject	();
	void		ExportLWO		();
	void		ExportSkin		();
	void		ExportSkinKeys	();
};

extern MeshExpUtility U;



#endif /*__MeshExpUtility__H__INCLUDED__*/


