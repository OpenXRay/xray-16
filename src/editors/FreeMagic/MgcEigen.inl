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
inline Real &Eigen::Matrix(int iRow, int iCol)
{
    return m_aafMat[iRow][iCol];
}
//----------------------------------------------------------------------------
inline Real Eigen::GetEigenvalue(int i) const
{
    return m_afDiag[i];
}
//----------------------------------------------------------------------------
inline Real Eigen::GetEigenvector(int iRow, int iCol) const
{
    return m_aafMat[iRow][iCol];
}
//----------------------------------------------------------------------------
inline Real *Eigen::GetEigenvalue()
{
    return m_afDiag;
}
//----------------------------------------------------------------------------
inline Real **Eigen::GetEigenvector()
{
    return m_aafMat;
}
//----------------------------------------------------------------------------
