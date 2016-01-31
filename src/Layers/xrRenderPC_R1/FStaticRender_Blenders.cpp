#include "stdafx.h"
#include "blenderdefault.h"
#include "blender_default_aref.h"
#include "blender_vertex.h"
#include "blender_vertex_aref.h"
#include "Layers/xrRender/blender_screen_set.h"
#include "blender_screen_gray.h"
#include "Layers/xrRender/blender_editor_wire.h"
#include "Layers/xrRender/blender_editor_selection.h"
#include "blender_LaEmB.h"
#include "Layers/xrRender/blender_Lm(EbB).h"
#include "Layers/xrRender/blender_BmmD.h"
#include "blender_shadow_world.h"
#include "blender_blur.h"
#include "blender_model.h"
#include "Layers/xrRender/Blender_Model_EbB.h"
#include "Layers/xrRender/Blender_detail_still.h"
#include "Layers/xrRender/Blender_tree.h"
#include "Layers/xrRender/Blender_Particle.h"

IBlender*	CRender::blender_create	(CLASS_ID cls)
{	
	switch (cls)
	{
	case B_DEFAULT:			return new CBlender_default			();		
	case B_DEFAULT_AREF:	return new CBlender_default_aref	();	
	case B_VERT:			return new CBlender_Vertex			();		
	case B_VERT_AREF:		return new CBlender_Vertex_aref		();	
	case B_SCREEN_SET:		return new CBlender_Screen_SET		();	
	case B_SCREEN_GRAY:		return new CBlender_Screen_GRAY		();	
	case B_EDITOR_WIRE:		return new CBlender_Editor_Wire		();	
	case B_EDITOR_SEL:		return new CBlender_Editor_Selection();
	case B_LaEmB:			return new CBlender_LaEmB			();		
	case B_LmEbB:			return new CBlender_LmEbB			();		
	case B_BmmD:			return new CBlender_BmmD			();			
	case B_SHADOW_WORLD:	return new CBlender_ShWorld			();		
	case B_BLUR:			return new CBlender_Blur			();			
	case B_MODEL:			return new CBlender_Model			();		
	case B_MODEL_EbB:		return new CBlender_Model_EbB		();	
	case B_DETAIL:			return new CBlender_Detail_Still	();	
	case B_TREE:			return new CBlender_Tree			();	
	case B_PARTICLE:		return new CBlender_Particle		();
	}
	return 0;
}

void		CRender::blender_destroy(IBlender* &B)
{
	xr_delete(B);
}
