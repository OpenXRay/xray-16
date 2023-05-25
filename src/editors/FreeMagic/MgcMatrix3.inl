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
inline Matrix3::Matrix3()
{
    // For efficiency reasons, do not initialize matrix.
}
//----------------------------------------------------------------------------
inline Real *Matrix3::operator[](int iRow) const
{
    return (Real *)&m_aafEntry[iRow][0];
}
//----------------------------------------------------------------------------
inline Matrix3::operator Real *()
{
    return &m_aafEntry[0][0];
}
//----------------------------------------------------------------------------
