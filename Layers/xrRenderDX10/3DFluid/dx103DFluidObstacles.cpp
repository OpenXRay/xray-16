#include "stdafx.h"
#include "dx103DFluidObstacles.h"

#include "../../xrRender/dxRenderDeviceRender.h"
#include "../../../xrEngine/xr_object.h"
#include "../../../xrEngine/IPhysicsShell.h"
#include "../../../xrEngine/IObjectPhysicsCollision.h"
#include "../../../xrEngine/IPhysicsGeometry.h"

#include "dx103DFluidBlenders.h"
#include "dx103DFluidData.h"
#include "dx103DFluidGrid.h"

/*
#ifdef	DEBUG
#include "../../xrRender/dxDebugRender.h"
#endif

static void draw_obb		( const Fmatrix &matrix, const u32 &color )
{
	Fvector							aabb[8];
	matrix.transform_tiny			(aabb[0],Fvector().set( -1, -1, -1)); // 0
	matrix.transform_tiny			(aabb[1],Fvector().set( -1, +1, -1)); // 1
	matrix.transform_tiny			(aabb[2],Fvector().set( +1, +1, -1)); // 2
	matrix.transform_tiny			(aabb[3],Fvector().set( +1, -1, -1)); // 3
	matrix.transform_tiny			(aabb[4],Fvector().set( -1, -1, +1)); // 4
	matrix.transform_tiny			(aabb[5],Fvector().set( -1, +1, +1)); // 5
	matrix.transform_tiny			(aabb[6],Fvector().set( +1, +1, +1)); // 6
	matrix.transform_tiny			(aabb[7],Fvector().set( +1, -1, +1)); // 7

	u16								aabb_id[12*2] = {
		0,1,  1,2,  2,3,  3,0,  4,5,  5,6,  6,7,  7,4,  1,5,  2,6,  3,7,  0,4
	};

	rdebug_render->add_lines						(aabb, sizeof(aabb)/sizeof(Fvector), &aabb_id[0], sizeof(aabb_id)/(2*sizeof(u16)), color);
}
*/

namespace
{
	//	For OOBB
//	shared_str	strBoxLBDcorner("boxLBDcorner");
//	shared_str	strBoxRTUcorner("boxRTUcorner");
	shared_str	strOOBBClipPlane("OOBBClipPlane");

	//	For velocity calculation
	shared_str	strWorldToLocal("WorldToLocal");
	shared_str	strLocalToWorld("LocalToWorld");
	shared_str	strMassCenter("MassCenter");
	shared_str	strOOBBWorldAngularVelocity("OOBBWorldAngularVelocity");
	shared_str	strOOBBWorldTranslationVelocity("OOBBWorldTranslationVelocity");

	Fvector4	UnitClipPlanes[] = 
	{
		{-1.f,	0.0f,	0.0f,	0.5f},	//	
		{1.f,	0.0f,	0.0f,	0.5f},	//	
		{0.0f,	-1.f,	0.0f,	0.5f},	//	Top
		{0.0f,	1.f,	0.0f,	0.5f},	//	Bottom
		{0.0f,	0.0f,	-1.f,	0.5f},	//	
		{0.0f,	0.0f,	1.f,	0.5f},	//	
	};
}

dx103DFluidObstacles::dx103DFluidObstacles(int gridWidth, int gridHeight, int gridDepth, dx103DFluidGrid *pGrid)
	:	m_pGrid(pGrid)
{
	VERIFY(m_pGrid);

	m_vGridDim[0] = float(gridWidth);
	m_vGridDim[1] = float(gridHeight);
	m_vGridDim[2] = float(gridDepth);

	InitShaders();
}

dx103DFluidObstacles::~dx103DFluidObstacles()
{
	DestroyShaders();

	m_pGrid = 0;
}

void dx103DFluidObstacles::InitShaders()
{
	{
		CBlender_fluid_obst		Blender;
		ref_shader				shader;
		shader.create(&Blender, "null");
		for (int i=0; i<2; ++i)
			m_ObstacleTechnique[OS_OOBB+i] = shader->E[i];
	}
}

void dx103DFluidObstacles::DestroyShaders()
{
	for (int i=0; i<OS_NumShaders; ++i)
	{
		//	Release shader's element.
		m_ObstacleTechnique[i] = 0;
	}
}

void dx103DFluidObstacles::ProcessObstacles( const dx103DFluidData &FluidData, float timestep )
{
	PIX_EVENT(ProcessObstacles);

	//	Prepare world-space to grid transform
	Fmatrix WorldToFluid;
	{
		Fmatrix InvFluidTranform;
		Fmatrix Scale;
		Fmatrix Translate;
		Fmatrix TranslateScale;
		//	Convert to 0..intDim space since it is used by simulation
		//Scale.scale((float)m_iTextureWidth-1, (float)m_iTextureHeight-1, (float)m_iTextureDepth-1);
		//Translate.translate(0.5, 0.5, 0.5);
		//It seems that y axis is inverted in fluid simulation, so shange maths a bit
//		Scale.scale(m_vGridDim.x-1, -(m_vGridDim.y-1), m_vGridDim.z-1 );
		Scale.scale(m_vGridDim.x, -(m_vGridDim.y), m_vGridDim.z );
		Translate.translate(0.5, -0.5, 0.5);
		//	Actually it is mul(Translate, Scale).
		//	Our matrix multiplication is not correct.
		TranslateScale.mul(Scale, Translate);
		InvFluidTranform.invert(FluidData.GetTransform());
		WorldToFluid.mul(TranslateScale,InvFluidTranform);
	}

	ProcessDynamicObstacles( FluidData, WorldToFluid, timestep );

	//	Render static obstacles last
	//	to override speed where bounding shapes of dynamic and
	//	static objects intersect.
	ProcessStaticObstacles( FluidData, WorldToFluid );
}

void dx103DFluidObstacles::RenderStaticOOBB( const Fmatrix &Transform)
{
	PIX_EVENT(RenderObstacle);

	//	Shader must be already set up!
	Fmatrix	InvTransform;
	Fmatrix ClipTransform;
	InvTransform.invert(Transform);
	ClipTransform.transpose(InvTransform);

	for ( int i=0; i<6; ++i)
	{
		Fvector4	TransformedPlane;
		ClipTransform.transform(TransformedPlane, UnitClipPlanes[i]);
		TransformedPlane.normalize_as_plane();
		RCache.set_ca(strOOBBClipPlane, i, TransformedPlane);
	}

	m_pGrid->DrawSlices();
}

void dx103DFluidObstacles::ProcessStaticObstacles( const dx103DFluidData &FluidData, const Fmatrix &WorldToFluid )
{
	RCache.set_Element(m_ObstacleTechnique[OS_OOBB]);

	const xr_vector<Fmatrix> &Obstacles = FluidData.GetObstaclesList();
	int iObstNum = Obstacles.size();
	for (int i=0; i<iObstNum; ++i)
	{
		Fmatrix Transform;
		Transform.mul(WorldToFluid, Obstacles[i]);

		RenderStaticOOBB(Transform);
	}
}

void dx103DFluidObstacles::ProcessDynamicObstacles( const dx103DFluidData &FluidData, const Fmatrix &WorldToFluid, float timestep )
{
	m_lstRenderables.clear_not_free();
	m_lstShells.clear_not_free();
	m_lstElements.clear_not_free();

	Fbox	box;
	box.min = Fvector3().set(-0.5f, -0.5f, -0.5f);
	box.max = Fvector3().set( 0.5f,  0.5f,  0.5f);
	box.xform(FluidData.GetTransform());
	Fvector3	center;
	Fvector3	size;
	box.getcenter(center);
	box.getradius(size);


	// Traverse object database
	g_SpatialSpace->q_box
		(
		m_lstRenderables,
		0, //ISpatial_DB::O_ORDERED,
		STYPE_RENDERABLE,
		center,
		size
		);

	// Determine visibility for dynamic part of scene
	for (u32 i=0; i<m_lstRenderables.size(); ++i)
	{
		ISpatial*	spatial		= m_lstRenderables[i];
		//	Can use to optimize invisible dynamic objects if necessary
		//CSector*	sector		= (CSector*)spatial->spatial.sector;
		//if	(0==sector)										continue;	// disassociated from S/P structure
		//if	(PortalTraverser.i_marker != sector->r_marker)	continue;	// inactive (untouched) sector

		// renderable
		//IRenderable*	renderable		= spatial->dcast_Renderable	();
		//if (0==renderable)				continue;					// unknown, but renderable object (r1_glow???)
		CObject*		pObject = spatial->dcast_CObject();
		if (!pObject) continue;

		const IObjectPhysicsCollision* pCollision = pObject->physics_collision();
		if (!pCollision) continue;

		const IPhysicsShell*		pShell = pCollision->physics_shell();
		const IPhysicsElement*	pElement = pCollision->physics_character();
		if (pShell)
		{
			//	Push shell here
			m_lstShells.push_back(pShell);
		}
		else if (pElement)
		{
			m_lstElements.push_back(pElement);
		}
	}

	if (! (m_lstShells.size() || m_lstElements.size()))
		return;

	RCache.set_Element(m_ObstacleTechnique[OS_DynamicOOBB]);

	Fmatrix	FluidToWorld;
	FluidToWorld.invert(WorldToFluid);

	RCache.set_c(strWorldToLocal, WorldToFluid);	
	RCache.set_c(strLocalToWorld, FluidToWorld);

	int iShellsNum = m_lstShells.size();
	for (int i=0; i<iShellsNum; ++i)
	{
		RenderPhysicsShell( m_lstShells[i], WorldToFluid, timestep );
	}

	int iElementsNum = m_lstElements.size();
	for (int i=0; i<iElementsNum; ++i)
	{
		RenderPhysicsElement( *m_lstElements[i], WorldToFluid, timestep );
	}
	
}

//	TODO: DX10: Do it using instancing.
void dx103DFluidObstacles::RenderPhysicsShell( const IPhysicsShell *pShell, const Fmatrix &WorldToFluid, float timestep )
{
	u16 iObstNum = pShell->get_ElementsNumber();
	for (u16 i=0; i<iObstNum; ++i)
	{
		const IPhysicsElement &Element = pShell->Element(i);

		RenderPhysicsElement( Element, WorldToFluid, timestep );
	}
}

void dx103DFluidObstacles::RenderPhysicsElement( const IPhysicsElement &Element, const Fmatrix &WorldToFluid, float timestep)
{
	//	Shader must be already set up!
	const Fvector3	&MassCenter3 = Element.mass_Center();
	Fvector3	AngularVelocity3;
	Fvector3	TranslationVelocity3;
	Element.get_AngularVel(AngularVelocity3);
	Element.get_LinearVel(TranslationVelocity3);

	Fvector4	MassCenter;
	Fvector4	AngularVelocity;
	Fvector4	TranslationVelocity;
	MassCenter.set( MassCenter3.x, MassCenter3.y, MassCenter3.z, 0.0f);
	AngularVelocity.set( AngularVelocity3.x, AngularVelocity3.y, AngularVelocity3.z, 0.0f);
	TranslationVelocity.set( TranslationVelocity3.x, TranslationVelocity3.y, TranslationVelocity3.z, 0.0f);

	float	fVelocityScale;

	VERIFY(timestep!=0);

	fVelocityScale = 1/timestep;

	//	Convert speed
	fVelocityScale /= 30.0f * 2.0f;

	
	//fVelocityScale *= Device.fTimeDelta;

	//	Emphasize velocity influence on the fog
	//fVelocityScale *= 10;
	//fVelocityScale *= 4;	//	Good for the beginning
	fVelocityScale *= 6;

	AngularVelocity.mul( fVelocityScale );
	TranslationVelocity.mul( fVelocityScale );

	//	Emphasize velocity influence on the fog
	//TranslationVelocity.mul( 2.0f );

	RCache.set_c(strMassCenter, MassCenter);
	RCache.set_c(strOOBBWorldAngularVelocity, AngularVelocity);
	RCache.set_c(strOOBBWorldTranslationVelocity, TranslationVelocity);

	int iShapeNum = Element.numberOfGeoms();

	for ( u16 i=0; i<iShapeNum; ++i)
	{
		if (Element.geometry(i)->collide_fluids())
			RenderDynamicOOBB( *Element.geometry(i), WorldToFluid, timestep);
	}
}

/*
void dx103DFluidObstacles::RenderDynamicOOBB( const IPhysicsElement &Element, const Fmatrix &WorldToFluid, float timestep)
{
	PIX_EVENT(RenderDynamicObstacle);

	//	dsdad;

	Fmatrix Transform;
	//Transform.mul(WorldToFluid, Element.XFORM());

	Fvector3 BoxSize;
	Fvector3 BoxCenter;
	Element.get_Box( BoxSize, BoxCenter );

	//	Get bone transform
	Fmatrix OOBBTransform = Element.XFORM();
	//	Overwrite bone position with oobb world position
	OOBBTransform.c = BoxCenter;

	Transform.mul(WorldToFluid, OOBBTransform);

	//	Shader must be already set up!
	Fmatrix	InvTransform;
	Fmatrix ClipTransform;
	InvTransform.invert(Transform);
	ClipTransform.transpose(InvTransform);

	for ( int i=0; i<6; ++i)
	{
		Fvector4	UpdatedPlane = UnitClipPlanes[i];
		UpdatedPlane.w *= BoxSize[i/2];
		//UpdatedPlane.w += (i%2) ? (-BoxCenter[i/2]) : BoxCenter[i/2];
		Fvector4	TransformedPlane;
		ClipTransform.transform(TransformedPlane, UpdatedPlane);
		TransformedPlane.normalize_as_plane();
		RCache.set_ca(strOOBBClipPlane, i, TransformedPlane);
	}

	const Fvector3	&MassCenter3 = Element.mass_Center();
	Fvector3	AngularVelocity3;
	Fvector3	TranslationVelocity3;
	Element.get_AngularVel(AngularVelocity3);
	Element.get_LinearVel(TranslationVelocity3);

	Fvector4	MassCenter;
	Fvector4	AngularVelocity;
	Fvector4	TranslationVelocity;
	MassCenter.set( MassCenter3.x, MassCenter3.y, MassCenter3.z, 0.0f);
	AngularVelocity.set( AngularVelocity3.x, AngularVelocity3.y, AngularVelocity3.z, 0.0f);
	TranslationVelocity.set( TranslationVelocity3.x, TranslationVelocity3.y, TranslationVelocity3.z, 0.0f);

	float	fVelocityScale;
	
	VERIFY(timestep!=0);

	fVelocityScale = 1/timestep;

	fVelocityScale *= Device.fTimeDelta;

	AngularVelocity.mul( fVelocityScale );
	TranslationVelocity.mul( fVelocityScale );

	RCache.set_c(strMassCenter, MassCenter);
	RCache.set_c(strOOBBWorldAngularVelocity, AngularVelocity);
	RCache.set_c(strOOBBWorldTranslationVelocity, TranslationVelocity);	

	m_pGrid->DrawSlices();
}
*/

void dx103DFluidObstacles::RenderDynamicOOBB( const IPhysicsGeometry &Geometry, const Fmatrix &WorldToFluid, float timestep)
{
	PIX_EVENT(RenderDynamicObstacle);

	//	dsdad;

	Fmatrix Transform;
	//Transform.mul(WorldToFluid, Element.XFORM());

	Fvector3 BoxSize;
	Fmatrix OOBBTransform;
	Geometry.get_Box( OOBBTransform, BoxSize );

	Transform.mul(WorldToFluid, OOBBTransform);

	//	Shader must be already set up!
	//	DynOOBBData must be already set up!
	Fmatrix	InvTransform;
	Fmatrix ClipTransform;
	InvTransform.invert(Transform);
	ClipTransform.transpose(InvTransform);

	for ( int i=0; i<6; ++i)
	{
		Fvector4	UpdatedPlane = UnitClipPlanes[i];
		UpdatedPlane.w *= BoxSize[i/2];
		//UpdatedPlane.w += (i%2) ? (-BoxCenter[i/2]) : BoxCenter[i/2];
		Fvector4	TransformedPlane;
		ClipTransform.transform(TransformedPlane, UpdatedPlane);
		TransformedPlane.normalize_as_plane();
		RCache.set_ca(strOOBBClipPlane, i, TransformedPlane);
	}

	m_pGrid->DrawSlices();
}