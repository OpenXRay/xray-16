#ifndef dx113DFluidEmitters_included
#define dx113DFluidEmitters_included
#pragma once

class dx113DFluidGrid;
class dx113DFluidData;

class dx113DFluidEmitters
{
public:
    enum EmitterType
    {
        ET_SimpleGausian = 0,
        ET_SimpleDraught,
        ET_EmittersTypeNum
    };

    struct CEmitter
    {
        EmitterType m_eType;

        Fvector m_vPosition; //	In fluid space
        float m_fRadius;

        //	Distribution attributes
        float m_InvSigma_2; //	1/Sigma^2 for gaussian distribution

        //	Flow attributes
        Fvector m_vFlowVelocity; //	In fluid space

        //	Density attributes
        float m_fSaturation;
        float m_fDensity;

        union
        {
            struct
            {
                float m_fPeriod;
                float m_fPhase;
                float m_fAmp; //	Speed == [FlowSpeed*(1-m_fAmp)..FlowSpeed*(1+m_fAmp)]
            } m_DraughtParams;
        };

        bool m_bApplyDensity;
        bool m_bApplyImpulse;
    };

public:
    dx113DFluidEmitters(int gridWidth, int gridHeight, int gridDepth, dx113DFluidGrid* pGrid);
    ~dx113DFluidEmitters();

    void RenderDensity(const dx113DFluidData& FluidData);
    void RenderVelocity(const dx113DFluidData& FluidData);

private:
    void InitShaders();
    void DestroyShaders();

    void ApplyDensity(const CEmitter& Emitter);
    void ApplyVelocity(const CEmitter& Emitter);

private:
    Fvector3 m_vGridDim;

    ref_selement m_EmitterTechnique[ET_EmittersTypeNum];

    dx113DFluidGrid* m_pGrid;
};

#endif //	dx113DFluidEmitters_included
