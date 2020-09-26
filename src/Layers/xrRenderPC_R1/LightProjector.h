// LightShadows.h: interface for the CLightShadows class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LIGHTPRJ_H__CFA216D9_CACB_4515_9FBE_7C531649168F__INCLUDED_)
#define AFX_LIGHTPRJ_H__CFA216D9_CACB_4515_9FBE_7C531649168F__INCLUDED_
#pragma once

#include "Layers/xrRender/r__dsgraph_types.h"

class CLightProjector : public pureAppActivate
{
private:
    static const int P_rt_size = 512;
    static const int P_o_size = 51;
    static const int P_o_line = P_rt_size / P_o_size;
    static const int P_o_count = P_o_line * P_o_line;

    //
    typedef R_dsgraph::_MatrixItem NODE;
    struct recv
    {
        IRenderable* O;
        Fvector C;
        Fmatrix UVgen;
        Fvector UVclamp_min;
        Fvector UVclamp_max;
        Fbox BB;
        u32 dwFrame;
        u32 dwTimeValid;
    };

private:
    IRenderable* current;
    xr_vector<recv> cache; // same as number of slots
    xr_vector<IRenderable*> receivers;
    xr_vector<int> taskid;

    ref_rt RT;
    shared_str c_xform;
    shared_str c_clamp;
    shared_str c_factor;

public:
    void set_object(IRenderable* O);
    BOOL shadowing() { return current != nullptr; }
    void calculate();
    void setup(int slot);
    void finalize()
    {
        receivers.clear();
        taskid.clear();
    }
    void invalidate();

    virtual void OnAppActivate();
#ifdef DEBUG
    void render();
#endif

    CLightProjector();
    ~CLightProjector();
};

#endif // !defined(AFX_LIGHTPRJ_H__CFA216D9_CACB_4515_9FBE_7C531649168F__INCLUDED_)
