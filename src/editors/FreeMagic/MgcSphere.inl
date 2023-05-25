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
inline Sphere::Sphere()
{
    // no initialization for efficiency
}
//----------------------------------------------------------------------------
inline Vector3 &Sphere::Center()
{
    return m_kCenter;
}
//----------------------------------------------------------------------------
inline const Vector3 &Sphere::Center() const
{
    return m_kCenter;
}
//----------------------------------------------------------------------------
inline Real &Sphere::Radius()
{
    return m_fRadius;
}
//----------------------------------------------------------------------------
inline const Real &Sphere::Radius() const
{
    return m_fRadius;
}
//----------------------------------------------------------------------------
