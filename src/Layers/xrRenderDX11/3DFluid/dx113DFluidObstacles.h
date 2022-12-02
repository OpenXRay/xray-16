#ifndef dx113DFluidObstacles_included
#define dx113DFluidObstacles_included
#pragma once

class dx113DFluidGrid;
class dx113DFluidData;
class IPhysicsShell;
class IPhysicsElement;
class IPhysicsGeometry;

class dx113DFluidObstacles
{
public:
    dx113DFluidObstacles(int gridWidth, int gridHeight, int gridDepth, dx113DFluidGrid* pGrid);
    ~dx113DFluidObstacles();

    void ProcessObstacles(const dx113DFluidData& FluidData, float timestep);

private:
    enum ObstacleShader
    {
        OS_OOBB = 0,
        OS_DynamicOOBB,
        OS_NumShaders
    };

private:
    void InitShaders();
    void DestroyShaders();

    void ProcessStaticObstacles(const dx113DFluidData& FluidData, const Fmatrix& WorldToFluid);
    void ProcessDynamicObstacles(const dx113DFluidData& FluidData, const Fmatrix& WorldToFluid, float timestep);

    //	This technique renders several objects.
    void RenderPhysicsShell(const IPhysicsShell* pShell, const Fmatrix& WorldToFluid, float timestep);
    void RenderPhysicsElement(const IPhysicsElement& Element, const Fmatrix& WorldToFluid, float timestep);

    void RenderStaticOOBB(const Fmatrix& Transform);
    // void	RenderDynamicOOBB( const IPhysicsElement &Element, const Fmatrix &WorldToFluid, float timestep);
    void RenderDynamicOOBB(const IPhysicsGeometry& Geometry, const Fmatrix& WorldToFluid, float timestep);

private:
    Fvector3 m_vGridDim;

    ref_selement m_ObstacleTechnique[OS_NumShaders];

    dx113DFluidGrid* m_pGrid;

    //	Cache vectors to avoid memory reallocations
    //	TODO: DX11: Reserve memory on object creation
    xr_vector<ISpatial*> m_lstRenderables;
    xr_vector<const IPhysicsShell*> m_lstShells;
    xr_vector<const IPhysicsElement*> m_lstElements;
};

#endif //	dx113DFluidObstacles_included
