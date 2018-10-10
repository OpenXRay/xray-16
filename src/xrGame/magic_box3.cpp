#include "StdAfx.h"
#include "magic_box3.h"

bool MagicBox3::intersects(const MagicBox3& rkBox1) const
{
    const MagicBox3& rkBox0 = *this;
    // convenience variables
    const Fvector* akA = rkBox0.Axes();
    const Fvector* akB = rkBox1.Axes();
    const float* afEA = rkBox0.Extents();
    const float* afEB = rkBox1.Extents();

    // compute difference of box centers, D = C1-C0
    Fvector kD = Fvector().sub(rkBox1.Center(), rkBox0.Center());

    float aafC[3][3]; // matrix C = A^T B, c_{ij} = dotproduct(A_i,B_j)
    float aafAbsC[3][3]; // |c_{ij}|
    float afAD[3]; // dotproduct(A_i,D)
    float fR0, fR1, fR; // interval radii and distance between centers
    float fR01; // = R0 + R1

    // axis C0+t*A0
    aafC[0][0] = akA[0].dotproduct(akB[0]);
    aafC[0][1] = akA[0].dotproduct(akB[1]);
    aafC[0][2] = akA[0].dotproduct(akB[2]);
    afAD[0] = akA[0].dotproduct(kD);
    aafAbsC[0][0] = _abs(aafC[0][0]);
    aafAbsC[0][1] = _abs(aafC[0][1]);
    aafAbsC[0][2] = _abs(aafC[0][2]);
    fR = _abs(afAD[0]);
    fR1 = afEB[0] * aafAbsC[0][0] + afEB[1] * aafAbsC[0][1] + afEB[2] * aafAbsC[0][2];
    fR01 = afEA[0] + fR1;
    if (fR > fR01)
        return false;

    // axis C0+t*A1
    aafC[1][0] = akA[1].dotproduct(akB[0]);
    aafC[1][1] = akA[1].dotproduct(akB[1]);
    aafC[1][2] = akA[1].dotproduct(akB[2]);
    afAD[1] = akA[1].dotproduct(kD);
    aafAbsC[1][0] = _abs(aafC[1][0]);
    aafAbsC[1][1] = _abs(aafC[1][1]);
    aafAbsC[1][2] = _abs(aafC[1][2]);
    fR = _abs(afAD[1]);
    fR1 = afEB[0] * aafAbsC[1][0] + afEB[1] * aafAbsC[1][1] + afEB[2] * aafAbsC[1][2];
    fR01 = afEA[1] + fR1;
    if (fR > fR01)
        return false;

    // axis C0+t*A2
    aafC[2][0] = akA[2].dotproduct(akB[0]);
    aafC[2][1] = akA[2].dotproduct(akB[1]);
    aafC[2][2] = akA[2].dotproduct(akB[2]);
    afAD[2] = akA[2].dotproduct(kD);
    aafAbsC[2][0] = _abs(aafC[2][0]);
    aafAbsC[2][1] = _abs(aafC[2][1]);
    aafAbsC[2][2] = _abs(aafC[2][2]);
    fR = _abs(afAD[2]);
    fR1 = afEB[0] * aafAbsC[2][0] + afEB[1] * aafAbsC[2][1] + afEB[2] * aafAbsC[2][2];
    fR01 = afEA[2] + fR1;
    if (fR > fR01)
        return false;

    // axis C0+t*B0
    fR = _abs(akB[0].dotproduct(kD));
    fR0 = afEA[0] * aafAbsC[0][0] + afEA[1] * aafAbsC[1][0] + afEA[2] * aafAbsC[2][0];
    fR01 = fR0 + afEB[0];
    if (fR > fR01)
        return false;

    // axis C0+t*B1
    fR = _abs(akB[1].dotproduct(kD));
    fR0 = afEA[0] * aafAbsC[0][1] + afEA[1] * aafAbsC[1][1] + afEA[2] * aafAbsC[2][1];
    fR01 = fR0 + afEB[1];
    if (fR > fR01)
        return false;

    // axis C0+t*B2
    fR = _abs(akB[2].dotproduct(kD));
    fR0 = afEA[0] * aafAbsC[0][2] + afEA[1] * aafAbsC[1][2] + afEA[2] * aafAbsC[2][2];
    fR01 = fR0 + afEB[2];
    if (fR > fR01)
        return false;

    // axis C0+t*A0xB0
    fR = _abs(afAD[2] * aafC[1][0] - afAD[1] * aafC[2][0]);
    fR0 = afEA[1] * aafAbsC[2][0] + afEA[2] * aafAbsC[1][0];
    fR1 = afEB[1] * aafAbsC[0][2] + afEB[2] * aafAbsC[0][1];
    fR01 = fR0 + fR1;
    if (fR > fR01)
        return false;

    // axis C0+t*A0xB1
    fR = _abs(afAD[2] * aafC[1][1] - afAD[1] * aafC[2][1]);
    fR0 = afEA[1] * aafAbsC[2][1] + afEA[2] * aafAbsC[1][1];
    fR1 = afEB[0] * aafAbsC[0][2] + afEB[2] * aafAbsC[0][0];
    fR01 = fR0 + fR1;
    if (fR > fR01)
        return false;

    // axis C0+t*A0xB2
    fR = _abs(afAD[2] * aafC[1][2] - afAD[1] * aafC[2][2]);
    fR0 = afEA[1] * aafAbsC[2][2] + afEA[2] * aafAbsC[1][2];
    fR1 = afEB[0] * aafAbsC[0][1] + afEB[1] * aafAbsC[0][0];
    fR01 = fR0 + fR1;
    if (fR > fR01)
        return false;

    // axis C0+t*A1xB0
    fR = _abs(afAD[0] * aafC[2][0] - afAD[2] * aafC[0][0]);
    fR0 = afEA[0] * aafAbsC[2][0] + afEA[2] * aafAbsC[0][0];
    fR1 = afEB[1] * aafAbsC[1][2] + afEB[2] * aafAbsC[1][1];
    fR01 = fR0 + fR1;
    if (fR > fR01)
        return false;

    // axis C0+t*A1xB1
    fR = _abs(afAD[0] * aafC[2][1] - afAD[2] * aafC[0][1]);
    fR0 = afEA[0] * aafAbsC[2][1] + afEA[2] * aafAbsC[0][1];
    fR1 = afEB[0] * aafAbsC[1][2] + afEB[2] * aafAbsC[1][0];
    fR01 = fR0 + fR1;
    if (fR > fR01)
        return false;

    // axis C0+t*A1xB2
    fR = _abs(afAD[0] * aafC[2][2] - afAD[2] * aafC[0][2]);
    fR0 = afEA[0] * aafAbsC[2][2] + afEA[2] * aafAbsC[0][2];
    fR1 = afEB[0] * aafAbsC[1][1] + afEB[1] * aafAbsC[1][0];
    fR01 = fR0 + fR1;
    if (fR > fR01)
        return false;

    // axis C0+t*A2xB0
    fR = _abs(afAD[1] * aafC[0][0] - afAD[0] * aafC[1][0]);
    fR0 = afEA[0] * aafAbsC[1][0] + afEA[1] * aafAbsC[0][0];
    fR1 = afEB[1] * aafAbsC[2][2] + afEB[2] * aafAbsC[2][1];
    fR01 = fR0 + fR1;
    if (fR > fR01)
        return false;

    // axis C0+t*A2xB1
    fR = _abs(afAD[1] * aafC[0][1] - afAD[0] * aafC[1][1]);
    fR0 = afEA[0] * aafAbsC[1][1] + afEA[1] * aafAbsC[0][1];
    fR1 = afEB[0] * aafAbsC[2][2] + afEB[2] * aafAbsC[2][0];
    fR01 = fR0 + fR1;
    if (fR > fR01)
        return false;

    // axis C0+t*A2xB2
    fR = _abs(afAD[1] * aafC[0][2] - afAD[0] * aafC[1][2]);
    fR0 = afEA[0] * aafAbsC[1][2] + afEA[1] * aafAbsC[0][2];
    fR1 = afEB[0] * aafAbsC[2][1] + afEB[1] * aafAbsC[2][0];
    fR01 = fR0 + fR1;
    if (fR > fR01)
        return false;

    return true;
}

void MagicBox3::ComputeVertices(Fvector* akVertex) const
{
    Fvector akEAxis[3] = {Fvector().mul(m_akAxis[0], m_afExtent[0]), Fvector().mul(m_akAxis[1], m_afExtent[1]),
        Fvector().mul(m_akAxis[2], m_afExtent[2])};

    akVertex[0] = Fvector().sub(m_kCenter, akEAxis[0]).sub(akEAxis[1]).sub(akEAxis[2]);
    akVertex[1] = Fvector().add(m_kCenter, akEAxis[0]).sub(akEAxis[1]).sub(akEAxis[2]);
    akVertex[2] = Fvector().add(m_kCenter, akEAxis[0]).add(akEAxis[1]).sub(akEAxis[2]);
    akVertex[3] = Fvector().sub(m_kCenter, akEAxis[0]).add(akEAxis[1]).sub(akEAxis[2]);
    akVertex[4] = Fvector().sub(m_kCenter, akEAxis[0]).sub(akEAxis[1]).add(akEAxis[2]);
    akVertex[5] = Fvector().add(m_kCenter, akEAxis[0]).sub(akEAxis[1]).add(akEAxis[2]);
    akVertex[6] = Fvector().add(m_kCenter, akEAxis[0]).add(akEAxis[1]).add(akEAxis[2]);
    akVertex[7] = Fvector().sub(m_kCenter, akEAxis[0]).add(akEAxis[1]).add(akEAxis[2]);
}
