#include "stdafx.h"
#include "SkinXW_CPP.hpp"

#include "Threading/ParallelFor.hpp"

#ifdef _EDITOR
#include "SkeletonX.h"
#include "SkeletonCustom.h"
#else
#include "Animation/Bone.hpp"
#include "Layers/xrRender/SkeletonXVertRender.h"
#endif

namespace XRay
{
namespace Math
{
void Skin1W_CPP(vertRender* D, vertBoned1W* S, u32 vCount, CBoneInstance* Bones)
{
    //	return;
    // Prepare
    int U_Count = vCount / 8;
    vertBoned1W* V = S;
    vertBoned1W* E = V + U_Count * 8;

    // Unrolled loop
    for (; S != E;)
    {
        Fmatrix& M0 = Bones[S->matrix].mRenderTransform;
        M0.transform_tiny(D->P, S->P);
        M0.transform_dir(D->N, S->N);
        D->u = S->u;
        D->v = S->v;
        S++;
        D++;

        Fmatrix& M1 = Bones[S->matrix].mRenderTransform;
        M1.transform_tiny(D->P, S->P);
        M1.transform_dir(D->N, S->N);
        D->u = S->u;
        D->v = S->v;
        S++;
        D++;

        Fmatrix& M2 = Bones[S->matrix].mRenderTransform;
        M2.transform_tiny(D->P, S->P);
        M2.transform_dir(D->N, S->N);
        D->u = S->u;
        D->v = S->v;
        S++;
        D++;

        Fmatrix& M3 = Bones[S->matrix].mRenderTransform;
        M3.transform_tiny(D->P, S->P);
        M3.transform_dir(D->N, S->N);
        D->u = S->u;
        D->v = S->v;
        S++;
        D++;

        Fmatrix& M4 = Bones[S->matrix].mRenderTransform;
        M4.transform_tiny(D->P, S->P);
        M4.transform_dir(D->N, S->N);
        D->u = S->u;
        D->v = S->v;
        S++;
        D++;

        Fmatrix& M5 = Bones[S->matrix].mRenderTransform;
        M5.transform_tiny(D->P, S->P);
        M5.transform_dir(D->N, S->N);
        D->u = S->u;
        D->v = S->v;
        S++;
        D++;

        Fmatrix& M6 = Bones[S->matrix].mRenderTransform;
        M6.transform_tiny(D->P, S->P);
        M6.transform_dir(D->N, S->N);
        D->u = S->u;
        D->v = S->v;
        S++;
        D++;

        Fmatrix& M7 = Bones[S->matrix].mRenderTransform;
        M7.transform_tiny(D->P, S->P);
        M7.transform_dir(D->N, S->N);
        D->u = S->u;
        D->v = S->v;
        S++;
        D++;
    }

    // The end part
    vertBoned1W* E2 = V + vCount;
    for (; S != E2;)
    {
        Fmatrix& M = Bones[S->matrix].mRenderTransform;
        M.transform_tiny(D->P, S->P);
        M.transform_dir(D->N, S->N);
        D->u = S->u;
        D->v = S->v;
        S++;
        D++;
    }
}

void Skin2W_CPP(vertRender* D, vertBoned2W* S, u32 vCount, CBoneInstance* Bones)
{
    // Prepare
    int U_Count = vCount;
    vertBoned2W* V = S;
    vertBoned2W* E = V + U_Count;
    Fvector P0, N0, P1, N1;

    // NON-Unrolled loop
    for (; S != E;)
    {
        if (S->matrix1 != S->matrix0)
        {
            Fmatrix& M0 = Bones[S->matrix0].mRenderTransform;
            Fmatrix& M1 = Bones[S->matrix1].mRenderTransform;
            M0.transform_tiny(P0, S->P);
            M0.transform_dir(N0, S->N);
            M1.transform_tiny(P1, S->P);
            M1.transform_dir(N1, S->N);
            D->P.lerp(P0, P1, S->w);
            D->N.lerp(N0, N1, S->w);
            D->u = S->u;
            D->v = S->v;
        }
        else
        {
            Fmatrix& M0 = Bones[S->matrix0].mRenderTransform;
            M0.transform_tiny(D->P, S->P);
            M0.transform_dir(D->N, S->N);
            D->u = S->u;
            D->v = S->v;
        }
        S++;
        D++;
    }
}


void Skin3W_CPP(vertRender* D, vertBoned3W* S, u32 vCount, CBoneInstance* Bones)
{
    // Prepare
    int U_Count = vCount;
    vertBoned3W* V = S;
    vertBoned3W* E = V + U_Count;
    Fvector P0, N0, P1, N1, P2, N2;

    // NON-Unrolled loop
    for (; S != E;)
    {
        Fmatrix& M0 = Bones[S->m[0]].mRenderTransform;
        Fmatrix& M1 = Bones[S->m[1]].mRenderTransform;
        Fmatrix& M2 = Bones[S->m[2]].mRenderTransform;

        M0.transform_tiny(P0, S->P);
        P0.mul(S->w[0]);
        M0.transform_dir(N0, S->N);
        N0.mul(S->w[0]);

        M1.transform_tiny(P1, S->P);
        P1.mul(S->w[1]);
        M1.transform_dir(N1, S->N);
        N1.mul(S->w[1]);

        M2.transform_tiny(P2, S->P);
        P2.mul(1.0f - S->w[0] - S->w[1]);
        M2.transform_dir(N2, S->N);
        N2.mul(1.0f - S->w[0] - S->w[1]);

        P0.add(P1);
        P0.add(P2);

        D->P = P0;

        N0.add(N1);
        N0.add(N2);

        D->N = N0;

        D->u = S->u;
        D->v = S->v;

        S++;
        D++;
    }
}


void Skin4W_CPP(vertRender* D, vertBoned4W* S, u32 vCount, CBoneInstance* Bones)
{
    // Prepare
    vertBoned4W* V = S;

    xr_parallel_for(TaskRange<u32>(0, vCount), [&](const TaskRange<u32>& range) 
    {
        Fvector P0, N0, P1, N1, P2, N2, P3, N3;
        for (u32 i = range.begin(); i != range.end(); ++i)
        {
            Fmatrix& M0 = Bones[S[i].m[0]].mRenderTransform;
            Fmatrix& M1 = Bones[S[i].m[1]].mRenderTransform;
            Fmatrix& M2 = Bones[S[i].m[2]].mRenderTransform;
            Fmatrix& M3 = Bones[S[i].m[3]].mRenderTransform;

            M0.transform_tiny(P0, S[i].P);
            P0.mul(S[i].w[0]);
            M0.transform_dir(N0, S[i].N);
            N0.mul(S[i].w[0]);

            M1.transform_tiny(P1, S[i].P);
            P1.mul(S[i].w[1]);
            M1.transform_dir(N1, S[i].N);
            N1.mul(S[i].w[1]);

            M2.transform_tiny(P2, S[i].P);
            P2.mul(S[i].w[2]);
            M2.transform_dir(N2, S[i].N);
            N2.mul(S[i].w[2]);

            M3.transform_tiny(P3, S[i].P);
            P3.mul(1.0f - S[i].w[0] - S[i].w[1] - S[i].w[2]);
            M3.transform_dir(N3, S[i].N);
            N3.mul(1.0f - S[i].w[0] - S[i].w[1] - S[i].w[2]);

            P0.add(P1);
            P0.add(P2);
            P0.add(P3);

            D[i].P = P0;

            N0.add(N1);
            N0.add(N2);
            N0.add(N3);

            D[i].N = N0;

            D[i].u = S[i].u;
            D[i].v = S[i].v;
        }
    });
}
} // namespace Math
} // namespace XRay
