#include "stdafx.h"
#include "dx103DFluidVolume.h"

#include "dx103DFluidManager.h"

dx103DFluidVolume::dx103DFluidVolume()
{
}

dx103DFluidVolume::~dx103DFluidVolume()
{

}

void dx103DFluidVolume::Load( LPCSTR N, IReader *data, u32 dwFlags )
{
	//	Uncomment this if choose to read from OGF
//	dxRender_Visual::Load		(N,data,dwFlags);

	//	Create shader for correct sort while rendering
	//	shader name can't start from a digit
	shader.create("fluid3d_stub","water\\water_ryaska1");

	//	Create debug geom
	m_Geom.create			(FVF::F_LIT, RCache.Vertex.Buffer(), RCache.QuadIB);

	Type = MT_3DFLUIDVOLUME;

	//	Version 3>
	m_FluidData.Load(data);

	//	Prepare transform
	const Fmatrix		&Transform = m_FluidData.GetTransform();

	//	Update visibility data
	vis.box.min = Fvector3().set(-0.5f, -0.5f, -0.5f);
	vis.box.max = Fvector3().set( 0.5f,  0.5f,  0.5f);

	vis.box.xform(Transform);

	vis.box.getcenter(vis.sphere.P);
	vis.sphere.R = vis.box.getradius();

/*
	//	Version 2
	//	Prepare transform
	Fmatrix		Transform;
	data->r( &Transform, sizeof(Transform) );
	m_FluidData.SetTransform(Transform);

	//	Update visibility data
	vis.box.min = Fvector3().set(-0.5f, -0.5f, -0.5f);
	vis.box.max = Fvector3().set( 0.5f,  0.5f,  0.5f);

	vis.box.xform(Transform);

	vis.box.getcenter(vis.sphere.P);
	vis.sphere.R = vis.box.getradius();

	//	Read obstacles
	u32 uiObstCnt = data->r_u32();
	for(u32 i=0; i<uiObstCnt; ++i)
	{
		Fmatrix		ObstTransform;
		data->r( &ObstTransform, sizeof(ObstTransform) );
		m_FluidData.AddObstacle(ObstTransform);
	}
	*/

	//	Version 0
	/*
	Fbox	B;
	data->r( &B, sizeof(B) );

	Fmatrix		Transform;
	Fvector3	temp;

	B.getsize(temp);
	Transform.scale(temp);

	B.getcenter(temp);
	Transform.translate_over(temp);

	m_FluidData.SetTransform(Transform);

	vis.box.set(B);
	//vis.box.min = Fvector3().set(-0.5f, -0.5f, -0.5f);
	//vis.box.max = Fvector3().set( 0.5f,  0.5f,  0.5f);

	vis.box.getcenter(vis.sphere.P);
	vis.sphere.R = vis.box.getradius();
	*/
}

void dx103DFluidVolume::Render( float LOD )		// LOD - Level Of Detail  [0.0f - min, 1.0f - max], Ignored ?
{
	//	Render debug box
	//	Do it BEFORE update since update resets shaders and other pipeline settings

//	FluidManager.RenderFluid( m_FluidData );

	u32 dwOffset,dwCount;

	FVF::LIT* pv_start	= (FVF::LIT*)RCache.Vertex.Lock(6*3*2,m_Geom->vb_stride,dwOffset);
	FVF::LIT* pv		= pv_start;

	u32 clr = 0xFFFFFFFF;

	Fbox	box;
	box.min = Fvector3().set(-0.5f, -0.5f, -0.5f);
	box.max = Fvector3().set( 0.5f,  0.5f,  0.5f);
	//box.min = Fvector3().set( 0.0f,  0.0f,  0.0f);
	//box.max = Fvector3().set( 1.0f,  1.0f,  1.0f);

	//	Prepare box here
	{
		//	Face 0
		pv->set( box.x1, box.y1, box.z1, clr, 0, 0);	pv++;
		pv->set( box.x1, box.y1, box.z2, clr, 0, 1);	pv++;
		pv->set( box.x1, box.y2, box.z1, clr, 1, 0);	pv++;
		pv->set( box.x1, box.y2, box.z2, clr, 1, 1);	pv++;

		//	Face 1
		pv->set( box.x2, box.y1, box.z2, clr, 0, 1);	pv++;		
		pv->set( box.x2, box.y1, box.z1, clr, 0, 0);	pv++;				
		pv->set( box.x2, box.y2, box.z2, clr, 1, 1);	pv++;
		pv->set( box.x2, box.y2, box.z1, clr, 1, 0);	pv++;

		//	Face 2
		pv->set( box.x1, box.y1, box.z2, clr, 0, 1);	pv++;
		pv->set( box.x1, box.y1, box.z1, clr, 0, 0);	pv++;		
		pv->set( box.x2, box.y1, box.z2, clr, 1, 1);	pv++;
		pv->set( box.x2, box.y1, box.z1, clr, 1, 0);	pv++;

		//	Face 3
		pv->set( box.x1, box.y2, box.z1, clr, 0, 0);	pv++;				
		pv->set( box.x1, box.y2, box.z2, clr, 0, 1);	pv++;				
		pv->set( box.x2, box.y2, box.z1, clr, 1, 0);	pv++;
		pv->set( box.x2, box.y2, box.z2, clr, 1, 1);	pv++;

		//	Face 4
		pv->set( box.x1, box.y1, box.z1, clr, 0, 1);	pv++;
		pv->set( box.x1, box.y2, box.z1, clr, 0, 0);	pv++;		
		pv->set( box.x2, box.y1, box.z1, clr, 1, 1);	pv++;
		pv->set( box.x2, box.y2, box.z1, clr, 1, 0);	pv++;

		//	Face 5
		pv->set( box.x1, box.y2, box.z2, clr, 0, 0);	pv++;				
		pv->set( box.x1, box.y1, box.z2, clr, 0, 1);	pv++;				
		pv->set( box.x2, box.y2, box.z2, clr, 1, 0);	pv++;
		pv->set( box.x2, box.y1, box.z2, clr, 1, 1);	pv++;

	}

	RCache.set_xform_world( m_FluidData.GetTransform() );

	dwCount = u32( pv - pv_start );
	RCache.Vertex.Unlock( dwCount, m_Geom->vb_stride );
	RCache.set_Geometry( m_Geom );

	//RCache.Render(D3DPT_TRIANGLELIST,dwOffset,0,dwCount,0,dwCount/2);

	//	Render obstacles
	const xr_vector<Fmatrix> &Obstacles = m_FluidData.GetObstaclesList();
	int iObstNum = Obstacles.size();
	for (int i=0; i<iObstNum; ++i)
	{
		RCache.set_xform_world( Obstacles[i] );
		//RCache.Render(D3DPT_TRIANGLELIST,dwOffset,0,dwCount,0,dwCount/2);
	}
	
//	FluidManager.Update( m_FluidData, 1.0f/30.0f);

	//float fTimeStep = Device.fTimeDelta*30*2.0f;
	const float fTimeStep = 2.0f;

	//FluidManager.Update( m_FluidData, 2.0f);
	FluidManager.Update( m_FluidData, fTimeStep);
	FluidManager.RenderFluid( m_FluidData );
}

void dx103DFluidVolume::Copy( dxRender_Visual *pFrom )
{
	dxRender_Visual::Copy(pFrom);
}

void dx103DFluidVolume::Release()
{
	dxRender_Visual::Release();
}