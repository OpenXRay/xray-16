//----------------------------------------------------
// file: PortalUtils.cpp
//----------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "portalutils.h"
#include "Scene.h"
#include "Portal.h"
#include "Sector.h"
#include "../ECore/Editor/EditMesh.h"
#include "../ECore/Editor/EditObject.h"
#include "SceneObject.h"
#include "../ECore/Editor/Library.h"
#include "../ECore/Editor/ui_main.h"
#include "ui_leveltools.h"

CPortalUtils PortalUtils;
#define EPS_P 0.001f

CSector* CPortalUtils::GetSelectedSector()
{
	ObjectList lst;
    Scene->GetQueryObjects(lst,OBJCLASS_SECTOR,1,1,0);
    if (lst.size()==0) return 0;
    if (lst.size()>1){
    	ELog.DlgMsg(mtError,"Please select only one sector.");
    	return 0;
    }
    CSector* sector=(CSector*)*lst.begin();
    VERIFY(sector);
    return sector;
}

void CPortalUtils::RemoveSectorPortal(CSector* S)
{
    // remove existence sector portal
    ObjectList& lst = Scene->ListObj(OBJCLASS_PORTAL);
    ObjectIt _I = lst.begin();
    ObjectIt _E = lst.end();
    for (;_I!=_E;_I++){
    	CPortal* P=(CPortal*)(*_I);
        if((P->m_SectorFront==S)||(P->m_SectorBack==S))
            xr_delete(*_I);
    }
    _I = remove(lst.begin(),lst.end(),(CCustomObject*)0);
    lst.erase(_I,lst.end());
}

int CPortalUtils::CalculateSelectedPortals()
{
    int iPCount=0;
    if (Validate(false)){
        // get selected sectors
        ObjectList s_lst;
		int s_cnt=Scene->GetQueryObjects(s_lst, OBJCLASS_SECTOR, 1, 1, -1);
        if (s_cnt<2){
			ELog.DlgMsg(mtError,"Select at least 2 sectors.");
            return 0;
        }
        // remove exists portals
        ObjectList& p_lst=Scene->ListObj(OBJCLASS_PORTAL);
        for(ObjectIt _F=p_lst.begin(); _F!=p_lst.end(); _F++){
        	CSector* SF = ((CPortal*)(*_F))->m_SectorFront;
        	CSector* SB = ((CPortal*)(*_F))->m_SectorBack;
            if ((std::find(s_lst.begin(),s_lst.end(),SF)!=s_lst.end())&&(std::find(s_lst.begin(),s_lst.end(),SB)!=s_lst.end()))
				xr_delete(*_F);
        }
        ObjectIt _E = remove(p_lst.begin(),p_lst.end(),(CCustomObject*)0);
		p_lst.erase(_E,p_lst.end());
        // transfer from list to vector
        iPCount = CalculateSelectedPortals(s_lst);
    }else{
		ELog.DlgMsg(mtError,"*ERROR: Scene has non associated face (face without sector)!");
    }

	UI->SetStatus("...");
    return iPCount;
}
//---------------------------------------------------------------------------

void CPortalUtils::RemoveAllPortals()
{
    // remove all existence portal
	ObjectList& p_lst = Scene->ListObj(OBJCLASS_PORTAL);
    for (ObjectIt _F=p_lst.begin(); _F!=p_lst.end(); _F++) xr_delete(*_F);
	p_lst.erase(p_lst.begin(),p_lst.end());
}
//---------------------------------------------------------------------------

bool CPortalUtils::CreateDefaultSector()
{
    Fbox box;
	if (Scene->GetBox(box,OBJCLASS_SCENEOBJECT)){
		CSector* sector_def=xr_new<CSector>((LPVOID)0,DEFAULT_SECTOR_NAME);
        sector_def->sector_color.set(1,0,0,0);
        sector_def->m_bDefault=true;
        sector_def->CaptureAllUnusedMeshes();
		if (!sector_def->IsEmpty()){
         	Scene->AppendObject	(sector_def,true);
            return true;
        } else xr_delete(sector_def);
    }
    return false;
}
//---------------------------------------------------------------------------

bool CPortalUtils::RemoveDefaultSector()
{
    CCustomObject* O=Scene->FindObjectByName(DEFAULT_SECTOR_NAME,OBJCLASS_SECTOR);
    if (O){
    	Scene->RemoveObject	(O,false,true);
        xr_delete			(O);
		Scene->UndoSave		();
        UI->UpdateScene		();
        return true;
    }
    return false;
}
//---------------------------------------------------------------------------
/*
int CPortalUtils::CalculateAllPortals2()
{
    int bResult=0;
    if (Validate(false)){
        RemoveAllPortals();
        // calculate 0-sector
//        RemoveDefaultSector();
//        CreateDefaultSector();

        // transfer from list to vector
        ObjectList& s_lst=Scene->ListObj(OBJCLASS_SECTOR);
        xr_vector<CSector*> sectors;
        for(ObjectIt _F=s_lst.begin(); _F!=s_lst.end(); _F++) sectors.push_back((CSector*)(*_F));

        // calculate all portals
        for (u32 f=0; f<sectors.size()-1; f++)
            for (u32 b=f+1; b<sectors.size(); b++){
                bResult+=CalculatePortals(sectors[f],sectors[b]);
                AnsiString t;
                t.sprintf("Calculate %d of %d",f,sectors.size());
                UI->SetStatus(t.c_str());
            }

        Scene->UndoSave();
    }else{
		ELog.DlgMsg(mtError,"*ERROR: Scene has non associated face (face without sector)!");
    }

	UI->SetStatus("...");
    return bResult;
}
*/
//---------------------------------------------------------------------------
CSector* CPortalUtils::FindSector(CSceneObject* o, CEditableMesh* m){
	ObjectIt _F = Scene->FirstObj(OBJCLASS_SECTOR);
	ObjectIt _E = Scene->LastObj(OBJCLASS_SECTOR);
    for(;_F!=_E;_F++)
    	if (((CSector*)(*_F))->Contains(o,m))
        	return (CSector*)(*_F);
    return 0;
}

bool CPortalUtils::Validate(bool bMsg)
{
    Fbox box;
    bool bResult 	= false;
	if (Scene->GetBox(box,OBJCLASS_SCENEOBJECT)){
	    bResult 	= true;
		CSector* sector_def=xr_new<CSector>((LPVOID)0,DEFAULT_SECTOR_NAME);
        sector_def->CaptureAllUnusedMeshes();
        int f_cnt;
        sector_def->GetCounts(0,0,&f_cnt);
		if (f_cnt!=0){
        	if (bMsg){ 
            	ELog.DlgMsg(mtError,"*ERROR: Scene has '%d' non associated face!",f_cnt);
                for (SItemIt it=sector_def->sector_items.begin();it!=sector_def->sector_items.end();it++)
                	Msg		("! - scene object: '%s' [O:'%s', M:'%s']",it->object->Name, it->object->RefName(), it->mesh->Name().c_str());
            }
            bResult = false;
        }
        xr_delete	(sector_def);

        // verify sectors
        ObjectList& s_lst=Scene->ListObj(OBJCLASS_SECTOR);
        for(ObjectIt _F=s_lst.begin(); _F!=s_lst.end(); _F++)
        	if (!((CSector*)(*_F))->Validate(bMsg)) bResult = false;
    }else{
		if (bMsg) ELog.DlgMsg(mtInformation,"Validation failed! Can't compute bbox.");
    }
    return bResult;
}
//--------------------------------------------------------------------------------------------------
// calculate portals
//--------------------------------------------------------------------------------------------------

const int clpMX = 64, clpMY=24, clpMZ=64;

class sCollector
{
    struct sFace
    {
        u32 		v[3];
        CSector* 	sector;

        bool	hasVertex(u32 vert)
        {
            return (v[0]==vert)||(v[1]==vert)||(v[2]==vert);
        }
    };
    struct sVert : public Fvector
    {
        U32Vec adj;
    };
    struct sEdge
    {
        CSector* s[2];
        u32 v[2];
        u32 used;
        u32 dummy;

        sEdge() { used=false; }
	    static bool c_less(const sEdge& E1, const sEdge& E2)
        {
            if (E1.s[0]<E2.s[0]) return true;
            if (E1.s[1]<E2.s[1]) return true;
            if (E1.v[0]<E2.v[0]) return true;
            if (E1.v[1]<E2.v[1]) return true;
            return false;
        }
	    static bool c_equal(const sEdge& E1, const sEdge& E2)
        {
	        return (E1.s[0]==E2.s[0])&&(E1.s[1]==E2.s[1])&&(E1.v[0]==E2.v[0])&&(E1.v[1]==E2.v[1]);
        }
        static int __cdecl compare( const void *arg1, const void *arg2 )
		{
        	return memcmp(arg1,arg2,2*2*4);
		}
    };
   	struct sPortal
    {
        xr_deque<int> 	e;
        CSector* 		s[2];
    };

    DEFINE_VECTOR(sVert, sVertVec, sVertIt);
    DEFINE_VECTOR(sFace, sFaceVec, sFaceIt);
    DEFINE_VECTOR(sEdge, sEdgeVec, sEdgeIt);
    DEFINE_VECTOR(sPortal, sPortalVec, sPortalIt);
public:
    sVertVec		verts;
    sFaceVec		faces;
    sEdgeVec		edges;
   	sPortalVec	 	portals;

    Fvector			VMmin, VMscale;
    U32Vec			VM[clpMX+1][clpMY+1][clpMZ+1];
    Fvector			VMeps;

    u32				VPack(Fvector& V)
    {
        u32 P = 0xffffffff;

        u32 ix,iy,iz;
        ix         = floorf(float(V.x-VMmin.x)/VMscale.x*clpMX);
        iy         = floorf(float(V.y-VMmin.y)/VMscale.y*clpMY);
        iz         = floorf(float(V.z-VMmin.z)/VMscale.z*clpMZ);
        R_ASSERT	(ix<=clpMX && iy<=clpMY && iz<=clpMZ);

        {
            U32Vec* vl;
            vl 			= &(VM[ix][iy][iz]);
            U32It it	= vl->begin();
            U32It it_e	= vl->end();
            xr_vector<sCollector::sVert>::iterator verts_begin = verts.begin();
            for(;it!=it_e; ++it)
            {
//              if(verts[*it].similar(V) )	
                if( (*(verts_begin+*it)).similar(V) )	
                {
                    P = *it;
                    break;
                }
            }
        }
        if (0xffffffff==P)
        {
            P 					= verts.size();
            sVert 				sV; 
            sV.set				(V);
            verts.push_back		(sV);

            VM[ix][iy][iz].push_back(P);

            u32 ixE,iyE,izE;
            ixE                 = floorf(float(V.x+VMeps.x-VMmin.x)/VMscale.x*clpMX);
            iyE                 = floorf(float(V.y+VMeps.y-VMmin.y)/VMscale.y*clpMY);
            izE                 = floorf(float(V.z+VMeps.z-VMmin.z)/VMscale.z*clpMZ);

            R_ASSERT(ixE<=clpMX && iyE<=clpMY && izE<=clpMZ);

            if (ixE!=ix)							
            	VM[ixE][iy][iz].push_back	(P);

            if (iyE!=iy)							
            	VM[ix][iyE][iz].push_back	(P);

            if (izE!=iz)							
            	VM[ix][iy][izE].push_back	(P);

            if ((ixE!=ix)&&(iyE!=iy))				
            	VM[ixE][iyE][iz].push_back	(P);

            if ((ixE!=ix)&&(izE!=iz))				
            	VM[ixE][iy][izE].push_back	(P);

            if ((iyE!=iy)&&(izE!=iz))				
            	VM[ix][iyE][izE].push_back	(P);
                
            if ((ixE!=ix)&&(iyE!=iy)&&(izE!=iz))	
            	VM[ixE][iyE][izE].push_back	(P);
        }
        return P;
    }

    sCollector(const Fbox &bb)
    {
        VMscale.set	(bb.max.x-bb.min.x, bb.max.y-bb.min.y, bb.max.z-bb.min.z);
        VMmin.set	(bb.min);
        VMeps.set	(VMscale.x/clpMX/2, VMscale.y/clpMY/2, VMscale.z/clpMZ/2);
        VMeps.x		= (VMeps.x<EPS_L)?VMeps.x:EPS_L;
        VMeps.y		= (VMeps.y<EPS_L)?VMeps.y:EPS_L;
        VMeps.z		= (VMeps.z<EPS_L)?VMeps.z:EPS_L;
    }

    void			add_face(
        Fvector& v0,    // vertices
        Fvector& v1,
        Fvector& v2,
        CSector* sector
        )
    {
        sFace T;
        T.v[0] 	= VPack(v0);
        T.v[1] 	= VPack(v1);
        T.v[2] 	= VPack(v2);
        T.sector= sector;
        faces.push_back(T);
    }
    void update_adjacency(){
    	for (u32 i=0; i<faces.size(); i++){
        	sFace& F=faces[i];
			verts[F.v[0]].adj.push_back(i);
			verts[F.v[1]].adj.push_back(i);
			verts[F.v[2]].adj.push_back(i);
        }
    }
    void find_edges(){
    	for (u32 i=0; i<faces.size(); i++){
        	sFace& F=faces[i];
            U32It a_it;
            sVert& v0=verts[F.v[0]];
            sVert& v1=verts[F.v[1]];
            sVert& v2=verts[F.v[2]];

            // 1 pair (0-1)
            for (a_it=v0.adj.begin(); a_it!=v0.adj.end(); a_it++) {
            	if (*a_it==i) continue;

                sFace& T = faces[*a_it];
                if (T.sector==F.sector) continue;

                if (!T.hasVertex(F.v[1])) continue;

                sEdge E;
                E.v[0]=F.v[0];
                E.v[1]=F.v[1];
                E.s[0]=F.sector;
                E.s[1]=T.sector;
                edges.push_back(E);
            }
            // 2 pair (1-2)
            for (a_it=v1.adj.begin(); a_it!=v1.adj.end(); a_it++) {
            	if (*a_it==i) continue;

                sFace& T = faces[*a_it];
                if (T.sector==F.sector) continue;

                if (!T.hasVertex(F.v[2])) continue;

                sEdge E;
                E.v[0]=F.v[1];
                E.v[1]=F.v[2];
                E.s[0]=F.sector;
                E.s[1]=T.sector;
                edges.push_back(E);
            }
            // 3 pair (2-0)
            for (a_it=v2.adj.begin(); a_it!=v2.adj.end(); a_it++) {
            	if (*a_it==i) continue;

                sFace& T = faces[*a_it];
                if (T.sector==F.sector) continue;

                if (!T.hasVertex(F.v[0])) continue;

                sEdge E;
                E.v[0]=F.v[2];
                E.v[1]=F.v[0];
                E.s[0]=F.sector;
                E.s[1]=T.sector;
                edges.push_back(E);
            }
        }
    }
    void dump_edges() {
       	ELog.Msg(mtInformation,"********* dump");
    	for (u32 i=0; i<edges.size(); i++){
        	sEdge& E = edges[i];
        	ELog.Msg(mtInformation,"%d: %d,%d",i,E.v[0],E.v[1]);
        }
    }
    void sort_edges(){
    	// sort inside edges
    	for (u32 i=0; i<edges.size(); i++){
        	sEdge& E = edges[i];
            if (E.v[0]>E.v[1]) std::swap(E.v[0],E.v[1]);
            if (E.s[0]>E.s[1]) std::swap(E.s[0],E.s[1]);
        }

        // remove equal
        qsort(edges.begin(),edges.size(),sizeof(sEdge),sEdge::compare);
        sEdgeIt NewEnd = std::unique(edges.begin(),edges.end(),sEdge::c_equal);
        edges.erase(NewEnd,edges.end());
		//dump_edges();
    }
            
    void make_portals() {
        for(u32 e_it=0; e_it<edges.size(); e_it++)
        {
        	if (edges[e_it].used) continue;

            sPortal current;
            current.e.push_back (e_it);
            current.s[0] = edges[e_it].s[0];
            current.s[1] = edges[e_it].s[1];
            edges[e_it].used= true;

            for (;;) {
                sEdge& 	eFirst 	= edges[current.e[0]];
                sEdge& 	eLast  	= edges[current.e.back()];
                u32	vFirst	= eFirst.v[0];
                u32	vLast	= eLast.v[1];
                bool 	bFound 	= false;
                for (u32 i=0; i<edges.size(); i++)
                {
                    sEdge& E = edges[i];
                    if (E.used)					continue;
                    if (E.s[0]!=current.s[0]) 	continue;
                    if (E.s[1]!=current.s[1]) 	continue;

                    if (vLast ==E.v[0]) { E.used=true; current.e.push_back(i); bFound=true; break; }
                    if (vLast ==E.v[1]) { E.used=true; std::swap(E.v[0],E.v[1]); current.e.push_back (i); bFound=true; break; }
                    if (vFirst==E.v[0]) { E.used=true; std::swap(E.v[0],E.v[1]); current.e.push_front(i); bFound=true; break; }
                    if (vFirst==E.v[1]) { E.used=true; current.e.push_front(i); bFound=true; break; }
                }
                if (!bFound) break;
            }
            portals.push_back	(current);
        }
    }
    void export_portals()
    {
    	Tools->ClearDebugDraw();
        int ps = portals.size();
        int curr = 0;
    	for (sPortalIt p_it=portals.begin(); p_it!=portals.end(); ++p_it, ++curr)
        {
		    if (p_it->e.size()>1)
            {
                Msg("portal %d of %d", curr, ps);
            	// build vert-list
                xr_vector<int>	vlist;
                xr_deque<int>&	elist=p_it->e;
                vlist.reserve	(elist.size()*2);
                for (xr_deque<int>::iterator e=elist.begin(); e!=elist.end(); e++)
                {
                	vlist.push_back(edges[*e].v[0]);
                	vlist.push_back(edges[*e].v[1]);
                }
                IntIt end = std::unique(vlist.begin(), vlist.end());
                vlist.erase(end,vlist.end());

                // append portal
                string256 namebuffer;
                Scene->GenObjectName( OBJCLASS_PORTAL, namebuffer );
                CPortal* _O = xr_new<CPortal>((LPVOID)0,namebuffer);
                for (u32 i=0; i<vlist.size(); i++) {
	                _O->Vertices().push_back(verts[vlist[i]]);
                }
                _O->SetSectors(p_it->s[0],p_it->s[1]);
                _O->Update();
                if (_O->Valid()){
	 	            Scene->AppendObject(_O,false);
                }else{
                	xr_delete(_O);
				    ELog.Msg(mtError,"Can't simplify Portal :(\nPlease check geometry.\n'%s'<->'%s'",p_it->s[0]->Name,p_it->s[1]->Name);
                }
            }else
            	if (p_it->e.size()==0){
				    ELog.Msg(mtError,"Can't create Portal from 0 edge :(\nPlease check geometry.\n'%s'<->'%s'\n",p_it->s[0]->Name,p_it->s[1]->Name);
                }else{
                	Fvector& v0=verts[edges[p_it->e[0]].v[0]];
                	Fvector& v1=verts[edges[p_it->e[0]].v[1]];
				    ELog.Msg(mtError,"Can't create Portal from one edge :(\nPlease check geometry.\n'%s'<->'%s'", p_it->s[0]->Name, p_it->s[1]->Name);
                    Tools->m_DebugDraw.AppendLine(v0,v1);
                }

        }
    }
};

int CPortalUtils::CalculateSelectedPortals(ObjectList& sectors){
    // calculate portals
    Fbox bb;
    Scene->GetBox(bb,OBJCLASS_SCENEOBJECT);
    sCollector* CL = xr_new<sCollector>(bb);
    Fmatrix T;

    //1. xform + weld
    UI->SetStatus("xform + weld...");
    for (ObjectIt s_it=sectors.begin(); s_it!=sectors.end(); s_it++){
        CSector* S=(CSector*)(*s_it);
        for (SItemIt s_it=S->sector_items.begin();s_it!=S->sector_items.end();s_it++){
        	if (s_it->object->IsMUStatic()) continue;
            s_it->GetTransform(T);
            Fvector* m_verts=s_it->mesh->m_Vertices;
            for (u32 f_id=0; f_id<s_it->mesh->GetFCount(); f_id++){
                Fvector v0, v1, v2;
                st_Face& P			= s_it->mesh->GetFaces()[f_id];
                T.transform_tiny	(v0,m_verts[P.pv[0].pindex]);
                T.transform_tiny	(v1,m_verts[P.pv[1].pindex]);
                T.transform_tiny	(v2,m_verts[P.pv[2].pindex]);
                CL->add_face		(v0,v1,v2,S);
            }
        }
    }
    //2. update pervertex adjacency
    UI->SetStatus("updating per-vertex adjacency...");
    CL->update_adjacency();
    //3. find edges
    UI->SetStatus("searching edges...");
    CL->find_edges();
    //4. sort edges
    UI->SetStatus("sorting edges...");
    CL->sort_edges();
    //5. make portals
    UI->SetStatus("calculating portals...");
    CL->make_portals();
    //6. export portals
    UI->SetStatus("building portals...");
    CL->export_portals();

    Scene->UndoSave();

    int iRes = CL->portals.size();

    xr_delete(CL);

    return iRes;
}

int CPortalUtils::CalculateAllPortals()
{
    int iPCount=0;
    if (Validate(false)){
		UI->SetStatus("Prepare...");
        RemoveAllPortals();
        ObjectList& s_lst=Scene->ListObj(OBJCLASS_SECTOR);
        iPCount = CalculateSelectedPortals(s_lst);
    }else{
		ELog.DlgMsg(mtError,"*ERROR: Sector validation failed.");
    }

	UI->ResetStatus();
    return iPCount;
}

int CPortalUtils::CalculatePortals(CSector* SF, CSector* SB)
{
    int iPCount=0;
    if (Validate(false)){
		UI->SetStatus("Prepare...");
        RemoveAllPortals();
        // transfer from list to vector
        ObjectList sectors;
        sectors.push_back(SF);
        sectors.push_back(SB);

        iPCount = CalculateSelectedPortals(sectors);
    }else{
		ELog.DlgMsg(mtError,"*ERROR: Scene has non associated face (face without sector)!");
    }

	UI->ResetStatus();
    return iPCount;
}
