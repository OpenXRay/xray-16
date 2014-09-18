//----------------------------------------------------
// file: Sector.cpp
//----------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "ESceneSectorTools.h"
#include "Sector.h"
#include "../ECore/Editor/EditMesh.h"
#include "SceneObject.h"
#include "Scene.h"
#include "../ECore/Engine/Texture.h"
#include "../../ECore/Engine/cl_intersect.h"
#include "../../ECore/Engine/cl_collector.h"
#include "portal.h"
#include "portalutils.h"
#include "MgcConvexHull3D.h"
#include "../ECore/Editor/ui_main.h"
#include "../ECore/Editor/D3DUtils.h"
#include "ESceneGroupTools.h"

#define SECTOR_VERSION   					0x0012
//----------------------------------------------------
#define SECTOR_CHUNK_VERSION				0xF010
#define SECTOR_CHUNK_COLOR					0xF020
#define SECTOR_CHUNK_PRIVATE				0xF025
#define SECTOR_CHUNK_ITEMS					0xF030
#define 	SECTOR_CHUNK_ONE_ITEM			0xF031
#define 	SECTOR_CHUNK_MAP_IDX			0xF032
//----------------------------------------------------
CSectorItem::CSectorItem(){
	object=NULL;
    mesh=NULL;
}
CSectorItem::CSectorItem(CSceneObject* o, CEditableMesh* m){
	object=o;
    mesh=m;
}
void CSectorItem::GetTransform(Fmatrix& parent){
	object->GetFullTransformToWorld(parent);
}
bool CSectorItem::IsItem(const char* O, const char* M){
	return (0==stricmp(O,object->Name))&&(0==stricmp(M,mesh->Name().c_str()));
}
//------------------------------------------------------------------------------

CSector::CSector(LPVOID data, LPCSTR name):CCustomObject(data,name)
{
	Construct(data);
}

void CSector::Construct(LPVOID data)
{
	ClassID			= OBJCLASS_SECTOR;
    sector_color.set(1,1,1,0);
	m_bDefault		= false;
	m_sector_num		= -1;
	m_bHasLoadError = false;
    m_Flags.zero	();
    m_map_idx		= u8(-1);
}

CSector::~CSector()
{
	OnDestroy();
}

void CSector::OnFrame()
{
	inherited::OnFrame();
    if (m_Flags.is(flNeedUpdateVolume))
    	UpdateVolume();
}


bool CSector::FindSectorItem(const char* O, const char* M, SItemIt& it){
	for (it=sector_items.begin();it!=sector_items.end();it++)
    	if ((*it).IsItem(O,M)) return true;
    return false;
}

bool CSector::FindSectorItem(CSceneObject* o, CEditableMesh* m, SItemIt& it){
	for (it=sector_items.begin();it!=sector_items.end();it++)
    	if ((*it).IsItem(o,m)) return true;
    return false;
}

bool CSector::AddMesh	(CSceneObject* O, CEditableMesh* M)
{
	VERIFY(O&&M);
	SItemIt it;
	if (!(O->IsStatic()||O->IsMUStatic())) return false;
	if (!PortalUtils.FindSector(O,M))
	    if (!FindSectorItem(O, M, it)){
    	 	sector_items.push_back(CSectorItem(O, M));
		    m_Flags.set(flNeedUpdateVolume,TRUE);
            return true;
        }
    return false;
}                  

int CSector::DelMesh	(CSceneObject* O, CEditableMesh* M)
{
	VERIFY(O&&M);
	int res = 0;
	SItemIt it;
    if (FindSectorItem(O, M, it)){
    	sector_items.erase(it);
	    m_Flags.set(flNeedUpdateVolume,TRUE);
        res = 1;
    }
	if (sector_items.empty()){
    	res = 2;
    	ELog.Msg(mtInformation,"Last mesh deleted.\nSector has no meshes and will be removed.");
        DeleteThis();
    }
    return res;
}

bool CSector::GetBox( Fbox& box ) const
{
	box.set(m_SectorBox);
	return true;
}

void CSector::Render(int priority, bool strictB2F)
{
    ESceneSectorTool* lt = dynamic_cast<ESceneSectorTool*>(ParentTool); VERIFY(lt);
	if (2==priority)
    {
        if (true==strictB2F)
        {
            if (!lt->m_Flags.is(ESceneSectorTool::flDrawSolid)){
                Fmatrix matrix;
                Fcolor color;
                float k = Selected()?0.4f:0.2f;
                color.set(sector_color.r,sector_color.g,sector_color.b,k);
			    EDevice.SetShader(EDevice.m_SelectionShader);
                EDevice.SetRS(D3DRS_CULLMODE,D3DCULL_NONE);
                for (SItemIt it=sector_items.begin();it!=sector_items.end();++it)
                {
                    it->object->GetFullTransformToWorld(matrix);
                    it->mesh->RenderSelection( matrix, 0, color.get() );
                }
                EDevice.SetRS(D3DRS_CULLMODE,D3DCULL_CCW);
            }
        }else if (false==strictB2F)
        {
            Fmatrix matrix;
            Fcolor color;
            Fcolor color2;
            float k = Selected()?0.8f:0.5f;
            float k2 = Selected()?0.5f:0.2f;
            color.set(sector_color.r*k,sector_color.g*k,sector_color.b*k,1.f);
            color2.set(sector_color.r*k2,sector_color.g*k2,sector_color.b*k2,1.f);
            if (lt->m_Flags.is(ESceneSectorTool::flDrawSolid))
            {
                EDevice.SetShader(EDevice.m_WireShader);
                EDevice.SetRS(D3DRS_CULLMODE,D3DCULL_NONE);
                for (SItemIt it=sector_items.begin();it!=sector_items.end();++it)
                {
                    it->object->GetFullTransformToWorld(matrix);
                    it->mesh->RenderSelection( matrix, 0, color.get() );
                    it->mesh->RenderEdge( matrix, 0, color2.get() );
                }
                EDevice.SetRS(D3DRS_CULLMODE,D3DCULL_CCW);
            }
            if (Selected()){
                RCache.set_xform_world(Fidentity);
                DU_impl.DrawSelectionBoxB(m_SectorBox);
            }
        }
    }
}

void CSector::Move( Fvector& amount ){
// internal use only!!!
    m_SectorCenter.add(amount);
}

bool CSector::FrustumPick(const CFrustum& frustum)
{
	if (!frustum.testSphere_dirty(m_SectorCenter,m_SectorRadius)) return false;
	for (SItemIt s_it=sector_items.begin();s_it!=sector_items.end();s_it++)
    	if (s_it->mesh->FrustumPick(frustum,s_it->object->_Transform())) return true;
	return false;
}

bool CSector::RayPick(float& distance, const Fvector& start, const Fvector& direction, SRayPickInfo* pinf){
    bool bPick=false;
	for (SItemIt s_it=sector_items.begin();s_it!=sector_items.end();s_it++)
    	if (s_it->mesh->RayPick(distance,start,direction,s_it->object->_ITransform(),pinf)) bPick=true;
	return bPick;
}
//----------------------------------------------------

void CSector::UpdateVolume()
{
    Fbox bb;
    Fvector pt;
    m_SectorBox.invalidate();
    for (SItemIt s_it=sector_items.begin();s_it!=sector_items.end();s_it++){
        s_it->mesh->GetBox(bb);
        bb.xform(s_it->object->_Transform());
        for(int i=0; i<8; i++){
            bb.getpoint(i, pt);
            m_SectorBox.modify(pt);
        }
    }
    m_SectorBox.getsphere(m_SectorCenter,m_SectorRadius);

    UI->RedrawScene();

    m_Flags.set(flNeedUpdateVolume,FALSE);
}
//----------------------------------------------------
void CSector::OnDestroy( )
{
    // remove existence sector portal
    PortalUtils.RemoveSectorPortal(this);
}

void CSector::OnSceneUpdate()
{
/*
	bool bUpdate=false;
    for(SItemIt it = sector_items.begin();it!=sector_items.end();it++){
    	if (!(Scene->ContainsObject(it->object,OBJCLASS_SCENEOBJECT)&&it->object->GetReference()->ContainsMesh(it->mesh))){
            sector_items.erase(it); it--;
            bUpdate=true;
        }
    }
    if (bUpdate) PortalUtils.RemoveSectorPortal(this);
*/
	m_Flags.set(flNeedUpdateVolume,TRUE);
}
//----------------------------------------------------

EVisible CSector::Intersect(const Fvector& center, float radius)
{
	float dist=m_SectorCenter.distance_to(center);

    Fvector R;
    m_SectorBox.getradius(R);

	bool bInSphere = ((dist+radius)<m_SectorRadius)&&(radius>R.x)&&(radius>R.y)&&(radius>R.z);
	if (m_SectorBox.contains(center)){
    	if (bInSphere) return fvFully;
        else return fvPartialInside;
    }else{
    	if (dist<(radius+m_SectorRadius)) return fvPartialOutside;
    }
	return fvNone;
}
//----------------------------------------------------

EVisible CSector::Intersect(const Fbox& box)
{
	if (m_SectorBox.intersect(box)){
    	Fvector c; float r;
        box.getsphere(c,r);
    	return Intersect(c,r);
    }else return fvNone;
}
//----------------------------------------------------

void CSector::CaptureInsideVolume(){
	// test all mesh faces
	// fill object list (test bounding sphere intersection)
    ObjectList lst;
	if (Scene->SpherePick(m_SectorCenter, m_SectorRadius, OBJCLASS_SCENEOBJECT, lst)){
    // test all object meshes
        Fmatrix matrix;
	    CSceneObject *obj=NULL;
        // ignore dynamic objects
		for(ObjectIt _F = lst.begin();_F!=lst.end();_F++){
	        obj = (CSceneObject*)(*_F);
	        if (!(obj->IsStatic()||obj->IsMUStatic())) continue;
	        EditMeshVec* M = obj->Meshes();
            R_ASSERT(M);
            for(EditMeshIt m_def = M->begin();m_def!=M->end();m_def++){
                obj->GetFullTransformToWorld(matrix);
                Fbox bb;
				(*m_def)->GetBox(bb);
                bb.xform(matrix);
                EVisible vis=Intersect(bb);
            	if ((fvFully==vis)||(fvPartialInside==vis))
					AddMesh(obj,*m_def);
            }
        }
		m_Flags.set		(flNeedUpdateVolume,TRUE);
		UI->RedrawScene	();
        ExecCommand		(COMMAND_UPDATE_PROPERTIES);
    }
}
//----------------------------------------------------

//. Fvector _dir[6]={{0,-1,0},{0,1,0},}

void CSector::DistributeInsideObjects(){
/*
	// test all mesh faces
	// fill object list (test bounding sphere intersection)
    ObjectList lst;
	if (Scene->SpherePick(m_SectorCenter, m_SectorRadius, OBJCLASS_SCENEOBJECT, lst)){
    // test all object meshes
        Fmatrix matrix;
	    CSceneObject *obj=NULL;
        // ignore dynamic objects
		for(ObjectIt _F = lst.begin();_F!=lst.end();_F++){
	        obj = (CSceneObject*)(*_F);
	        if (!(obj->IsStatic()||obj->IsMUStatic())) continue;
	        EditMeshVec* M = obj->Meshes();
            R_ASSERT(M);
            for(EditMeshIt m_def = M->begin();m_def!=M->end();m_def++){
                obj->GetFullTransformToWorld(matrix);
                Fbox bb;
				(*m_def)->GetBox(bb);
                bb.xform		(matrix);
                EVisible vis=Intersect(bb);
            	if ((fvFully==vis)||(fvPartialInside==vis)){
					float dist		= m_SectorRadius;
                    Fvector start,dir;
                    bb.getcenter	(start);
                    _f o r ()
                    Scene->RayPickObject(dist,start,dir,)
					//.AddMesh(obj,*m_def);
                }
            }
        }
		m_Flags.set		(flNeedUpdateVolume,TRUE);
		UI->RedrawScene	();
        ExecCommand		(COMMAND_UPDATE_PROPERTIES);
    }
*/    
}
//----------------------------------------------------


void CSector::CaptureAllUnusedMeshes()
{
    U32Vec fl;
    CSceneObject *obj=NULL;
    ObjectList& lst=Scene->ListObj(OBJCLASS_SCENEOBJECT);
    // ignore dynamic objects
    SPBItem* pb = UI->ProgressStart(lst.size(),"Capturing unused face...");
    for(ObjectIt _F = lst.begin();_F!=lst.end();_F++){
        pb->Inc();
        obj = (CSceneObject*)(*_F);
        if (!(obj->IsStatic()||obj->IsMUStatic())) continue;
        EditMeshVec* M = obj->Meshes();
        R_ASSERT(M);
        for(EditMeshIt m_def = M->begin(); m_def!=M->end();m_def++)
        	AddMesh(obj,*m_def);
    }
    UI->ProgressEnd(pb);
    UI->RedrawScene();
}

//----------------------------------------------------
bool CSector::SpherePick(const Fvector& center, float radius){
	float R=radius+m_SectorRadius;
    float dist_sqr=center.distance_to_sqr(m_SectorCenter);
    if (dist_sqr<R*R) return true;
    return false;
}
//----------------------------------------------------

bool CSector::IsEmpty()
{
    int count=0;
    for (SItemIt it=sector_items.begin();it!=sector_items.end();it++)
        count+=it->mesh->GetFaceCount(true);
	return !count;
}
//----------------------------------------------------

void CSector::GetCounts(int* objects, int* meshes, int* faces)
{
	if (faces){
    	*faces=0;
	    for (SItemIt it=sector_items.begin();it!=sector_items.end();it++)
    	    *faces+=it->mesh->GetFaceCount(true);
    }
	if (meshes) *meshes=sector_items.size();
	if (objects){
        xr_set<CSceneObject*> objs;
	    for (SItemIt it=sector_items.begin();it!=sector_items.end();it++)
        	objs.insert(it->object);
    	*objects=objs.size();
    }
}
//----------------------------------------------------

void CSector::LoadSectorDef( IReader* F )
{
	string256 o_name="";
	string256 m_name="";

    CSectorItem sitem;

	// sector item
    R_ASSERT(F->find_chunk(SECTOR_CHUNK_ONE_ITEM));
	F->r_stringZ(o_name,sizeof(o_name));
	sitem.object=(CSceneObject*)Scene->FindObjectByName(o_name,OBJCLASS_SCENEOBJECT);
    if (sitem.object==NULL)
    {
        ELog.Msg		(mtError,"Sector Item contains object '%s' - can't load.\nObject not found.",o_name);
        m_bHasLoadError = true;
        return;
    }

    if (!(sitem.object->IsStatic()||sitem.object->IsMUStatic()))
    {
    	ELog.Msg(mtError,"Sector Item contains object '%s' - can't load.\nObject is dynamic.",o_name);
        m_bHasLoadError = true;
        return;
    }

	F->r_stringZ(m_name,sizeof(m_name));
	sitem.mesh=sitem.object->GetReference()->FindMeshByName(m_name);
    if (sitem.mesh==0)
    {
    	ELog.Msg(mtError,"Sector Item contains object '%s' mesh '%s' - can't load.\nMesh not found.",o_name,m_name);
        m_bHasLoadError = true;
        return;
    }

    sector_items.push_back(sitem);
}

void CSector::LoadSectorDefLTX( CInifile& ini, LPCSTR sect_name, u32 item_idx )
{
	LPCSTR 		o_name = NULL;
	LPCSTR 		m_name = NULL;

    CSectorItem 	sitem;
    string512 		buff;

    sprintf			(buff,"item_object_name_%.4d",item_idx);
    o_name			= ini.r_string	(sect_name, buff);

	// sector item
	o_name = ini.r_string(sect_name, buff);
    if(!o_name)
            ELog.Msg		(mtError,"Sector Item contains not nnamed object - can't load");
    
	sitem.object=(CSceneObject*)Scene->FindObjectByName(o_name,OBJCLASS_SCENEOBJECT);
    if (sitem.object==NULL)
    {
        ELog.Msg		(mtError,"Sector Item contains object '%s' - can't load.\nObject not found.",o_name);
        m_bHasLoadError = true;
        return;
    }

    if (!(sitem.object->IsStatic()||sitem.object->IsMUStatic()))
    {
    	ELog.Msg(mtError,"Sector Item contains object '%s' - can't load.\nObject is dynamic.",o_name);
        m_bHasLoadError = true;
        return;
    }

    sprintf			(buff,"item_mesh_name_%.4d",item_idx);
    m_name			= ini.r_string	(sect_name, buff);

	sitem.mesh=sitem.object->GetReference()->FindMeshByName(m_name);
    if (sitem.mesh==0)
    {
    	ELog.Msg(mtError,"Sector Item contains object '%s' mesh '%s' - can't load.\nMesh not found.",o_name,m_name);
        m_bHasLoadError = true;
        return;
    }

    sector_items.push_back(sitem);
}

bool CSector::LoadLTX(CInifile& ini, LPCSTR sect_name)
{
	u32 version = ini.r_u32		(sect_name, "version");
    if( version<0x0011)
    {
        ELog.Msg( mtError, "CSector: Unsupported version.");
        return false;
    }

	CCustomObject::LoadLTX		(ini, sect_name);

    sector_color.set(			ini.r_color(sect_name, "sector_color") );

    m_bDefault 					= ini.r_bool(sect_name, "default");

    u32 obj_cnt 				= ini.r_u32(sect_name, "items_count");
    for(u32 i=0; i<obj_cnt; ++i)
    {
        LoadSectorDefLTX(ini, sect_name, i);
    }

    if(version>=0x0012)
    	m_map_idx 				= ini.r_u8(sect_name, "change_map_to_idx");
        
    if (sector_items.empty()) return false;

    m_Flags.set(flNeedUpdateVolume,TRUE);
    return true;
}

void CSector::SaveLTX(CInifile& ini, LPCSTR sect_name)
{
	CCustomObject::SaveLTX(ini, sect_name);

	ini.w_u32			(sect_name, "version", SECTOR_VERSION);

	ini.w_color			(sect_name, "sector_color", sector_color.get());

	ini.w_bool			(sect_name, "default", m_bDefault);

    int count=0;
	ini.w_u32			(sect_name, "items_count", sector_items.size());
    string512			buff;
    for(SItemIt it=sector_items.begin(); it!=sector_items.end(); ++it)
    {
            sprintf			(buff,"item_object_name_%.4d",count);
            ini.w_string	(sect_name, buff, it->object->Name);
            sprintf			(buff,"item_mesh_name_%.4d",count);
            ini.w_string	(sect_name, buff, it->mesh->Name().c_str());
            ++count;
    }
   	ini.w_u8(sect_name, "change_map_to_idx", m_map_idx);
    
}

bool CSector::LoadStream(IReader& F)
{
	u16 version = 0;

    char buf[1024];
    R_ASSERT(F.r_chunk(SECTOR_CHUNK_VERSION,&version));
    if( version!=SECTOR_VERSION ){
        ELog.Msg( mtError, "CSector: Unsupported version.");
        return false;
    }

	CCustomObject::LoadStream(F);

    R_ASSERT(F.r_chunk(SECTOR_CHUNK_COLOR,&sector_color));

	R_ASSERT(F.find_chunk(SECTOR_CHUNK_PRIVATE));
    m_bDefault 		= F.r_u8();

    // Objects
    IReader* OBJ 	= F.open_chunk(SECTOR_CHUNK_ITEMS);
    if(OBJ){
        IReader* O   	= OBJ->open_chunk(0);
        for (int count=1; O; count++) {
            LoadSectorDef(O);
            O->close	();
            O 			= OBJ->open_chunk(count);
        }
        OBJ->close();
    }

	if(F.find_chunk	(SECTOR_CHUNK_MAP_IDX))
		m_map_idx		= F.r_u8();

    if (sector_items.empty()) return false;

    m_Flags.set(flNeedUpdateVolume,TRUE);
    return true;
}

void CSector::SaveStream(IWriter& F)
{
	CCustomObject::SaveStream(F);

	F.open_chunk	(SECTOR_CHUNK_VERSION);
	F.w_u16			(SECTOR_VERSION);
	F.close_chunk	();

    F.w_chunk		(SECTOR_CHUNK_COLOR,&sector_color,sizeof(Fcolor));

	F.open_chunk	(SECTOR_CHUNK_PRIVATE);
	F.w_u8			(m_bDefault);
	F.close_chunk	();

	F.open_chunk	(SECTOR_CHUNK_ITEMS);
    int count=0;
    for(SItemIt it=sector_items.begin(); it!=sector_items.end(); it++){
        F.open_chunk(count); count++;
            F.open_chunk	(SECTOR_CHUNK_ONE_ITEM);
            F.w_stringZ		(it->object->Name);
            F.w_stringZ		(it->mesh->Name());
	        F.close_chunk	();
        F.close_chunk		();
    }
	F.close_chunk	();

	F.open_chunk	(SECTOR_CHUNK_MAP_IDX);
	F.w_u8			(m_map_idx);
	F.close_chunk	();
}

//----------------------------------------------------
xr_token level_sub_map[] =
{
	{"default", u8(-1)},
	{"#0", 0},
	{"#1", 1},
	{"#2", 2},
	{"#3", 3},
	{NULL, 4}
};

void CSector::FillProp(LPCSTR pref, PropItemVec& items)
{
	inherited::FillProp(pref,items);        
    PHelper().CreateFColor(items, PrepareKey(pref,"Color"), &sector_color);
    int faces, objects, meshes;
    GetCounts(&objects,&meshes,&faces);
    PHelper().CreateCaption(items,PrepareKey(pref,Name,"Contents\\Objects"),	AnsiString(objects).c_str());
    PHelper().CreateCaption(items,PrepareKey(pref,Name,"Contents\\Meshes"), 	AnsiString(meshes).c_str());
    PHelper().CreateCaption(items,PrepareKey(pref,Name,"Contents\\Faces"), 	AnsiString(faces).c_str());
	PHelper().CreateToken8(items, PrepareKey(pref,Name,"Change LevelMap to"), &m_map_idx, level_sub_map);
    
}
//----------------------------------------------------

bool CSector::GetSummaryInfo(SSceneSummary* inf)
{
	inherited::GetSummaryInfo	(inf);
	inf->sector_cnt++;
	return true;
}

bool CSector::Validate(bool bMsg)
{
	bool bRes		= true;
    // verify face count
    int f_cnt;
    GetCounts		(0,0,&f_cnt);
    if (f_cnt<=4){
        if (bMsg) 	ELog.Msg(mtError,"*ERROR: Sector: '%s' - face count < 4!",Name);
        bRes		= false;
    }
    // verify shader compatibility
	bool bRenderableFound	= false;    
    for (SItemIt it=sector_items.begin();it!=sector_items.end();it++){
        for (SurfFacesPairIt sf_it=it->mesh->m_SurfFaces.begin(); sf_it!=it->mesh->m_SurfFaces.end(); sf_it++){
            CSurface* surf 		= sf_it->first;
            Shader_xrLC* c_sh	= EDevice.ShaderXRLC.Get(surf->_ShaderXRLCName());
            if (c_sh->flags.bRendering)	bRenderableFound = true;
        }
	}
    if (!bRenderableFound){
        if (bMsg) 	ELog.Msg(mtError,"*ERROR: Sector: '%s' - can't find any renderable face!",Name);
    	bRes 		= false;
	}        
   	return bRes;
}
