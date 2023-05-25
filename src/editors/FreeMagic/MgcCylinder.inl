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

//----------------------------------------------------------------------------
inline Cylinder::Cylinder()
{
    // no initialization for efficiency
}
//----------------------------------------------------------------------------
inline Vector3 &Cylinder::Center()
{
    return m_kCenter;
}
//----------------------------------------------------------------------------
inline const Vector3 &Cylinder::Center() const
{
    return m_kCenter;
}
//----------------------------------------------------------------------------
inline Vector3 &Cylinder::Direction()
{
    return m_kDirection;
}
//----------------------------------------------------------------------------
inline const Vector3 &Cylinder::Direction() const
{
    return m_kDirection;
}
//----------------------------------------------------------------------------
inline Real &Cylinder::Height()
{
    return m_fHeight;
}
//----------------------------------------------------------------------------
inline Real Cylinder::Height() const
{
    return m_fHeight;
}
//----------------------------------------------------------------------------
inline Real &Cylinder::Radius()
{
    return m_fRadius;
}
//----------------------------------------------------------------------------
inline Real Cylinder::Radius() const
{
    return m_fRadius;
}
//----------------------------------------------------------------------------
inline bool &Cylinder::Capped()
{
    return m_bCapped;
}
//----------------------------------------------------------------------------
inline bool Cylinder::Capped() const
{
    return m_bCapped;
}
//----------------------------------------------------------------------------
inline Segment3 Cylinder::GetSegment() const
{
    Segment3 kSegment;
    kSegment.Direction() = m_fHeight * m_kDirection;
    kSegment.Origin() = m_kCenter - 0.5 * kSegment.Direction();
    return kSegment;
}
//----------------------------------------------------------------------------
inline void Cylinder::GenerateCoordinateSystem()
{
    Vector3::GenerateOrthonormalBasis(m_kU, m_kV, m_kDirection, true);
}
//----------------------------------------------------------------------------
inline Vector3 &Cylinder::U()
{
    return m_kU;
}
//----------------------------------------------------------------------------
inline const Vector3 &Cylinder::U() const
{
    return m_kU;
}
//----------------------------------------------------------------------------
inline Vector3 &Cylinder::V()
{
    return m_kV;
}
//----------------------------------------------------------------------------
inline const Vector3 &Cylinder::V() const
{
    return m_kV;
}
//----------------------------------------------------------------------------
inline Vector3 &Cylinder::W()
{
    return m_kDirection;
}
//----------------------------------------------------------------------------
inline const Vector3 &Cylinder::W() const
{
    return m_kDirection;
}
//----------------------------------------------------------------------------
