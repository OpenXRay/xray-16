#include "stdafx.h"
#pragma hdrstop

#include "Scene.h"
#include "SceneObject.h"
#include "bottombar.h"
#include "d3dutils.h"

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
#define RENDER_OBJECT(P,B)\
{\
    try{\
        (N->val)->RenderRoot(P,B);\
    }catch(...){\
        ELog.DlgMsg(mtError, "Please notify AlexMX!!! Critical error has occured in render routine!!! [Type B] - Tools: '%s' Object: '%s'",(N->val)->ParentTool->ClassName(),(N->val)->Name);\
    }\
}
    
void __fastcall object_Normal_0(EScene::mapObject_Node *N)	 {RENDER_OBJECT(0,false); }
void __fastcall object_Normal_1(EScene::mapObject_Node *N)	 {RENDER_OBJECT(1,false); }
void __fastcall object_Normal_2(EScene::mapObject_Node *N)	 {RENDER_OBJECT(2,false); }
void __fastcall object_Normal_3(EScene::mapObject_Node *N)	 {RENDER_OBJECT(3,false); }
//------------------------------------------------------------------------------
void __fastcall object_StrictB2F_0(EScene::mapObject_Node *N){RENDER_OBJECT(0,true);}
void __fastcall object_StrictB2F_1(EScene::mapObject_Node *N){RENDER_OBJECT(1,true);}
void __fastcall object_StrictB2F_2(EScene::mapObject_Node *N){RENDER_OBJECT(2,true);}
void __fastcall object_StrictB2F_3(EScene::mapObject_Node *N){RENDER_OBJECT(3,true);}
//------------------------------------------------------------------------------

#define RENDER_SCENE_TOOLS(P,B)\
	{\
		SceneMToolsIt s_it 	= scene_tools.begin();\
		SceneMToolsIt s_end	= scene_tools.end();\
        for (; s_it!=s_end; s_it++){\
            EDevice.SetShader		(B?EDevice.m_SelectionShader:EDevice.m_WireShader);\
            RCache.set_xform_world	(Fidentity);\
            try{\
            	(*s_it)->OnRenderRoot(P,B);\
            }catch(...){\
		        ELog.DlgMsg(mtError, "Please notify AlexMX!!! Critical error has occured in render routine!!! [Type B] - Tools: '%s'",(*s_it)->ClassName());\
            }\
        }\
    }

void EScene::RenderSky(const Fmatrix& camera)
{
	if( !valid() )	return;

//	draw sky
/*
//.
	if (m_SkyDome&&fraBottomBar->miDrawSky->Checked){
        st_Environment& E = m_LevelOp.m_Envs[m_LevelOp.m_CurEnv];
        m_SkyDome->PPosition = camera.c;
        m_SkyDome->UpdateTransform(true);
		EDevice.SetRS(D3DRS_TEXTUREFACTOR, E.m_SkyColor.get());
    	m_SkyDome->RenderSingle();
	    EDevice.SetRS(D3DRS_TEXTUREFACTOR,	0xffffffff);
    }
*/
}
//------------------------------------------------------------------------------

struct tools_rp_pred : public std::binary_function<ESceneToolBase*, ESceneToolBase*, bool>
{
    IC bool operator()(ESceneToolBase* x, ESceneToolBase* y) const
    {	return x->RenderPriority()<y->RenderPriority();	}
};

#define DEFINE_MSET_PRED(T,N,I,P)	typedef xr_multiset< T, P > N;		typedef N::iterator I;

DEFINE_MSET_PRED(ESceneToolBase*,SceneMToolsSet,SceneMToolsIt,tools_rp_pred);
DEFINE_MSET_PRED(ESceneCustomOTool*,SceneOToolsSet,SceneOToolsIt,tools_rp_pred);


void EScene::Render( const Fmatrix& camera )
{
	if( !valid() )	return;

//	if( locked() )	return;

    // extract and sort object tools
    SceneOToolsSet object_tools;
    SceneMToolsSet scene_tools;
    {
        SceneToolsMapPairIt t_it 	= m_SceneTools.begin();
        SceneToolsMapPairIt t_end 	= m_SceneTools.end();
        for (; t_it!=t_end; t_it++)
            if (t_it->second){
            	// before render
            	t_it->second->BeforeRender(); 
                // sort tools
                ESceneCustomOTool* mt = dynamic_cast<ESceneCustomOTool*>(t_it->second);
                if (mt)           	object_tools.insert(mt);
                scene_tools.insert	(t_it->second);
            }
    }

    // insert objects
    {
	    SceneOToolsIt t_it	= object_tools.begin();
	    SceneOToolsIt t_end	= object_tools.end();
        for (; t_it!=t_end; t_it++)
        {
            ObjectList& lst = (*t_it)->GetObjects();
            ObjectIt o_it 	= lst.begin();
            ObjectIt o_end 	= lst.end();
            for(;o_it!=o_end;o_it++){
                if( (*o_it)->Visible()&& (*o_it)->IsRender() ){
                    float distSQ = EDevice.vCameraPosition.distance_to_sqr((*o_it)->FPosition);
                    mapRenderObjects.insertInAnyWay(distSQ,*o_it);
                }
            }
        }
    }
    
// priority #0
    // normal
    mapRenderObjects.traverseLR		(object_Normal_0);
    RENDER_SCENE_TOOLS				(0,false);
    // alpha
    mapRenderObjects.traverseRL		(object_StrictB2F_0);
    RENDER_SCENE_TOOLS				(0,true);

// priority #1
    // normal
    mapRenderObjects.traverseLR		(object_Normal_1);
    RENDER_SCENE_TOOLS				(1,false);
    // alpha
    mapRenderObjects.traverseRL		(object_StrictB2F_1);
    RENDER_SCENE_TOOLS				(1,true);
// priority #2
    // normal
    mapRenderObjects.traverseLR		(object_Normal_2);
    RENDER_SCENE_TOOLS				(2,false);
    // alpha
    mapRenderObjects.traverseRL		(object_StrictB2F_2);
    RENDER_SCENE_TOOLS				(2,true);
// priority #3
    // normal
    mapRenderObjects.traverseLR		(object_Normal_3);
    RENDER_SCENE_TOOLS				(3,false);
    // alpha
    mapRenderObjects.traverseRL		(object_StrictB2F_3);
    RENDER_SCENE_TOOLS				(3,true);

    // render snap
    RenderSnapList			();

    // clear
    mapRenderObjects.clear			();


    SceneMToolsIt s_it 	= scene_tools.begin();
    SceneMToolsIt s_end	= scene_tools.end();
    for (; s_it!=s_end; s_it++) (*s_it)->AfterRender();
}
//------------------------------------------------------------------------------

 
