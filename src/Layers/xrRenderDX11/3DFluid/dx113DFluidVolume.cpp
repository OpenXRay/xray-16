#include "stdafx.h"
#include "dx113DFluidVolume.h"

#include "dx113DFluidManager.h"

namespace xray::render::fg
{
dx113DFluidVolume::dx113DFluidVolume() {}
dx113DFluidVolume::~dx113DFluidVolume() {}
void dx113DFluidVolume::Load(LPCSTR /*N*/, IReader* data, u32 /*dwFlags*/)
{
    //	Uncomment this if choose to read from OGF
    //	dxRender_Visual::Load		(N,data,dwFlags);

    //	Create shader for correct sort while rendering
    //	shader name can't start from a digit
    shader.create("fluid3d_stub", "water\\water_ryaska1");

    //	Create debug geom
    m_Geom.create(FVF::F_LIT, RImplementation.Vertex.Buffer(), RImplementation.QuadIB);

    Type = MT_3DFLUIDVOLUME;

    //	Version 3>
    m_FluidData.Load(data);

    //	Prepare transform
    const Fmatrix& Transform = m_FluidData.GetTransform();

    //	Update visibility data
    vis.box.vMin = Fvector3().set(-0.5f, -0.5f, -0.5f);
    vis.box.vMax = Fvector3().set(0.5f, 0.5f, 0.5f);

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
        vis.box.vMin = Fvector3().set(-0.5f, -0.5f, -0.5f);
        vis.box.vMax = Fvector3().set( 0.5f,  0.5f,  0.5f);

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
    //vis.box.vMin = Fvector3().set(-0.5f, -0.5f, -0.5f);
    //vis.box.vMax = Fvector3().set( 0.5f,  0.5f,  0.5f);

    vis.box.getcenter(vis.sphere.P);
    vis.sphere.R = vis.box.getradius();
    */
}

void dx113DFluidVolume::Copy(dxRender_Visual* pFrom) { dxRender_Visual::Copy(pFrom); }
void dx113DFluidVolume::Release() { dxRender_Visual::Release(); }
} // namespace xray::render::fg
