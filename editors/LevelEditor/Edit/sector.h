#ifndef SectorH
#define SectorH

#include "portalutils.h"
#include "CustomObject.h"
enum EVisible{
	fvNone,
    fvPartialInside,
    fvPartialOutside,
    fvFully
};

#define DEFAULT_SECTOR_NAME "Sector_DEFAULT"
// refs
class CEditableMesh;

class CSectorItem{
	friend class CSector;
	friend class SceneBuilder;
    friend class CPortalUtils;
	CSceneObject* object;
	CEditableMesh* mesh;
public:
    CSectorItem		();
    CSectorItem		(CSceneObject* o, CEditableMesh* m);
    void GetTransform(Fmatrix& parent);
    bool IsItem		(const char* O, const char* M);
    IC bool IsItem	(CSceneObject* o, CEditableMesh* m){ return (o==object)&&(m==mesh); }
};

DEFINE_VECTOR(CSectorItem,SItemVec,SItemIt);

class CSector : public CCustomObject {
	friend class TfrmPropertiesSector;
	friend class SceneBuilder;
    friend class CPortalUtils;
    friend class CPortal;
	typedef CCustomObject inherited;

    u8				m_bDefault;
    bool			m_bHasLoadError;

    Fbox			m_SectorBox;

    Fvector 		m_SectorCenter;
    float 			m_SectorRadius;

	Fcolor			sector_color;
	SItemVec 		sector_items;
	bool 			FindSectorItem		(CSceneObject* o, CEditableMesh* m, SItemIt& it);
	bool 			FindSectorItem		(const char* O, const char* M, SItemIt& it);
	void 			LoadSectorDef		( IReader* F );
    void 			LoadSectorDefLTX	( CInifile& ini, LPCSTR sect_name, u32 item_idx );
    enum{
    	flNeedUpdateVolume = (1<<0)
    };

    Flags32			m_Flags;

    // only for build
    int				m_sector_num;
	void 			OnDestroy	();
    void			UpdateVolume();
public:
	u8    			m_map_idx;

					CSector		(LPVOID data, LPCSTR name);
	void 			Construct	(LPVOID data);
	virtual 		~CSector	();
    virtual bool	CanAttach		() {return false;}

    virtual void 	Render		(int priority, bool strictB2F);
	virtual bool 	RayPick 	(float& distance,const Fvector& start,const Fvector& direction,
								SRayPickInfo* pinf = NULL);
    virtual bool 	FrustumPick	(const CFrustum& frustum);
    virtual bool 	SpherePick	(const Fvector& center, float radius);
	virtual bool 	GetBox		(Fbox& box) const;
	virtual void 	Move		( Fvector& amount ); // need for Shift Level
	virtual void 	OnSceneUpdate();

    // file system function
	virtual bool 	LoadStream		(IReader&);
	virtual bool 	LoadLTX			(CInifile& ini, LPCSTR sect_name);
	virtual void 	SaveStream			(IWriter&);
	virtual void 	SaveLTX			(CInifile& ini, LPCSTR sect_name);
	virtual void	FillProp		(LPCSTR pref, PropItemVec& values);
	virtual bool 	GetSummaryInfo	(SSceneSummary* inf);

	bool			AddMesh		(CSceneObject* O, CEditableMesh* M); // возвращает добавлен ли объект
	int		  		DelMesh		(CSceneObject* O, CEditableMesh* M); // 0-не удален 1-удален 2-удален сектор вообще

    bool			IsDefault	(){return m_bDefault;}
    bool			Contains	(CSceneObject* O, CEditableMesh* M){SItemIt it; return FindSectorItem(O,M,it);}

    void 			SetColor 	(u32 clr){sector_color.set(subst_alpha(clr,0));}
    
    void			CaptureInsideVolume();
	void 			DistributeInsideObjects	();
    void			CaptureAllUnusedMeshes	();
    void			GetCounts	(int* objects, int* meshes, int* faces);
    bool			IsEmpty		();

	virtual void 	OnFrame		();

    EVisible		Intersect	(const Fvector& center, float radius);
	EVisible 		Intersect	(const Fbox& box);

    virtual bool	Validate	(bool bMsg);
};

#endif /*_INCDEF_Sector_H_*/

