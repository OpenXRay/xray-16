////////////////////////////////////////////////////////////////////////////
//	Module 		: base_client_classes.h
//	Created 	: 20.12.2004
//  Modified 	: 20.12.2004
//	Author		: Dmitriy Iassenev
//	Description : XRay base client classes script export
////////////////////////////////////////////////////////////////////////////

#pragma once

#include "script_export_space.h"

class DLL_Pure;
class ISpatial;
class ISheduled;
class IRenderable;
class ICollidable;
class CObject;
class IRenderVisual;
class FHierrarhyVisual;
class CBlend;
class IKinematics;
class IKinematicsAnimated;

typedef class_exporter<DLL_Pure>	DLL_PureScript;
add_to_type_list(DLL_PureScript)
#undef script_type_list
#define script_type_list save_type_list(DLL_PureScript)

//typedef class_exporter<ISpatial>	ISpatialScript;
//add_to_type_list(ISpatialScript)
//#undef script_type_list
//#define script_type_list save_type_list(ISpatialScript)

typedef class_exporter<ISheduled>	ISheduledScript;
add_to_type_list(ISheduledScript)
#undef script_type_list
#define script_type_list save_type_list(ISheduledScript)

typedef class_exporter<IRenderable>	IRenderableScript;
add_to_type_list(IRenderableScript)
#undef script_type_list
#define script_type_list save_type_list(IRenderableScript)

typedef class_exporter<ICollidable>	ICollidableScript;
add_to_type_list(ICollidableScript)
#undef script_type_list
#define script_type_list save_type_list(ICollidableScript)

typedef class_exporter<CObject>		CObjectScript;
add_to_type_list(CObjectScript)
#undef script_type_list
#define script_type_list save_type_list(CObjectScript)

typedef class_exporter<CBlend>	CBlendScript;
add_to_type_list(CBlendScript)
#undef script_type_list
#define script_type_list save_type_list(CBlendScript)

typedef class_exporter<IRenderVisual>	IRender_VisualScript;
add_to_type_list(IRender_VisualScript)
#undef script_type_list
#define script_type_list save_type_list(IRender_VisualScript)
/*
typedef class_exporter<FHierrarhyVisual>	FHierrarhyVisualScript;
add_to_type_list(FHierrarhyVisualScript)
#undef script_type_list
#define script_type_list save_type_list(FHierrarhyVisualScript)

typedef class_exporter<IKinematics>	IKinematicsScript;
add_to_type_list(IKinematicsScript)
#undef script_type_list
#define script_type_list save_type_list(IKinematicsScript)
*/

typedef class_exporter<IKinematicsAnimated>	IKinematicsAnimatedScript;
add_to_type_list(IKinematicsAnimatedScript)
#undef script_type_list
#define script_type_list save_type_list(IKinematicsAnimatedScript)

