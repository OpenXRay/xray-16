// Magic Software, Inc.
// http://www.magic-software.com
// Copyright (c) 2000-2002.  All Rights Reserved
//
// Source code from Magic Software is supplied under the terms of a license
// agreement and may not be copied or disclosed except in accordance with the
// terms of that agreement.  The various license agreements may be found at
// the Magic Software web site.  This file is subject to the license
//
// FREE SOURCE CODE
// http://www.magic-software.com/License/free.pdf
#pragma hdrstop
#include "MgcBox3.h"
using namespace Mgc;

//----------------------------------------------------------------------------
void Box3::ComputeVertices(Vector3 akVertex[8]) const
{
    Vector3 akEAxis[3] =
        {
            m_afExtent[0] * m_akAxis[0],
            m_afExtent[1] * m_akAxis[1],
            m_afExtent[2] * m_akAxis[2]};

    akVertex[0] = m_kCenter - akEAxis[0] - akEAxis[1] - akEAxis[2];
    akVertex[1] = m_kCenter + akEAxis[0] - akEAxis[1] - akEAxis[2];
    akVertex[2] = m_kCenter + akEAxis[0] + akEAxis[1] - akEAxis[2];
    akVertex[3] = m_kCenter - akEAxis[0] + akEAxis[1] - akEAxis[2];
    akVertex[4] = m_kCenter - akEAxis[0] - akEAxis[1] + akEAxis[2];
    akVertex[5] = m_kCenter + akEAxis[0] - akEAxis[1] + akEAxis[2];
    akVertex[6] = m_kCenter + akEAxis[0] + akEAxis[1] + akEAxis[2];
    akVertex[7] = m_kCenter - akEAxis[0] + akEAxis[1] + akEAxis[2];
}
//----------------------------------------------------------------------------
