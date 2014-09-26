// Magic Software, Inc.
// http://www.magic-software.com
// http://www.wild-magic.com
// Copyright (c) 2004.  All Rights Reserved
//
// The Wild Magic Library (WML) source code is supplied under the terms of
// the license agreement http://www.magic-software.com/License/WildMagic.pdf
// and may not be copied or disclosed except in accordance with the terms of
// that agreement.

//----------------------------------------------------------------------------
template <int N, class Real>
Matrix<N,Real>::Matrix ()
{
    // the matrix is uninitialized
}
//----------------------------------------------------------------------------
template <int N, class Real>
Matrix<N,Real>::Matrix (bool bZero)
{
    memset(m_afEntry,0,N*N*sizeof(Real));

    if ( !bZero )
    {
        for (int i = 0; i < N; i++)
            m_afEntry[I(i,i)] = (Real)1.0;
    }
}
//----------------------------------------------------------------------------
template <int N, class Real>
Matrix<N,Real>::Matrix (const Matrix& rkM)
{
    memcpy(m_afEntry,rkM.m_afEntry,N*N*sizeof(Real));
}
//----------------------------------------------------------------------------
template <int N, class Real>
void Matrix<N,Real>::MakeZero ()
{
    memset(m_afEntry,0,N*N*sizeof(Real));
}
//----------------------------------------------------------------------------
template <int N, class Real>
void Matrix<N,Real>::MakeIdentity ()
{
    memset(m_afEntry,0,N*N*sizeof(Real));
    for (int i = 0; i < N; i++)
        m_afEntry[I(i,i)] = (Real)1.0;
}
//----------------------------------------------------------------------------
template <int N, class Real>
void Matrix<N,Real>::MakeDiagonal (const Real* afDiag)
{
    memset(m_afEntry,0,N*N*sizeof(Real));
    for (int i = 0; i < N; i++)
        m_afEntry[I(i,i)] = afDiag[i];
}
//----------------------------------------------------------------------------
template <int N, class Real>
Matrix<N,Real>::operator const Real* () const
{
    return m_afEntry;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Matrix<N,Real>::operator Real* ()
{
    return m_afEntry;
}
//----------------------------------------------------------------------------
template <int N, class Real>
const Real* Matrix<N,Real>::operator[] (int iRow) const
{
    assert( 0 <= iRow && iRow < N );
    return &m_afEntry[N*iRow];
}
//----------------------------------------------------------------------------
template <int N, class Real>
Real* Matrix<N,Real>::operator[] (int iRow)
{
    assert( 0 <= iRow && iRow < N );
    return &m_afEntry[N*iRow];
}
//----------------------------------------------------------------------------
template <int N, class Real>
Real Matrix<N,Real>::operator() (int iRow, int iCol) const
{
    return m_afEntry[I(iRow,iCol)];
}
//----------------------------------------------------------------------------
template <int N, class Real>
Real& Matrix<N,Real>::operator() (int iRow, int iCol)
{
    return m_afEntry[I(iRow,iCol)];
}
//----------------------------------------------------------------------------
template <int N, class Real>
void Matrix<N,Real>::SetRow (int iRow, const Vector<N,Real>& rkV)
{
    assert( 0 <= iRow && iRow < N );
    for (int iCol = 0, i = N*iRow; iCol < N; iCol++, i++)
        m_afEntry[i] = rkV[iCol];
}
//----------------------------------------------------------------------------
template <int N, class Real>
Vector<N,Real> Matrix<N,Real>::GetRow (int iRow) const
{
    assert( 0 <= iRow && iRow < N );
    Vector<N,Real> kV;
    for (int iCol = 0, i = N*iRow; iCol < N; iCol++, i++)
        kV[iCol] = m_afEntry[i];
    return kV;
}
//----------------------------------------------------------------------------
template <int N, class Real>
void Matrix<N,Real>::SetColumn (int iCol, const Vector<N,Real>& rkV)
{
    assert( 0 <= iCol && iCol < N );
    for (int iRow = 0, i = iCol; iRow < N; iRow++, i += N)
        m_afEntry[i] = rkV[iRow];
}
//----------------------------------------------------------------------------
template <int N, class Real>
Vector<N,Real> Matrix<N,Real>::GetColumn (int iCol) const
{
    assert( 0 <= iCol && iCol < N );
    Vector<N,Real> kV;
    for (int iRow = 0, i = iCol; iRow < N; iRow++, i += N)
        kV[iRow] = m_afEntry[i];
    return kV;
}
//----------------------------------------------------------------------------
template <int N, class Real>
void Matrix<N,Real>::GetColumnMajor (Real* afCMajor) const
{
    for (int iRow = 0, i = 0; iRow < N; iRow++)
    {
        for (int iCol = 0; iCol < N; iCol++)
            afCMajor[i++] = m_afEntry[I(iCol,iRow)];
    }
}
//----------------------------------------------------------------------------
template <int N, class Real>
Matrix<N,Real>& Matrix<N,Real>::operator= (const Matrix& rkM)
{
    memcpy(m_afEntry,rkM.m_afEntry,N*N*sizeof(Real));
    return *this;
}
//----------------------------------------------------------------------------
template <int N, class Real>
bool Matrix<N,Real>::operator== (const Matrix& rkM) const
{
    return memcmp(m_afEntry,rkM.m_afEntry,N*N*sizeof(Real)) == 0;
}
//----------------------------------------------------------------------------
template <int N, class Real>
bool Matrix<N,Real>::operator!= (const Matrix& rkM) const
{
    return memcmp(m_afEntry,rkM.m_afEntry,N*N*sizeof(Real)) != 0;
}
//----------------------------------------------------------------------------
template <int N, class Real>
int Matrix<N,Real>::CompareArrays (const Matrix& rkM) const
{
    return memcmp(m_afEntry,rkM.m_afEntry,N*N*sizeof(Real));
}
//----------------------------------------------------------------------------
template <int N, class Real>
bool Matrix<N,Real>::operator<  (const Matrix& rkM) const
{
    return CompareArrays(rkM) < 0;
}
//----------------------------------------------------------------------------
template <int N, class Real>
bool Matrix<N,Real>::operator<= (const Matrix& rkM) const
{
    return CompareArrays(rkM) <= 0;
}
//----------------------------------------------------------------------------
template <int N, class Real>
bool Matrix<N,Real>::operator>  (const Matrix& rkM) const
{
    return CompareArrays(rkM) > 0;
}
//----------------------------------------------------------------------------
template <int N, class Real>
bool Matrix<N,Real>::operator>= (const Matrix& rkM) const
{
    return CompareArrays(rkM) >= 0;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Matrix<N,Real> Matrix<N,Real>::operator+ (const Matrix& rkM) const
{
    Matrix<N,Real> kSum;
    for (int i = 0; i < N*N; i++)
        kSum.m_afEntry[i] = m_afEntry[i] + rkM.m_afEntry[i];
    return kSum;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Matrix<N,Real> Matrix<N,Real>::operator- (const Matrix& rkM) const
{
    Matrix<N,Real> kDiff;
    for (int i = 0; i < N*N; i++)
        kDiff.m_afEntry[i] = m_afEntry[i] - rkM.m_afEntry[i];
    return kDiff;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Matrix<N,Real> Matrix<N,Real>::operator* (const Matrix& rkM) const
{
    Matrix<N,Real> kProd;
    for (int iRow = 0; iRow < N; iRow++)
    {
        for (int iCol = 0; iCol < N; iCol++)
        {
            int i = I(iRow,iCol);
            kProd.m_afEntry[i] = (Real)0.0f;
            for (int iMid = 0; iMid < N; iMid++)
            {
                kProd.m_afEntry[i] +=
                    m_afEntry[I(iRow,iMid)]*rkM.m_afEntry[I(iMid,iCol)];
            }
        }
    }
    return kProd;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Matrix<N,Real> Matrix<N,Real>::operator* (Real fScalar) const
{
    Matrix<N,Real> kProd;
    for (int i = 0; i < N*N; i++)
        kProd.m_afEntry[i] = fScalar*m_afEntry[i];
    return kProd;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Matrix<N,Real> Matrix<N,Real>::operator/ (Real fScalar) const
{
    Matrix<N,Real> kQuot;
    int i;

    if ( fScalar != (Real)0.0 )
    {
        Real fInvScalar = ((Real)1.0)/fScalar;
        for (i = 0; i < N*N; i++)
            kQuot.m_afEntry[i] = fInvScalar*m_afEntry[i];
    }
    else
    {
        for (i = 0; i < N*N; i++)
            kQuot.m_afEntry[i] = Math<Real>::MAX_REAL;
    }

    return kQuot;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Matrix<N,Real> Matrix<N,Real>::operator- () const
{
    Matrix<N,Real> kNeg;
    for (int i = 0; i < N*N; i++)
        kNeg.m_afEntry[i] = -m_afEntry[i];
    return kNeg;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Matrix<N,Real> operator* (Real fScalar, const Matrix<N,Real>& rkM)
{
    Matrix<N,Real> kProd;
    const Real* afMEntry = rkM;
    Real* afPEntry = kProd;
    for (int i = 0; i < N*N; i++)
        afPEntry[i] = fScalar*afMEntry[i];
    return kProd;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Matrix<N,Real>& Matrix<N,Real>::operator+= (const Matrix& rkM)
{
    for (int i = 0; i < N*N; i++)
        m_afEntry[i] += rkM.m_afEntry[i];
    return *this;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Matrix<N,Real>& Matrix<N,Real>::operator-= (const Matrix& rkM)
{
    for (int i = 0; i < N*N; i++)
        m_afEntry[i] -= rkM.m_afEntry[i];
    return *this;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Matrix<N,Real>& Matrix<N,Real>::operator*= (Real fScalar)
{
    for (int i = 0; i < N*N; i++)
        m_afEntry[i] *= fScalar;
    return *this;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Matrix<N,Real>& Matrix<N,Real>::operator/= (Real fScalar)
{
    int i;

    if ( fScalar != (Real)0.0 )
    {
        Real fInvScalar = ((Real)1.0)/fScalar;
        for (i = 0; i < N*N; i++)
            m_afEntry[i] *= fInvScalar;
    }
    else
    {
        for (i = 0; i < N*N; i++)
            m_afEntry[i] = Math<Real>::MAX_REAL;
    }

    return *this;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Matrix<N,Real> Matrix<N,Real>::Transpose () const
{
    Matrix<N,Real> kTranspose;
    for (int iRow = 0; iRow < N; iRow++)
    {
        for (int iCol = 0; iCol < N; iCol++)
            kTranspose.m_afEntry[I(iRow,iCol)] = m_afEntry[I(iCol,iRow)];
    }
    return kTranspose;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Matrix<N,Real> Matrix<N,Real>::TransposeTimes (const Matrix& rkM) const
{
    // P = A^T*B, P[r][c] = sum_m A[m][r]*B[m][c]
    Matrix<N,Real> kProd;
    for (int iRow = 0; iRow < N; iRow++)
    {
        for (int iCol = 0; iCol < N; iCol++)
        {
            int i = I(iRow,iCol);
            kProd.m_afEntry[i] = (Real)0.0;
            for (int iMid = 0; iMid < N; iMid++)
            {
                kProd.m_afEntry[i] +=
                    m_afEntry[I(iMid,iRow)]*rkM.m_afEntry[I(iMid,iCol)];
            }
        }
    }
    return kProd;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Matrix<N,Real> Matrix<N,Real>::TimesTranspose (const Matrix& rkM) const
{
    // P = A*B^T, P[r][c] = sum_m A[r][m]*B[c][m]
    Matrix<N,Real> kProd;
    for (int iRow = 0; iRow < N; iRow++)
    {
        for (int iCol = 0; iCol < N; iCol++)
        {
            int i = I(iRow,iCol);
            kProd.m_afEntry[i] = (Real)0.0;
            for (int iMid = 0; iMid < N; iMid++)
            {
                kProd.m_afEntry[i] +=
                    m_afEntry[I(iRow,iMid)]*rkM.m_afEntry[I(iCol,iRow)];
            }
        }
    }
    return kProd;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Vector<N,Real> Matrix<N,Real>::operator* (const Vector<N,Real>& rkV) const
{
    Vector<N,Real> kProd;
    for (int iRow = 0; iRow < N; iRow++)
    {
        kProd[iRow] = (Real)0.0;
        for (int iCol = 0; iCol < N; iCol++)
            kProd[iRow] += m_afEntry[I(iRow,iCol)]*rkV[iCol];
            
    }
    return kProd;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Vector<N,Real> operator* (const Vector<N,Real>& rkV,
    const Matrix<N,Real>& rkM)
{
    Vector<N,Real> kProd;
    const Real* afMEntry = rkM;
    Real* afPEntry = kProd;
    for (int iCol = 0; iCol < N; iCol++)
    {
        afPEntry[iCol] = (Real)0.0;
        for (int iRow = 0; iRow < N; iRow++)
            afPEntry[iCol] += rkV[iRow]*afMEntry[iCol + N*iRow];
            
    }
    return kProd;
}
//----------------------------------------------------------------------------
template <int N, class Real>
Real Matrix<N,Real>::QForm (const Vector<N,Real>& rkU,
    const Vector<N,Real>& rkV) const
{
    return rkU.Dot((*this)*rkV);
}
//----------------------------------------------------------------------------
template <int N, class Real>
int Matrix<N,Real>::I (int iRow, int iCol)
{
    assert( 0 <= iRow && iRow < N && 0 <= iCol && iCol < N );
    return iCol + N*iRow;
}
//----------------------------------------------------------------------------
