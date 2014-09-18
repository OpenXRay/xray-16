//----------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "ESceneDOTools.h"
#include "../ECore/Editor/EditMesh.h"
#include "../ECore/Editor/EditObject.h"
#include "../ECore/Engine/Texture.h"
#include "Scene.h"
#include "SceneObject.h"
#include "leftbar.h"
#include "cl_intersect.h"
#include "../ECore/Editor/Library.h"
#include "ui_levelmain.h"

#include "..\..\Layers\xrRender\DetailFormat.h"
#include "bottombar.h"
#include "../ECore/Editor/ImageManager.h"
#include "../ECORE/Editor/D3DUtils.h"

static const u32 DETMGR_VERSION = 0x0003ul;
//------------------------------------------------------------------------------
enum{
    DETMGR_CHUNK_VERSION		= 0x1000ul,
    DETMGR_CHUNK_HEADER 		= 0x0000ul,
    DETMGR_CHUNK_OBJECTS 		= 0x0001ul,
    DETMGR_CHUNK_SLOTS			= 0x0002ul,
    DETMGR_CHUNK_BBOX			= 0x1001ul,
    DETMGR_CHUNK_BASE_TEXTURE	= 0x1002ul,
    DETMGR_CHUNK_COLOR_INDEX 	= 0x1003ul,
    DETMGR_CHUNK_SNAP_OBJECTS 	= 0x1004ul,
    DETMGR_CHUNK_DENSITY	 	= 0x1005ul,
    DETMGR_CHUNK_FLAGS			= 0x1006ul,
};
//----------------------------------------------------

//------------------------------------------------------------------------------
EDetailManager::EDetailManager():ESceneToolBase(OBJCLASS_DO)
{
	dtSlots				= 0;
    ZeroMemory			(&dtH,sizeof(dtH));
    m_Selected.clear	();
    InitRender			();
//.	EDevice.seqDevCreate.Add	(this,REG_PRIORITY_LOW);
//.	EDevice.seqDevDestroy.Add(this,REG_PRIORITY_NORMAL);
    m_Flags.assign		(flObjectsDraw);
}

EDetailManager::~EDetailManager(){
//.	EDevice.seqDevCreate.Remove(this);
//.	EDevice.seqDevDestroy.Remove(this);
	Clear	();
    Unload	();
}
//------------------------------------------------------------------------------

void EDetailManager::ClearColorIndices()
{
	inherited::Clear	();
    RemoveDOs			();
    m_ColorIndices.clear();
}
void EDetailManager::ClearSlots()
{
    ZeroMemory			(&dtH,sizeof(DetailHeader));
    xr_free				(dtSlots);
	m_Selected.clear	();
    InvalidateCache		();
}
void EDetailManager::ClearBase()
{
    m_Base.Clear		();
    m_SnapObjects.clear	();
    ExecCommand			(COMMAND_REFRESH_SNAP_OBJECTS);
}
void EDetailManager::Clear(bool bSpecific)
{
	ClearBase			();
	ClearColorIndices	();
    ClearSlots			();
    m_Flags.zero		();
    m_RTFlags.zero		();
}
//------------------------------------------------------------------------------

void EDetailManager::InvalidateCache()
{
	// resize visible
	m_visibles[0].resize	(objects.size());	// dump(visible[0]);
	m_visibles[1].resize	(objects.size());	// dump(visible[1]);
	m_visibles[2].resize	(objects.size());	// dump(visible[2]);
	// Initialize 'vis' and 'cache'
	cache_Initialize	();
}

extern void bwdithermap	(int levels, int magic[16][16] );
void EDetailManager::InitRender()
{
	// inavlidate cache
	InvalidateCache		();
	// Make dither matrix
	bwdithermap		(2,dither);

	soft_Load	();
}
//------------------------------------------------------------------------------

void EDetailManager::OnRender(int priority, bool strictB2F)
{
	if (dtSlots){
    	if (1==priority){
        	if (false==strictB2F){
            	if (m_Flags.is(flSlotBoxesDraw)){
                    RCache.set_xform_world(Fidentity);
                    EDevice.SetShader	(EDevice.m_WireShader);

                    Fvector			c;
                    Fbox			bbox;
                    u32			inactive = 0xff808080;
                    u32			selected = 0xffffffff;
                    float dist_lim	= 75.f*75.f;
                    for (u32 z=0; z<dtH.size_z; z++){
                        c.z			= fromSlotZ(z);
                        for (u32 x=0; x<dtH.size_x; x++){
                            bool bSel 	= m_Selected[z*dtH.size_x+x];
                            DetailSlot* slot = dtSlots+z*dtH.size_x+x;
                            c.x			= fromSlotX(x);
                            c.y			= slot->r_ybase()+slot->r_yheight()*0.5f; //(slot->y_max+slot->y_min)*0.5f;
                            float dist = EDevice.m_Camera.GetPosition().distance_to_sqr(c);
                         	if ((dist<dist_lim)&&::Render->ViewBase.testSphere_dirty(c,DETAIL_SLOT_SIZE_2)){
								bbox.min.set(c.x-DETAIL_SLOT_SIZE_2, slot->r_ybase(), 					c.z-DETAIL_SLOT_SIZE_2);
                            	bbox.max.set(c.x+DETAIL_SLOT_SIZE_2, slot->r_ybase()+slot->r_yheight(),	c.z+DETAIL_SLOT_SIZE_2);
                            	bbox.shrink	(0.05f);
								DU_impl.DrawSelectionBoxB(bbox,bSel?&selected:&inactive);
							}
                        }
                    }
                }
            }else{
				RCache.set_xform_world				(Fidentity);
                if (m_Flags.is(flBaseTextureDraw))	m_Base.Render			(m_Flags.is(flBaseTextureBlended));
				if (m_Flags.is(flObjectsDraw))		CDetailManager::Render	();
            }
        }
    }
}
//------------------------------------------------------------------------------

void EDetailManager::OnDeviceCreate()
{
	// base texture
    m_Base.CreateShader();
	// detail objects
	for (DetailIt it=objects.begin(); it!=objects.end(); it++)
    	((EDetail*)(*it))->OnDeviceCreate();
	soft_Load	();
}

void EDetailManager::OnDeviceDestroy()
{
	// base texture
    m_Base.DestroyShader();
	// detail objects
	for (DetailIt it=objects.begin(); it!=objects.end(); it++)
    	((EDetail*)(*it))->OnDeviceDestroy();
	soft_Unload	();
}


void EDetailManager::OnObjectRemove(CCustomObject* O, bool bDeleting)
{
	ObjectIt it=std::find(m_SnapObjects.begin(),m_SnapObjects.end(),O);
	if (it!=m_SnapObjects.end()){
    	m_RTFlags.set		(flRTGenerateBaseMesh,TRUE);
		m_SnapObjects.remove(O);
    }
}
void EDetailManager::OnSynchronize()
{
}
void EDetailManager::OnSceneUpdate()       
{
}

void EDetailManager::OnFrame()
{
    if (m_RTFlags.is(flRTGenerateBaseMesh)&&m_Base.Valid()){
    	m_RTFlags.set		(flRTGenerateBaseMesh,FALSE);
	    m_Base.CreateRMFromObjects(m_BBox,m_SnapObjects);
    }
}

void EDetailManager::ExportColorIndices(LPCSTR fname)
{
	IWriter* F 	= FS.w_open(fname);
    if (F){
	    SaveColorIndices(*F);
    	FS.w_close	(F);
    }
}

bool EDetailManager::ImportColorIndices(LPCSTR fname)
{
	IReader* F=FS.r_open(fname);
    if (F){
        ClearColorIndices	();
        LoadColorIndices	(*F);
        FS.r_close			(F);
        return true;
    }else{
    	ELog.DlgMsg			(mtError,"Can't open file '%s'.",fname);
        return false;
    }
}

void EDetailManager::SaveColorIndices(IWriter& F)
{
	// objects
	F.open_chunk		(DETMGR_CHUNK_OBJECTS);
    for (DetailIt it=objects.begin(); it!=objects.end(); it++){
		F.open_chunk	(it-objects.begin());
        ((EDetail*)(*it))->Save		(F);
	    F.close_chunk	();
    }
    F.close_chunk		();
    // color index map
	F.open_chunk		(DETMGR_CHUNK_COLOR_INDEX);
    F.w_u8				((u8)m_ColorIndices.size());
    ColorIndexPairIt S 	= m_ColorIndices.begin();
    ColorIndexPairIt E 	= m_ColorIndices.end();
    ColorIndexPairIt i_it= S;
	for(; i_it!=E; i_it++){
		F.w_u32		(i_it->first);
        F.w_u8			((u8)i_it->second.size());
	    for (DOIt d_it=i_it->second.begin(); d_it!=i_it->second.end(); d_it++)
        	F.w_stringZ	((*d_it)->GetName());
    }
    F.close_chunk		();
}

bool EDetailManager::LoadColorIndices(IReader& F)
{
	VERIFY				(objects.empty());
    VERIFY  			(m_ColorIndices.empty());

    bool bRes			= true;
    // objects
    IReader* OBJ 		= F.open_chunk(DETMGR_CHUNK_OBJECTS);
    if (OBJ){
        IReader* O   	= OBJ->open_chunk(0);
        for (int count=1; O; count++) {
            EDetail* DO	= xr_new<EDetail>();
            if (DO->Load(*O)) 	objects.push_back(DO);
            else				bRes = false;
            O->close();
            O = OBJ->open_chunk(count);
        }
        OBJ->close();
    }
    // color index map
    R_ASSERT			(F.find_chunk(DETMGR_CHUNK_COLOR_INDEX));
    int cnt				= F.r_u8();
    string256			buf;
    u32 index;
    int ref_cnt;
    for (int k=0; k<cnt; k++){
		index			= F.r_u32();
        ref_cnt			= F.r_u8();
		for (int j=0; j<ref_cnt; j++){
        	F.r_stringZ	(buf,sizeof(buf));
            EDetail* DO	= FindDOByName(buf);
            if (DO) 	m_ColorIndices[index].push_back(DO);    
            else		bRes=false;
        }
    }
	InvalidateCache		();

    return bRes;
}
bool EDetailManager::LoadLTX(CInifile& ini)
{
	R_ASSERT2			(0, "not_implemented");
    return true;
}

void EDetailManager::SaveLTX(CInifile& ini, int id)
{
	R_ASSERT2			(0, "not_implemented");
/*
	inherited::SaveLTX	(ini);

    ini.w_u32			("main", "version", DETMGR_VERSION);

    ini.w_u32			("main", "flags", m_Flags.get());

	// header

    ini.w_u32			("detail_header", "version", dtH.version);
    ini.w_u32			("detail_header", "object_count", dtH.object_count);
    ini.w_ivector2		("detail_header", "offset", Ivector2().set(dtH.offs_x, dtH.offs_z) );
    ini.w_ivector2		("detail_header", "size", Ivector2().set(dtH.size_x, dtH.size_z) );

    // objects
    SaveColorIndicesLTX	(F);

    // slots
	F.open_chunk		(DETMGR_CHUNK_SLOTS);
    F.w_u32				(dtH.size_x*dtH.size_z);
	F.w					(dtSlots,dtH.size_x*dtH.size_z*sizeof(DetailSlot));
    F.close_chunk		();

    // internal
    // bbox
    ini.w_fvector3		("main", "bbox_min", m_BBox.min);
    ini.w_fvector3		("main", "bbox_max", m_BBox.max);

	// base texture
    if (m_Base.Valid())
    {
    	ini.w_string	("main", "base_texture", m_Base.GetName());
    }
    ini.w_float			("main", "detail_density", ps_r__Detail_density);

	// snap objects
    for (ObjectIt o_it=m_SnapObjects.begin(); o_it!=m_SnapObjects.end(); ++o_it)
    	ini.w_string	("snap_objects", (*o_it)->Name, NULL);
*/        
}

bool EDetailManager::LoadStream(IReader& F)
{
	inherited::LoadStream	(F);

    string256 buf;
    R_ASSERT			(F.find_chunk(DETMGR_CHUNK_VERSION));
	u32 version			= F.r_u32();

    if (version!=DETMGR_VERSION){
    	ELog.Msg(mtError,"EDetailManager: unsupported version.");
        return false;
    }

    if (F.find_chunk(DETMGR_CHUNK_FLAGS)) m_Flags.assign(F.r_u32());
    
	// header
    R_ASSERT			(F.r_chunk(DETMGR_CHUNK_HEADER,&dtH));

    // slots
    R_ASSERT			(F.find_chunk(DETMGR_CHUNK_SLOTS));
    int slot_cnt		= F.r_u32();
	if (slot_cnt)dtSlots= xr_alloc<DetailSlot>(slot_cnt);
    m_Selected.resize	(slot_cnt);
	F.r					(dtSlots,slot_cnt*sizeof(DetailSlot));

    // objects
    if (!LoadColorIndices(F)){
        ELog.DlgMsg		(mtError,"EDetailManager: Some objects removed. Reinitialize objects.",buf);
        InvalidateSlots	();
    }

    // internal
    // bbox
    R_ASSERT			(F.r_chunk(DETMGR_CHUNK_BBOX,&m_BBox));

	// snap objects
    if (F.find_chunk(DETMGR_CHUNK_SNAP_OBJECTS)){
		int snap_cnt 		= F.r_u32();
        if (snap_cnt){
	        for (int i=0; i<snap_cnt; i++){
    	    	F.r_stringZ	(buf,sizeof(buf));
        	    CCustomObject* O = Scene->FindObjectByName(buf,OBJCLASS_SCENEOBJECT);
            	if (!O)		ELog.Msg(mtError,"EDetailManager: Can't find snap object '%s'.",buf);
	            else		m_SnapObjects.push_back(O);
    	    }
        }
    }

    if (F.find_chunk(DETMGR_CHUNK_DENSITY))
		ps_r__Detail_density= F.r_float();

	// base texture
	if(F.find_chunk(DETMGR_CHUNK_BASE_TEXTURE)){
	    F.r_stringZ		(buf,sizeof(buf));
    	if (m_Base.LoadImage(buf)){
		    m_Base.CreateShader();
            m_RTFlags.set(flRTGenerateBaseMesh,TRUE);
        }else{
        	ELog.Msg(mtError,"EDetailManager: Can't find base texture '%s'.",buf);
            ClearSlots();
            ClearBase();
        }
    }

    InvalidateCache		();

    return true;
}

bool EDetailManager::LoadSelection(IReader& F)
{
	Clear();
	return LoadStream			(F);
}

void EDetailManager::SaveStream(IWriter& F)
{
	inherited::SaveStream	(F);

	// version
	F.open_chunk		(DETMGR_CHUNK_VERSION);
    F.w_u32				(DETMGR_VERSION);
    F.close_chunk		();

	F.open_chunk		(DETMGR_CHUNK_FLAGS);
    F.w_u32				(m_Flags.get());
	F.close_chunk		();

	// header
	F.w_chunk			(DETMGR_CHUNK_HEADER,&dtH,sizeof(DetailHeader));

    // objects
    SaveColorIndices	(F);

    // slots
	F.open_chunk		(DETMGR_CHUNK_SLOTS);
    F.w_u32				(dtH.size_x*dtH.size_z);
	F.w					(dtSlots,dtH.size_x*dtH.size_z*sizeof(DetailSlot));
    F.close_chunk		();
    // internal
    // bbox
	F.w_chunk			(DETMGR_CHUNK_BBOX,&m_BBox,sizeof(Fbox));
	// base texture
    if (m_Base.Valid()){
		F.open_chunk	(DETMGR_CHUNK_BASE_TEXTURE);
    	F.w_stringZ		(m_Base.GetName());
	    F.close_chunk	();
    }
    F.open_chunk		(DETMGR_CHUNK_DENSITY);
    F.w_float			(ps_r__Detail_density);
    F.close_chunk		();
	// snap objects
	F.open_chunk		(DETMGR_CHUNK_SNAP_OBJECTS);
    F.w_u32				(m_SnapObjects.size());
    for (ObjectIt o_it=m_SnapObjects.begin(); o_it!=m_SnapObjects.end(); o_it++)
    	F.w_stringZ		((*o_it)->Name);
    F.close_chunk		();
}

void EDetailManager::SaveSelection(IWriter& F)
{
	SaveStream(F);
}

bool EDetailManager::Export(LPCSTR path) 
{
    AnsiString fn		= AnsiString(path)+"build.details";
    bool bRes=true;

    SPBItem* pb = UI->ProgressStart(5,"Making details...");
	CMemoryWriter F;

    pb->Inc				("merge textures");
    Fvector2Vec			offsets;
    Fvector2Vec			scales;
    boolVec				rotated;
    RStringSet 			textures_set;
    RStringVec 			textures;
    U32Vec				remap;
    U8Vec remap_object	(objects.size(),u8(-1));

    int slot_cnt		= dtH.size_x*dtH.size_z;
	for (int slot_idx=0; slot_idx<slot_cnt; slot_idx++){
    	DetailSlot* it 	= &dtSlots[slot_idx];
        for (int part=0; part<4; part++){
        	u8 id		= it->r_id(part);
        	if (id!=DetailSlot::ID_Empty) {
            	textures_set.insert(((EDetail*)(objects[id]))->GetTextureName());
                remap_object[id] = 1;
            }
        }
    }
    textures.assign		(textures_set.begin(),textures_set.end());

    U8It remap_object_it= remap_object.begin();

    u32 new_idx			= 0;
    for (DetailIt d_it=objects.begin(); d_it!=objects.end(); d_it++,remap_object_it++)
    	if ((*remap_object_it==1)&&(textures_set.find(((EDetail*)(*d_it))->GetTextureName())!=textures_set.end()))
	    	*remap_object_it	= (u8)new_idx++;

    AnsiString 			do_tex_name = ChangeFileExt(fn,"_details");
    int res				= ImageLib.CreateMergedTexture(textures,do_tex_name.c_str(),STextureParams::tfADXT1,256,1024,256,1024,offsets,scales,rotated,remap);
    if (1!=res)			bRes=FALSE;

    pb->Inc				("export geometry");
    // objects
    int object_idx		= 0;
    if (bRes){
	    do_tex_name 	= ExtractFileName(do_tex_name);
        F.open_chunk	(DETMGR_CHUNK_OBJECTS);
        for (DetailIt it=objects.begin(); it!=objects.end(); it++){
        	if (remap_object[it-objects.begin()]!=u8(-1)){
                F.open_chunk	(object_idx++);
                if (!((EDetail*)(*it))->m_pRefs){
                    ELog.DlgMsg(mtError, "Bad object or object not found '%s'.", ((EDetail*)(*it))->m_sRefs.c_str());
                    bRes=false;
                }else{
                    LPCSTR tex_name = ((EDetail*)(*it))->GetTextureName();
                    for (u32 t_idx=0; t_idx<textures.size(); t_idx++) 
                        if (textures[t_idx]==tex_name) break;
                    VERIFY(t_idx<textures.size());
                    t_idx = remap[t_idx];
                    ((EDetail*)(*it))->Export	(F,do_tex_name.c_str(),offsets[t_idx],scales[t_idx],rotated[t_idx]);
                }
                F.close_chunk	();
                if (!bRes) break;
            }
        }
        F.close_chunk		();
    }
    
    pb->Inc	("export slots");
    // slots
    if (bRes){
    	xr_vector<DetailSlot> dt_slots(slot_cnt); dt_slots.assign(dtSlots,dtSlots+slot_cnt);
        for (slot_idx=0; slot_idx<slot_cnt; slot_idx++){
            DetailSlot& it 	= dt_slots[slot_idx];
            // zero colors need lighting
	        it.c_dir		= 0;
	        it.c_hemi		= 0;
	        it.c_r			= 0;
	        it.c_g			= 0;
	        it.c_b			= 0;
            for (int part=0; part<4; part++){
                u8 id		= it.r_id(part);
                if (id!=DetailSlot::ID_Empty) it.w_id(part,remap_object[id]);
            }
        }
		F.open_chunk	(DETMGR_CHUNK_SLOTS);
		F.w				(dt_slots.begin(),dtH.size_x*dtH.size_z*sizeof(DetailSlot));
	    F.close_chunk	();
        pb->Inc();

        // write header
        dtH.version		= DETAIL_VERSION;
        dtH.object_count= object_idx;

        F.w_chunk		(DETMGR_CHUNK_HEADER,&dtH,sizeof(DetailHeader));

    	bRes 			= F.save_to(fn.c_str());
    }

    pb->Inc();
    UI->ProgressEnd(pb);
    return bRes;
}

void EDetailManager::OnDensityChange(PropValue* prop)
{
	InvalidateCache		();
}	


void EDetailManager::OnBaseTextureChange(PropValue* prop)
{
	m_Base.OnImageChange	(prop);
    InvalidateSlots			();
    ELog.DlgMsg				(mtInformation,"Texture changed. Reinitialize objects.");
}

void EDetailManager::FillProp(LPCSTR pref, PropItemVec& items)
{
	PropValue* P;
    P=PHelper().CreateFloat	(items, PrepareKey(pref,"Objects per square"),				&ps_r__Detail_density);
    P->OnChangeEvent.bind	(this,&EDetailManager::OnDensityChange);
    P=PHelper().CreateChoose(items, PrepareKey(pref,"Base Texture"),					&m_Base.name, smTexture);
    P->OnChangeEvent.bind	(this,&EDetailManager::OnBaseTextureChange);
    PHelper().CreateFlag32	(items, PrepareKey(pref,"Common\\Draw objects"),			&m_Flags,	flObjectsDraw);
    PHelper().CreateFlag32	(items, PrepareKey(pref,"Common\\Draw base texture"),		&m_Flags,	flBaseTextureDraw);
    PHelper().CreateFlag32	(items, PrepareKey(pref,"Common\\Base texture blended"),	&m_Flags,	flBaseTextureBlended);
    PHelper().CreateFlag32	(items, PrepareKey(pref,"Common\\Draw slot boxes"),			&m_Flags,	flSlotBoxesDraw);
}

bool EDetailManager::GetSummaryInfo(SSceneSummary* inf)
{
	for (DetailIt it=objects.begin(); it!=objects.end(); it++){
    	((EDetail*)(*it))->OnDeviceCreate();
        CEditableObject* E 	= ((EDetail*)(*it))->m_pRefs;
		if (!E)				continue;
	    CSurface* surf		= *E->FirstSurface(); VERIFY(surf);
		inf->AppendTexture	(surf->_Texture(),SSceneSummary::sttDO,0,0,"$DETAILS$");
    }
    return true;
}

