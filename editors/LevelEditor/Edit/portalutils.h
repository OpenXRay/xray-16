#ifndef PortalUtilsH
#define PortalUtilsH

#include "ESceneClassList.h"

//refs
class CEditableMesh;
class CEditableObject;
class CSceneObject;
class CSector;

struct SVertex: public Fvector{
	IntVec link[2];
    IntVec ulink;
    int portal;
    SVertex(const Fvector& v){set(v);portal=-1;}
    IC void SetLink(int id, int i0, int i1){if (i0>=0)link[id].push_back(i0); if (i1>=0)link[id].push_back(i1);}
    IC void ConsolidateLink(){
    	std::sort(link[0].begin(),link[0].end());
    	std::sort(link[1].begin(),link[1].end());
        std::set_intersection(	link[0].begin(),link[0].end(),
        						link[1].begin(),link[1].end(),
                	        	inserter(ulink,ulink.begin()));
        VERIFY(ulink.size()<=2);
    }
};

DEFINE_VECTOR(SVertex,SVertexVec,SVertexIt)

class CPortalUtils{
//	void FindSVertexLinks(int id, CSector* S, SVertexVec& V);
	int  CalculateSelectedPortals(ObjectList& sectors);
public:
	bool CalculateConvexHull(FvectorVec& points);
	int  CalculatePortals(CSector* SF, CSector* SB);
	int  CalculateSelectedPortals();
	int  CalculateAllPortals();
//.	int  CalculateAllPortals2();
	void RemoveAllPortals();
	void RemoveSectorPortal(CSector* S);
	bool CreateDefaultSector();
	bool RemoveDefaultSector();

	CSector* FindSector(CSceneObject* o, CEditableMesh* m);

//	void CreateDebugCollection();
    bool Validate(bool bMsg);

    CSector* GetSelectedSector();
};

extern CPortalUtils PortalUtils;

#endif /*_INCDEF_PortalUtils_H_*/
