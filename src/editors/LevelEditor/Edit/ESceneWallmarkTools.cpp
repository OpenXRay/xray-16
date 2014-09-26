//---------------------------------------------------------------------------
#include "stdafx.h"
#pragma hdrstop

#include "ESceneWallmarkTools.h"
#include "ESceneWallmarkControls.h"
#include "Scene.h"
#include "Builder.h"
#include "../ECore/Editor/EditObject.h"
#include "SceneObject.h"
#include "../ECore/Editor/UI_ToolsCustom.h"
#include "scene.h"
#include "../ECore/Editor/UI_Main.h"
#include "../ECore/Editor/D3DUtils.h"
#include "ResourceManager.h"
#include "UI_LevelTools.h"

// chunks
#define WM_VERSION  				0x0004
//----------------------------------------------------
#define WM_CHUNK_VERSION			0x0001       
#define WM_CHUNK_FLAGS				0x0002
#define WM_CHUNK_PARAMS				0x0003
#define WM_CHUNK_ITEMS				0x0004
#define WM_CHUNK_ITEMS2				0x0005
//----------------------------------------------------

#define MAX_WALLMARK_COUNT			500
#define MAX_WALLMARK_VERTEX_COUNT	512
#define COMPILER_SHADER				"def_shaders\\def_vertex_ghost_no_shadow"

ESceneWallmarkTool::ESceneWallmarkTool():ESceneToolBase(OBJCLASS_WM)
{
	m_MarkWidth		= 1.f;
	m_MarkHeight	= 1.f;
    m_MarkRotate	= 0.f;
    m_Flags.assign	(flDrawWallmark);
//.    m_ShName		= "effects\\wallmarkblend";
    m_ShName		= "effects\\wallmarkmult";
    m_TxName		= "";
}

ESceneWallmarkTool::~ESceneWallmarkTool()
{
}

int ESceneWallmarkTool::RaySelect(int flag, float& distance, const Fvector& start, const Fvector& direction, BOOL bDistanceOnly)
{
	if (!m_Flags.is(flDrawWallmark)) return 0;

    wallmark* W		= 0;
    for (WMSVecIt slot_it=marks.begin(); slot_it!=marks.end(); slot_it++){
        wm_slot* slot		= *slot_it;
        for (WMVecIt w_it=slot->items.begin(); w_it!=slot->items.end(); w_it++){
            Fvector 		pt;
            if (Fbox::rpOriginOutside==(*w_it)->bbox.Pick2(start,direction,pt)){
            	float range	= start.distance_to(pt);
                if (range<distance){
		            W		= *w_it;
                	distance= range;
                }
            }
        }
    }
    if (W&&!bDistanceOnly){
    	if (flag==-1)	W->flags.invert(wallmark::flSelected);
        W->flags.set	(wallmark::flSelected,flag);
        return 1;
    }
    return 0;
}

int ESceneWallmarkTool::FrustumSelect(int flag, const CFrustum& frustum)
{
	if (!m_Flags.is(flDrawWallmark)) return 0;

    int count = 0;
    for (WMSVecIt p_it=marks.begin(); p_it!=marks.end(); p_it++){
        for (WMVecIt m_it=(*p_it)->items.begin(); m_it!=(*p_it)->items.end(); m_it++){
        	wallmark* W = *m_it;
	        u32 mask 	= 0xffff;
            if (frustum.testSAABB(W->bounds.P,W->bounds.R,W->bbox.data(),mask)){
                if (-1==flag)	W->flags.invert	(wallmark::flSelected);
                else			W->flags.set	(wallmark::flSelected,flag);
                count++;
            }
        }
    }
    UI->RedrawScene		();
    return count;
}

void ESceneWallmarkTool::SelectObjects(bool flag)
{
	if (!m_Flags.is(flDrawWallmark)) return;
    for (WMSVecIt p_it=marks.begin(); p_it!=marks.end(); p_it++){
        for (WMVecIt m_it=(*p_it)->items.begin(); m_it!=(*p_it)->items.end(); m_it++)
            (*m_it)->flags.set(wallmark::flSelected,flag);
    }
    UI->RedrawScene		();
}

void ESceneWallmarkTool::InvertSelection()
{
	if (!m_Flags.is(flDrawWallmark)) return;
    for (WMSVecIt p_it=marks.begin(); p_it!=marks.end(); p_it++){
        for (WMVecIt m_it=(*p_it)->items.begin(); m_it!=(*p_it)->items.end(); m_it++)
            (*m_it)->flags.invert(wallmark::flSelected);
    }
    UI->RedrawScene		();
}

void ESceneWallmarkTool::RemoveSelection()
{
	if (!m_Flags.is(flDrawWallmark)) return;
    for (WMSVecIt p_it=marks.begin(); p_it!=marks.end(); p_it++){
        for (WMVecIt m_it=(*p_it)->items.begin(); m_it!=(*p_it)->items.end(); )
        	if ((*m_it)->flags.is(wallmark::flSelected)){
            	wm_destroy	(*m_it);
                *m_it		= (*p_it)->items.back();
                (*p_it)->items.pop_back();
            }else{
            	m_it++;
            }
    }
    UI->RedrawScene		();
}

int ESceneWallmarkTool::SelectionCount(bool testflag)
{
	if (!m_Flags.is(flDrawWallmark)) return 0;
	int count = 0;
    for (WMSVecIt p_it=marks.begin(); p_it!=marks.end(); p_it++)
        for (WMVecIt m_it=(*p_it)->items.begin(); m_it!=(*p_it)->items.end(); m_it++)
            if ((*m_it)->flags.is(wallmark::flSelected)) count++;
	return count;
}

void ESceneWallmarkTool::Clear(bool bOnlyNodes)
{
	inherited::Clear	();
	{
		for (WMSVecIt p_it=marks.begin(); p_it!=marks.end(); p_it++){
			for (WMVecIt m_it=(*p_it)->items.begin(); m_it!=(*p_it)->items.end(); m_it++)
				wm_destroy	(*m_it);
			xr_delete(*p_it);
		}
		marks.clear	();
	}
	{
		for (u32 it=0; it<pool.size(); it++)
			xr_delete	(pool[it]);
		pool.clear	();
	}
}

bool ESceneWallmarkTool::Valid		(){return !marks.empty();}
bool ESceneWallmarkTool::IsNeedSave(){return marks.size();}
void ESceneWallmarkTool::OnFrame	(){}

struct zero_slot_pred : public std::unary_function<ESceneWallmarkTool::wm_slot*, bool>
{
	bool operator()(const ESceneWallmarkTool::wm_slot*& x){ return x==0; }
};
void ESceneWallmarkTool::RefiningSlots()
{
    for (WMSVecIt slot_it=marks.begin(); slot_it!=marks.end(); slot_it++){
        wm_slot*& slot		= *slot_it;	
        if (slot->items.empty()) xr_delete(slot);
    }
    WMSVecIt new_end		= std::remove_if(marks.begin(),marks.end(),zero_slot_pred());
    marks.erase				(new_end,marks.end());
}

extern ECORE_API float r_ssaDISCARD;
const int	MAX_R_VERTEX	= 4096;

void ESceneWallmarkTool::OnRender(int priority, bool strictB2F)
{
	if (!m_Flags.is(flDrawWallmark))return;
    if (marks.empty())				return;

    for (WMSVecIt slot_it=marks.begin(); slot_it!=marks.end(); slot_it++){
        wm_slot* slot		= *slot_it;	
        VERIFY(slot->shader);
		if ((u32(priority)==slot->shader->E[0]->flags.iPriority)&&(strictB2F==!!(slot->shader->E[0]->flags.bStrictB2F))){
            // Projection and xform
            float _43				= EDevice.mProject._43;
            EDevice.mProject._43 	-= 0.01f;
            RCache.set_xform_world	(Fidentity);
            RCache.set_xform_project(EDevice.mProject);

            float	ssaCLIP		   	= r_ssaDISCARD/4;

            u32			w_offset   	= 0;
            FVF::LIT*	w_verts 	= (FVF::LIT*) RCache.Vertex.Lock(MAX_R_VERTEX,hGeom->vb_stride,w_offset);
            FVF::LIT*	w_start 	= w_verts;

            for (WMVecIt w_it=slot->items.begin(); w_it!=slot->items.end(); w_it++){
                wallmark* W			= *w_it;
                VERIFY3				(W->verts.size()<=MAX_R_VERTEX,"ERROR: Invalid wallmark.",*slot->tx_name);
                if (RImplementation.ViewBase.testSphere_dirty(W->bounds.P,W->bounds.R)){
                    float dst		= EDevice.vCameraPosition.distance_to_sqr(W->bounds.P);
                    float ssa		= W->bounds.R * W->bounds.R / dst;
                    if (ssa>=ssaCLIP){
                        // fill wallmark
                        u32	C		= color_rgba(255,255,255,255);
                        int t_cnt	= W->verts.size()/3;
                        for (int t_idx=0; t_idx<t_cnt; t_idx++){
	                        u32 w_count	= u32(w_verts-w_start);
                            if (w_count+3>MAX_R_VERTEX){
                                // Flush stream
                                RCache.Vertex.Unlock   	(w_count,hGeom->vb_stride);
                                RCache.set_Shader		(slot->shader);
                                RCache.set_Geometry		(hGeom);
                                RCache.Render			(D3DPT_TRIANGLELIST,w_offset,w_count/3);
                                // Restart (re-lock/re-calc)
                                w_verts					= (FVF::LIT*)	RCache.Vertex.Lock	(MAX_R_VERTEX,hGeom->vb_stride,w_offset);
                                w_start					= w_verts;
                            }
                            // real fill buffer
                            FVF::LIT* S		= W->verts.begin()+t_idx*3;
                        	for (int k=0; k<3; k++,S++,w_verts++){
                                w_verts->p.set	(S->p);
                                w_verts->color	= C;
                                w_verts->t.set	(S->t);
                            }
                        }
                    }
                }
            }
            // Flush stream
            u32 w_count				= u32(w_verts-w_start);
            RCache.Vertex.Unlock	(w_count,hGeom->vb_stride);
            if (w_count)			
            {
				RCache.set_Shader	(slot->shader);
                RCache.set_Geometry	(hGeom);
                RCache.Render		(D3DPT_TRIANGLELIST,w_offset,w_count/3);
            }
            // Projection
            EDevice.mProject._43		= _43;
            RCache.set_xform_project	(EDevice.mProject);
        }
        if ((1==priority)&&(false==strictB2F)){
            for (WMVecIt w_it=slot->items.begin(); w_it!=slot->items.end(); w_it++){
                wallmark* W		= *w_it;
                if (W->flags.is(wallmark::flSelected))
                    if (RImplementation.ViewBase.testSphere_dirty(W->bounds.P,W->bounds.R))
                        DU_impl.DrawSelectionBoxB(W->bbox);
            }
        }
    }
}

struct zero_item_pred : public std::unary_function<ESceneWallmarkTool::wallmark*, bool>
{
	bool operator()(const ESceneWallmarkTool::wallmark*& x){ return x==0; }
};
bool ESceneWallmarkTool::LoadLTX(CInifile& ini)
{
	R_ASSERT(0);
	return true;
}
void ESceneWallmarkTool::SaveLTX(CInifile& ini, int id)
{
	inherited::SaveLTX	(ini, id);

    ini.w_u32			("main", "version", WM_VERSION);

    ini.w_u32			("main", "flags",m_Flags.get());

    ini.w_float			("main", "mark_width", m_MarkWidth);
    ini.w_float			("main", "mark_height", m_MarkHeight);
    ini.w_float			("main", "mark_rotate", m_MarkRotate);
    ini.w_string		("main", "sh_name", m_ShName.c_str());
    ini.w_string		("main", "tx_name", m_TxName.c_str());

	u32 i				= 0;
    string128			buff, buff2;
    for (WMSVecIt slot_it=marks.begin(); slot_it!=marks.end(); ++slot_it, ++i)
    {
        wm_slot* slot	= *slot_it;

        sprintf			(buff,"slot_%d", i);
        ini.w_u32		(buff, "items_count", slot->items.size());
        if (slot->items.size()==0)
        	continue;

        ini.w_string		(buff, "sh_name", slot->sh_name.c_str());
        ini.w_string		(buff, "tx_name", slot->tx_name.c_str());

        u32 ii				= 0;
        for (WMVecIt w_it=slot->items.begin(); w_it!=slot->items.end(); ++w_it, ++ii)
        {
            wallmark* W		= *w_it;
        	sprintf			(buff2,"itm_%d_flags", ii);
            ini.w_u32		(buff, buff2, W->flags.get());

        	sprintf			(buff2,"itm_%d_bb_min", ii);
            ini.w_fvector3	(buff, buff2, W->bbox.min);
        	sprintf			(buff2,"itm_%d_bb_max", ii);
            ini.w_fvector3	(buff, buff2, W->bbox.max);

        	sprintf			(buff2,"itm_%d_bsphere_p", ii);
            ini.w_fvector3	(buff, buff2, W->bounds.P);
        	sprintf			(buff2,"itm_%d_bsphere_r", ii);
            ini.w_float		(buff, buff2, W->bounds.R);

        	sprintf			(buff2,"itm_%d_w", ii);
            ini.w_float		(buff, buff2, W->w);
        	sprintf			(buff2,"itm_%d_h", ii);
            ini.w_float		(buff, buff2, W->h);
        	sprintf			(buff2,"itm_%d_r", ii);
            ini.w_float		(buff, buff2, W->r);

        	sprintf			(buff2,"itm_%d_vert_cnt", ii);
            ini.w_u32		(buff, buff2, W->verts.size());

            R_ASSERT2		(0, "not_implemented");
//.            F.w				(&*W->verts.begin(),sizeof(FVF::LIT)*W->verts.size());
        }
    }
}

bool ESceneWallmarkTool::LoadStream(IReader& F)
{
	inherited::LoadStream	(F);

	u16 version = 0;

    R_ASSERT(F.r_chunk(WM_CHUNK_VERSION,&version));


    if(version!=0x0003 && version!=WM_VERSION)
    {
        ELog.Msg( mtError, "Static Wallmark: Unsupported version.");
        return false;
    }

    R_ASSERT(F.find_chunk(WM_CHUNK_FLAGS));
    F.r				(&m_Flags,sizeof(m_Flags));

    R_ASSERT(F.find_chunk(WM_CHUNK_PARAMS));
    m_MarkWidth		= F.r_float	();
    m_MarkHeight	= F.r_float	();
    m_MarkRotate	= F.r_float	();
    F.r_stringZ		(m_ShName);

    if(version==0x0003)
       m_ShName		= "effects\\wallmarkmult";

    F.r_stringZ		(m_TxName);

    IReader* OBJ 	= F.open_chunk(WM_CHUNK_ITEMS);
    if (OBJ){
        IReader* O  = OBJ->open_chunk(0);
        for (int count=1; O; count++) {
            u32 item_count	= O->r_u32();	
            if (item_count){
                shared_str		tex_name,sh_name;
                O->r_stringZ	(sh_name);
                O->r_stringZ	(tex_name);
                wm_slot* slot	= AppendSlot(sh_name,tex_name);
                if (slot){
                    slot->items.resize(item_count);
                    for (WMVecIt w_it=slot->items.begin(); w_it!=slot->items.end(); w_it++){
                        *w_it	= wm_allocate();
                        wallmark* W	= *w_it;
                        O->r	    (&W->flags,sizeof(W->flags));
                        O->r	    (&W->bbox,sizeof(W->bbox));
                        O->r	    (&W->bounds,sizeof(W->bounds));
                        W->parent	= slot;
                        W->w 	    = 1.f;
                        W->h 	    = 1.f;
                        W->r 	    = 1.f;
                        W->verts.resize(O->r_u32());
                        O->r	(&*W->verts.begin(),sizeof(FVF::LIT)*W->verts.size());
                    }
                }
            }
            O->close();
            O = OBJ->open_chunk(count);
        }
        OBJ->close();
    }else{
        IReader* OBJ 	= F.open_chunk(WM_CHUNK_ITEMS2);
        if (OBJ){
            IReader* O  = OBJ->open_chunk(0);
            for (int count=1; O; count++) {
                u32 item_count	= O->r_u32();	
                if (item_count){
                    shared_str		tex_name,sh_name;
                    O->r_stringZ	(sh_name);
                    O->r_stringZ	(tex_name);
                    wm_slot* slot	= AppendSlot(sh_name,tex_name);
                    if (slot){
                        slot->items.resize(item_count);
                        for (WMVecIt w_it=slot->items.begin(); w_it!=slot->items.end(); w_it++){
                            *w_it	= wm_allocate();
                            wallmark* W	= *w_it;
                            O->r	    (&W->flags,sizeof(W->flags));
                            O->r	    (&W->bbox,sizeof(W->bbox));
                            O->r	    (&W->bounds,sizeof(W->bounds));
	                        W->parent	= slot;
                            W->w 	    = O->r_float();
                            W->h 	    = O->r_float();
                            W->r 	    = O->r_float();
                            W->verts.resize(O->r_u32());
                            O->r	(&*W->verts.begin(),sizeof(FVF::LIT)*W->verts.size());
                        }
                    }
                }
                O->close();
                O = OBJ->open_chunk(count);
            }
            OBJ->close();
        }
    }

    // validate wallmarks
    for (WMSVecIt slot_it=marks.begin(); slot_it!=marks.end(); slot_it++){
        wm_slot* slot		= *slot_it;	
        for (WMVecIt w_it=slot->items.begin(); w_it!=slot->items.end(); w_it++){
            wallmark*& W	= *w_it;
            if (W->verts.size()>MAX_WALLMARK_VERTEX_COUNT){
                ELog.Msg	(mtError,"ERROR: Invalid wallmark (Contain more than %d vertices). Removed.", MAX_WALLMARK_VERTEX_COUNT);
                wm_destroy	(W);
                W			= 0;
            }
        }
        WMVecIt new_end		= std::remove_if(slot->items.begin(),slot->items.end(),zero_item_pred());
	    slot->items.erase	(new_end,slot->items.end());
    }
    
    return true;
}

void ESceneWallmarkTool::SaveStream(IWriter& F)
{
	inherited::SaveStream	(F);

	F.open_chunk	(WM_CHUNK_VERSION);
    F.w_u16			(WM_VERSION);
	F.close_chunk	();

	F.open_chunk	(WM_CHUNK_FLAGS);
    F.w				(&m_Flags,sizeof(m_Flags));
	F.close_chunk	();

	F.open_chunk	(WM_CHUNK_PARAMS);
    F.w_float		(m_MarkWidth);
    F.w_float		(m_MarkHeight);
    F.w_float		(m_MarkRotate);
    F.w_stringZ		(m_ShName);
    F.w_stringZ		(m_TxName);
	F.close_chunk	();

	F.open_chunk	(WM_CHUNK_ITEMS2);
    for (WMSVecIt slot_it=marks.begin(); slot_it!=marks.end(); slot_it++){
		F.open_chunk(slot_it-marks.begin());
        wm_slot* slot= *slot_it;	
        F.w_u32		(slot->items.size());
        if (slot->items.size()){
            F.w_stringZ	(slot->sh_name);
            F.w_stringZ	(slot->tx_name);
            for (WMVecIt w_it=slot->items.begin(); w_it!=slot->items.end(); w_it++){
                wallmark* W	= *w_it;
                F.w			(&W->flags,sizeof(W->flags));
                F.w			(&W->bbox,sizeof(W->bbox));
                F.w			(&W->bounds,sizeof(W->bounds));
                F.w_float	(W->w);
                F.w_float	(W->h);
                F.w_float	(W->r);
                F.w_u32		(W->verts.size());
                F.w			(&*W->verts.begin(),sizeof(FVF::LIT)*W->verts.size());
            }
        }
		F.close_chunk();
    }
	F.close_chunk	();
}

bool ESceneWallmarkTool::LoadSelection(IReader& F)
{
	Clear();
	return LoadStream(F);
}

void ESceneWallmarkTool::SaveSelection(IWriter& F)
{
	SaveStream(F);
}

bool ESceneWallmarkTool::Export(LPCSTR path)
{
	RefiningSlots		();
    
    AnsiString fn		= AnsiString(path)+"level.wallmarks";
	IWriter*	F		= FS.w_open(fn.c_str()); R_ASSERT(F);
                             
    F->open_chunk		(1);
    F->w_u32			(marks.size());
    for (WMSVecIt slot_it=marks.begin(); slot_it!=marks.end(); slot_it++){
        wm_slot* slot= *slot_it;	
        F->w_u32		(slot->items.size());
        if (slot->items.size()){
            F->w_stringZ(slot->sh_name);
            F->w_stringZ(slot->tx_name);
            for (WMVecIt w_it=slot->items.begin(); w_it!=slot->items.end(); w_it++){
                wallmark* W	= *w_it;
                F->w	(&W->bounds,sizeof(W->bounds));
                F->w_u32(W->verts.size());
                F->w	(&*W->verts.begin(),sizeof(FVF::LIT)*W->verts.size());
            }
        }
    }
    F->close_chunk	();

    FS.w_close		(F);
    
	return true;
}                

void ESceneWallmarkTool::OnDeviceCreate()
{
	hGeom.create	(FVF::F_LIT, RCache.Vertex.Buffer(), NULL);
}

void ESceneWallmarkTool::OnDeviceDestroy()
{
	hGeom.destroy	();
}

void ESceneWallmarkTool::OnSynchronize()
{
}

// allocate
ESceneWallmarkTool::wallmark*	ESceneWallmarkTool::wm_allocate		()
{
	wallmark*			W = 0;
	if (pool.empty())	W = xr_new<wallmark> ();
	else				{ W = pool.back(); pool.pop_back(); }

	W->verts.clear		();
	return W;
}
// destroy
void		ESceneWallmarkTool::wm_destroy		(wallmark*	W	)
{
	pool.push_back		(W);
}

struct SWMSlotFindPredicate {
	shared_str			sh_name;
	shared_str			tx_name;
						SWMSlotFindPredicate(shared_str sh, shared_str tx):sh_name(sh),tx_name(tx){}
	bool				operator()			(const ESceneWallmarkTool::wm_slot* slot) const
	{
		return			(slot->tx_name==tx_name)&&(slot->sh_name==sh_name);
	}
};
ESceneWallmarkTool::wm_slot* ESceneWallmarkTool::FindSlot	(shared_str sh_name, shared_str tx_name)
{
	WMSVecIt it				= std::find_if(marks.begin(),marks.end(),SWMSlotFindPredicate(sh_name,tx_name));
	return					(it!=marks.end())?*it:0;
}
ESceneWallmarkTool::wm_slot* ESceneWallmarkTool::AppendSlot(shared_str sh_name, shared_str tx_name)
{
	wm_slot* slot			= xr_new<wm_slot>(sh_name,tx_name);
    if (0==slot->shader)	xr_delete(slot);
    else marks.push_back	(slot);
    return slot;
}

void ESceneWallmarkTool::RecurseTri(u32 t, Fmatrix &mView, wallmark &W)
{
	CDB::TRI*	T			= sml_collector.getT()+t;
	if (T->dummy)			return;
	T->dummy				= 0xffffffff;
	
	// Some vars
	u32*		v_ids		= T->verts;
	Fvector*	v_data		= sml_collector.getV();
	sml_poly_src.clear		();
	sml_poly_src.push_back	(v_data[v_ids[0]]);
	sml_poly_src.push_back	(v_data[v_ids[1]]);
	sml_poly_src.push_back	(v_data[v_ids[2]]);
	sml_poly_dest.clear		();
	
	sPoly* P = sml_clipper.ClipPoly	(sml_poly_src, sml_poly_dest);
	
	if (P) {
		// Create vertices and triangulate poly (tri-fan style triangulation)
		FVF::LIT			V0,V1,V2;
		Fvector				UV;

		mView.transform_tiny(UV, (*P)[0]);
		V0.set				((*P)[0],0,(1+UV.x)*.5f,(1-UV.y)*.5f);
		mView.transform_tiny(UV, (*P)[1]);
		V1.set				((*P)[1],0,(1+UV.x)*.5f,(1-UV.y)*.5f);

		for (u32 i=2; i<P->size(); i++)
		{
			mView.transform_tiny(UV, (*P)[i]);
			V2.set				((*P)[i],0,(1+UV.x)*.5f,(1-UV.y)*.5f);
			W.verts.push_back	(V0);
			W.verts.push_back	(V1);
			W.verts.push_back	(V2);
			V1					= V2;
		}
		
		// recurse
		for (i=0; i<3; i++)
		{
			u32 adj					= sml_adjacency[3*t+i];
			if (0xffffffff==adj)	continue;
			CDB::TRI*	SML			= sml_collector.getT() + adj;
			v_ids					= SML->verts;

			Fvector test_normal;
			test_normal.mknormal	(v_data[v_ids[0]],v_data[v_ids[1]],v_data[v_ids[2]]);
			float cosa				= test_normal.dotproduct(sml_normal);
			if (cosa<EPS)			continue;
			RecurseTri				(adj,mView,W);
		}
	}
}

void ESceneWallmarkTool::BuildMatrix	(Fmatrix &mView, float inv_w, float inv_h, float angle, const Fvector& from)
{
	// build projection
	Fmatrix				mScale,mRot;
    Fvector				at,up,right,y;
	at.sub				(from,sml_normal);
	y.set				(EDevice.vCameraTop);

    if (m_Flags.is(flAxisAlign)){
        y.set			(0,1,0);
        if (_abs(sml_normal.y)>0.99f) y.set(1,0,0);
    }else{
        y.set			(EDevice.vCameraTop);
        if (fsimilar(y.dotproduct(sml_normal),1.f,EPS)) y.set(EDevice.vCameraRight);
    }
	right.crossproduct	(y,sml_normal);
	up.crossproduct		(sml_normal,right);
	mView.build_camera	(from,at,up);
	mRot.rotateZ		(angle);
	mView.mulA_43		(mRot);
	mScale.scale		(inv_w,inv_h,_max(inv_w,inv_h));
	mView.mulA_43		(mScale);
}

int	ESceneWallmarkTool::ObjectCount()
{
	int count 			= 0;
    for (WMSVecIt p_it=marks.begin(); p_it!=marks.end(); p_it++)
        count			+= (*p_it)->items.size();
	return count;
}

BOOL ESceneWallmarkTool::AddWallmark_internal(const Fvector& start, const Fvector& dir, shared_str sh, shared_str tx, float width, float height, float rotate)
{
	if (ObjectCount()>=MAX_WALLMARK_COUNT){
    	ELog.DlgMsg			(mtError,"Maximum wallmark per level is reached [Max: %d].",MAX_WALLMARK_COUNT);
    	return FALSE;
    }
    
    if (0==sh.size()){
    	ELog.DlgMsg			(mtError,"Select texture before add wallmark.");
    	return 				FALSE;
    }
    if (0==tx.size()){
    	ELog.DlgMsg			(mtError,"Select texture before add wallmark.");
    	return 				FALSE;
    }
    // pick contact poly
    Fvector 				contact_pt;
    float dist				= UI->ZFar();
    ObjectList* snap_list	= Scene->GetSnapList(false);
    if (!snap_list){
    	ELog.DlgMsg			(mtError,"Fill and activate snap list.");
    	return 				FALSE;
    }
    // pick contact poly
    SPickQuery				PQ;
    sml_collector.clear		();
    if (Scene->RayQuery(PQ,start,dir,dist,CDB::OPT_ONLYNEAREST|CDB::OPT_CULL,snap_list)){ 
        contact_pt.mad		(PQ.m_Start,PQ.m_Direction,PQ.r_begin()->range); 
        sml_normal.mknormal	(PQ.r_begin()->verts[0],PQ.r_begin()->verts[1],PQ.r_begin()->verts[2]);
        sml_collector.add_face_packed_D	(PQ.r_begin()->verts[0],PQ.r_begin()->verts[1],PQ.r_begin()->verts[2],0);
    }else return FALSE;

    // box pick poly
    Fbox					bbox;
    bbox.set				(contact_pt,contact_pt);
    bbox.grow				(_max(height,width)*2);
    SPickQuery				BQ;
    if (Scene->BoxQuery(BQ,bbox,CDB::OPT_FULL_TEST,snap_list)){ 
    	for (u32 k=0; k<(u32)BQ.r_count(); k++){
        	SPickQuery::SResult* R = BQ.r_begin()+k;
			Fvector test_normal;
			test_normal.mknormal	(R->verts[0],R->verts[1],R->verts[2]);
			float cosa				= test_normal.dotproduct(sml_normal);
			if (cosa<0.1)			continue;
	        sml_collector.add_face_packed_D	(R->verts[0],R->verts[1],R->verts[2],0);
        }
    }

    // remove duplicate poly
	sml_collector.remove_duplicate_T	();
    // calculate adjacency
    sml_collector.calc_adjacency		(sml_adjacency);
    
	// build 3D ortho-frustum
	Fmatrix				mView;
	BuildMatrix			(mView,2/width,2/height,rotate,contact_pt); // width/2 height/2  (BuildMatrix need radius)
	sml_clipper.CreateFromMatrix (mView,FRUSTUM_P_LRTB);

	// create wallmark
	wallmark* W			= wm_allocate();
    W->w				= width;
    W->h				= height;
    W->r				= rotate;

    RecurseTri			(0,mView,*W);

	// calc sphere
	if ((W->verts.size()<3) || (W->verts.size()>MAX_WALLMARK_VERTEX_COUNT)) { 
    	ELog.DlgMsg		(mtError,"Invalid wallmark vertex count. [Min: %d. Max: %d].",3,MAX_WALLMARK_VERTEX_COUNT);
    	wm_destroy		(W); 
        return 			FALSE; 
    }else{
		W->bbox.invalidate();
		FVF::LIT* I=&*W->verts.begin	();
		FVF::LIT* E=&*W->verts.end		();
		for (; I!=E; I++) W->bbox.modify(I->p);
		W->bbox.getsphere				(W->bounds.P,W->bounds.R);
        W->flags.assign					(wallmark::flSelected);
        W->bbox.grow					(EPS_L);
	}

	// search if similar wallmark exists
	wm_slot* slot	= FindSlot(sh,tx);
	if (slot){
	    W->parent	= slot;
		WMVecIt		it	= slot->items.begin	();
		WMVecIt		end	= slot->items.end	();
		for (; it!=end; it++)
		{
			wallmark* wm	= *it;
			if (wm->bounds.P.similar(W->bounds.P,0.02f)){ // replace
				wm_destroy	(wm);
				*it			= W;
				return TRUE;
			}
		}
	}else{
		slot		= AppendSlot(sh,tx);
	    W->parent	= slot;
	}

	// no similar - register _new_
	if (slot) slot->items.push_back(W);
    return TRUE;
}

BOOL ESceneWallmarkTool::AddWallmark	(const Fvector& start, const Fvector& dir)
{
	return AddWallmark_internal(start,dir,m_ShName,m_TxName,m_MarkWidth,m_MarkHeight,m_MarkRotate);
}

BOOL ESceneWallmarkTool::MoveSelectedWallmarkTo(const Fvector& start, const Fvector& dir)
{
	if (!m_Flags.is(flDrawWallmark)) return 0;
    wallmark* wm	= 0;
    for (WMSVecIt p_it=marks.begin(); p_it!=marks.end(); p_it++){
        for (WMVecIt m_it=(*p_it)->items.begin(); m_it!=(*p_it)->items.end(); m_it++){
            if ((*m_it)->flags.is(wallmark::flSelected)){ 
            	if (wm) return FALSE;
                wm 	= *m_it;
            }
        }
    }
	if ((0!=wm)&&AddWallmark_internal(start,dir,wm->parent->sh_name,wm->parent->tx_name,wm->w,wm->h,wm->r)){
        // remove wm
        for (WMSVecIt p_it=marks.begin(); p_it!=marks.end(); p_it++){
            for (WMVecIt m_it=(*p_it)->items.begin(); m_it!=(*p_it)->items.end(); ){
            	if (*m_it==wm){
                    wm_destroy	(wm);
                    *m_it		= (*p_it)->items.back();
                    (*p_it)->items.pop_back();
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

void ESceneWallmarkTool::FillProp(LPCSTR pref, PropItemVec& items)
{
    PHelper().CreateFlag32		(items, PrepareKey(pref,"Common\\Draw Wallmarks"),	&m_Flags, 		flDrawWallmark);
    PHelper().CreateFlag32		(items,	PrepareKey(pref,"Common\\Alignment"),		&m_Flags, 		flAxisAlign, "By Camera", "By World Axis");
    PHelper().CreateFloat	 	(items, PrepareKey(pref,"Common\\Width"),			&m_MarkWidth, 	0.01f, 10.f);
    PHelper().CreateFloat	 	(items, PrepareKey(pref,"Common\\Height"),			&m_MarkHeight, 	0.01f, 10.f);
    PHelper().CreateAngle	 	(items, PrepareKey(pref,"Common\\Rotate"),			&m_MarkRotate);
    PHelper().CreateChoose		(items, PrepareKey(pref,"Common\\Shader"),			&m_ShName, 		smEShader);
    PHelper().CreateChoose		(items, PrepareKey(pref,"Common\\Texture"),			&m_TxName, 		smTexture);
}
//----------------------------------------------------

bool ESceneWallmarkTool::Validate(bool)
{
	bool bRes = true;

    for (WMSVecIt slot_it=marks.begin(); slot_it!=marks.end(); slot_it++){
        wm_slot* slot= *slot_it;	
        if (slot->items.size()){
            IBlender* 	B 	= EDevice.Resources->_FindBlender(*slot->sh_name); 
            if (!B||B->canBeLMAPped()){
                ELog.Msg	(mtError,"Wallmarks: Invalid or missing shader '%s'.",*slot->sh_name);
                bRes 		= false;
            }
        }
    }

    return bRes;
}
//----------------------------------------------------

void ESceneWallmarkTool::GetStaticDesc(int& v_cnt, int& f_cnt, bool b_selected_only, bool b_cform)
{
    if(b_cform)
    		return;
    for (WMSVecIt slot_it=marks.begin(); slot_it!=marks.end(); slot_it++)
    {
        wm_slot* slot		= *slot_it;
        for (WMVecIt w_it=slot->items.begin(); w_it!=slot->items.end(); w_it++)
        {
            wallmark* W		= *w_it;

            if(b_selected_only && !W->flags.test(wallmark::flSelected))
            	continue;

            v_cnt			+= W->verts.size();
            f_cnt			+= W->verts.size()/3;
        }
    }
}
//----------------------------------------------------

bool ESceneWallmarkTool::ExportStatic(SceneBuilder* B, bool b_selected_only)
{
    for (WMSVecIt slot_it=marks.begin(); slot_it!=marks.end(); slot_it++){
        wm_slot* slot		= *slot_it;	
        for (WMVecIt w_it=slot->items.begin(); w_it!=slot->items.end(); w_it++){
            wallmark* W		= *w_it;
		    int sect_num	= B->CalculateSector(W->bounds.P,W->bounds.R);
	        int m_id		= B->BuildMaterial	(*slot->sh_name,COMPILER_SHADER,*slot->tx_name,1,sect_num,false);
            u32 f_cnt 		= W->verts.size()/3;
            for (u32 f_it=0; f_it<f_cnt; f_it++,B->l_face_it++){
                R_ASSERT(B->l_face_it<B->l_face_cnt);
                b_face& dst_f		= B->l_faces[B->l_face_it];
            	for (u32 k=0; k<3; k++,B->l_vert_it++){
			    	R_ASSERT(B->l_vert_it<B->l_vert_cnt);
                	FVF::LIT& src	= W->verts[f_it*3+k];
                    Fvector& dst_v	= B->l_verts[B->l_vert_it];
	            	dst_v.set 		(src.p);
		            dst_f.v[k] 		= B->l_vert_it;
                    dst_f.t[k].set	(src.t);
                    dst_f.dwMaterial= (u16)m_id;
                }
            }
        }
    }
	return true;
}
//----------------------------------------------------


void ESceneWallmarkTool::CreateControls()
{
	inherited::CreateDefaultControls(estDefault);
	// node tools
    AddControl(xr_new<TUI_ControlWallmarkAdd>		(0,		etaAdd, 	this));
    AddControl(xr_new<TUI_ControlWallmarkMove>		(0,		etaMove,	this));
}
//----------------------------------------------------
 
void ESceneWallmarkTool::RemoveControls()
{
	inherited::RemoveControls();
}
//----------------------------------------------------

void ESceneWallmarkTool::GetBBox(Fbox& bb, bool bSelOnly)
{
    if (bSelOnly){
        for (WMSVecIt slot_it=marks.begin(); slot_it!=marks.end(); slot_it++){
            wm_slot* slot		= *slot_it;	
            for (WMVecIt w_it=slot->items.begin(); w_it!=slot->items.end(); w_it++){
                wallmark* W		= *w_it;
                if (W->flags.is(wallmark::flSelected)) bb.merge	(W->bbox);
            }
        }
    }else{
        for (WMSVecIt slot_it=marks.begin(); slot_it!=marks.end(); slot_it++){
            wm_slot* slot		= *slot_it;	
            for (WMVecIt w_it=slot->items.begin(); w_it!=slot->items.end(); w_it++)
                bb.merge	((*w_it)->bbox);
        }
    }
}

